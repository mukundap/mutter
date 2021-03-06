#include "cogl-config.h"

#include <stdlib.h>

#include "test-unit.h"
#include "test-utils.h"

#include "cogl/winsys/cogl-onscreen-glx.h"
#include "cogl/winsys/cogl-onscreen-xlib.h"

#define FB_WIDTH 512
#define FB_HEIGHT 512

static gboolean cogl_test_is_verbose;

CoglContext *test_ctx;
CoglFramebuffer *test_fb;

static gboolean
check_flags (TestFlags flags,
             CoglRenderer *renderer)
{
  if (flags & TEST_REQUIREMENT_GL &&
      cogl_renderer_get_driver (renderer) != COGL_DRIVER_GL &&
      cogl_renderer_get_driver (renderer) != COGL_DRIVER_GL3)
    {
      return FALSE;
    }

  if (flags & TEST_REQUIREMENT_TEXTURE_RG &&
      !cogl_has_feature (test_ctx, COGL_FEATURE_ID_TEXTURE_RG))
    {
      return FALSE;
    }

  if (flags & TEST_REQUIREMENT_MAP_WRITE &&
      !cogl_has_feature (test_ctx, COGL_FEATURE_ID_MAP_BUFFER_FOR_WRITE))
    {
      return FALSE;
    }

  if (flags & TEST_REQUIREMENT_FENCE &&
      !cogl_has_feature (test_ctx, COGL_FEATURE_ID_FENCE))
    {
      return FALSE;
    }

  if (flags & TEST_KNOWN_FAILURE)
    {
      return FALSE;
    }

  return TRUE;
}

static gboolean
is_boolean_env_set (const char *variable)
{
  char *val = getenv (variable);
  gboolean ret;

  if (!val)
    return FALSE;

  if (g_ascii_strcasecmp (val, "1") == 0 ||
      g_ascii_strcasecmp (val, "on") == 0 ||
      g_ascii_strcasecmp (val, "true") == 0)
    ret = TRUE;
  else if (g_ascii_strcasecmp (val, "0") == 0 ||
           g_ascii_strcasecmp (val, "off") == 0 ||
           g_ascii_strcasecmp (val, "false") == 0)
    ret = FALSE;
  else
    {
      g_critical ("Spurious boolean environment variable value (%s=%s)",
                  variable, val);
      ret = TRUE;
    }

  return ret;
}

static CoglOnscreen *
create_onscreen (CoglContext *cogl_context,
                 int          width,
                 int          height)
{
  CoglDisplay *display = cogl_context_get_display (test_ctx);
  CoglRenderer *renderer = cogl_display_get_renderer (display);

  switch (cogl_renderer_get_winsys_id (renderer))
    {
    case COGL_WINSYS_ID_GLX:
#ifdef COGL_HAS_GLX_SUPPORT
      return COGL_ONSCREEN (cogl_onscreen_glx_new (cogl_context,
                                                   width, height));
#else
      g_assert_not_reached ();
      break;
#endif
    case COGL_WINSYS_ID_EGL_XLIB:
#ifdef COGL_HAS_EGL_PLATFORM_XLIB_SUPPORT
      return COGL_ONSCREEN (cogl_onscreen_xlib_new (cogl_context,
                                                    width, height));
#else
      g_assert_not_reached ();
      break;
#endif
    default:
      g_assert_not_reached ();
      return NULL;
    }
}

gboolean
test_utils_init (TestFlags requirement_flags,
                 TestFlags known_failure_flags)
{
  static int counter = 0;
  GError *error = NULL;
  CoglOnscreen *onscreen = NULL;
  CoglDisplay *display;
  CoglRenderer *renderer;
  gboolean missing_requirement;
  gboolean known_failure;

  if (counter != 0)
    g_critical ("We don't support running more than one test at a time\n"
                "in a single test run due to the state leakage that can\n"
                "cause subsequent tests to fail.\n"
                "\n"
                "If you want to run all the tests you should run\n"
                "$ make test-report");
  counter++;

  if (is_boolean_env_set ("COGL_TEST_VERBOSE") ||
      is_boolean_env_set ("V"))
    cogl_test_is_verbose = TRUE;

  /* NB: This doesn't have any effect since commit 47444dac of glib
   * because the environment variable is read in a magic constructor
   * so it is too late to set them here */
  if (g_getenv ("G_DEBUG"))
    {
      char *debug = g_strconcat (g_getenv ("G_DEBUG"), ",fatal-warnings", NULL);
      g_setenv ("G_DEBUG", debug, TRUE);
      g_free (debug);
    }
  else
    g_setenv ("G_DEBUG", "fatal-warnings", TRUE);

  g_setenv ("COGL_X11_SYNC", "1", 0);

  test_ctx = cogl_context_new (NULL, &error);
  if (!test_ctx)
    g_critical ("Failed to create a CoglContext: %s", error->message);

  display = cogl_context_get_display (test_ctx);
  renderer = cogl_display_get_renderer (display);

  missing_requirement = !check_flags (requirement_flags, renderer);
  known_failure = !check_flags (known_failure_flags, renderer);

  if (is_boolean_env_set ("COGL_TEST_ONSCREEN"))
    {
      onscreen = create_onscreen (test_ctx, 640, 480);
      test_fb = COGL_FRAMEBUFFER (onscreen);
    }
  else
    {
      CoglOffscreen *offscreen;
      CoglTexture2D *tex = cogl_texture_2d_new_with_size (test_ctx,
                                                          FB_WIDTH, FB_HEIGHT);
      offscreen = cogl_offscreen_new_with_texture (COGL_TEXTURE (tex));
      test_fb = COGL_FRAMEBUFFER (offscreen);
    }

  if (!cogl_framebuffer_allocate (test_fb, &error))
    g_critical ("Failed to allocate framebuffer: %s", error->message);

  cogl_framebuffer_clear4f (test_fb,
                            COGL_BUFFER_BIT_COLOR |
                            COGL_BUFFER_BIT_DEPTH |
                            COGL_BUFFER_BIT_STENCIL,
                            0, 0, 0, 1);

  if (missing_requirement)
    g_print ("WARNING: Missing required feature[s] for this test\n");
  else if (known_failure)
    g_print ("WARNING: Test is known to fail\n");

  return (!missing_requirement && !known_failure);
}

void
test_utils_fini (void)
{
  if (test_fb)
    g_object_unref (test_fb);

  if (test_ctx)
    cogl_object_unref (test_ctx);
}

static gboolean
compare_component (int a, int b)
{
  return ABS (a - b) <= 1;
}

void
test_utils_compare_pixel_and_alpha (const uint8_t *screen_pixel,
                                    uint32_t expected_pixel)
{
  /* Compare each component with a small fuzz factor */
  if (!compare_component (screen_pixel[0], expected_pixel >> 24) ||
      !compare_component (screen_pixel[1], (expected_pixel >> 16) & 0xff) ||
      !compare_component (screen_pixel[2], (expected_pixel >> 8) & 0xff) ||
      !compare_component (screen_pixel[3], (expected_pixel >> 0) & 0xff))
    {
      uint32_t screen_pixel_num = GUINT32_FROM_BE (*(uint32_t *) screen_pixel);
      char *screen_pixel_string =
        g_strdup_printf ("#%08x", screen_pixel_num);
      char *expected_pixel_string =
        g_strdup_printf ("#%08x", expected_pixel);

      g_assert_cmpstr (screen_pixel_string, ==, expected_pixel_string);

      g_free (screen_pixel_string);
      g_free (expected_pixel_string);
    }
}

void
test_utils_compare_pixel (const uint8_t *screen_pixel, uint32_t expected_pixel)
{
  /* Compare each component with a small fuzz factor */
  if (!compare_component (screen_pixel[0], expected_pixel >> 24) ||
      !compare_component (screen_pixel[1], (expected_pixel >> 16) & 0xff) ||
      !compare_component (screen_pixel[2], (expected_pixel >> 8) & 0xff))
    {
      uint32_t screen_pixel_num = GUINT32_FROM_BE (*(uint32_t *) screen_pixel);
      char *screen_pixel_string =
        g_strdup_printf ("#%06x", screen_pixel_num >> 8);
      char *expected_pixel_string =
        g_strdup_printf ("#%06x", expected_pixel >> 8);

      g_assert_cmpstr (screen_pixel_string, ==, expected_pixel_string);

      g_free (screen_pixel_string);
      g_free (expected_pixel_string);
    }
}

void
test_utils_check_pixel (CoglFramebuffer *test_fb,
                        int x, int y, uint32_t expected_pixel)
{
  uint8_t pixel[4];

  cogl_framebuffer_read_pixels (test_fb,
                                x, y, 1, 1,
                                COGL_PIXEL_FORMAT_RGBA_8888_PRE,
                                pixel);

  test_utils_compare_pixel (pixel, expected_pixel);
}

void
test_utils_check_pixel_and_alpha (CoglFramebuffer *test_fb,
                                  int x, int y, uint32_t expected_pixel)
{
  uint8_t pixel[4];

  cogl_framebuffer_read_pixels (test_fb,
                                x, y, 1, 1,
                                COGL_PIXEL_FORMAT_RGBA_8888_PRE,
                                pixel);

  test_utils_compare_pixel_and_alpha (pixel, expected_pixel);
}

void
test_utils_check_pixel_rgb (CoglFramebuffer *test_fb,
                            int x, int y, int r, int g, int b)
{
  g_return_if_fail (r >= 0);
  g_return_if_fail (g >= 0);
  g_return_if_fail (b >= 0);
  g_return_if_fail (r <= 0xFF);
  g_return_if_fail (g <= 0xFF);
  g_return_if_fail (b <= 0xFF);

  test_utils_check_pixel (test_fb, x, y,
                          (((guint32) r) << 24) |
                          (((guint32) g) << 16) |
                          (((guint32) b) << 8));
}

void
test_utils_check_region (CoglFramebuffer *test_fb,
                         int x, int y,
                         int width, int height,
                         uint32_t expected_rgba)
{
  uint8_t *pixels, *p;

  pixels = p = g_malloc (width * height * 4);
  cogl_framebuffer_read_pixels (test_fb,
                                x,
                                y,
                                width,
                                height,
                                COGL_PIXEL_FORMAT_RGBA_8888,
                                p);

  /* Check whether the center of each division is the right color */
  for (y = 0; y < height; y++)
    for (x = 0; x < width; x++)
      {
        test_utils_compare_pixel (p, expected_rgba);
        p += 4;
      }

  g_free (pixels);
}

CoglTexture *
test_utils_create_color_texture (CoglContext *context,
                                 uint32_t color)
{
  CoglTexture2D *tex_2d;

  color = GUINT32_TO_BE (color);

  tex_2d = cogl_texture_2d_new_from_data (context,
                                          1, 1, /* width/height */
                                          COGL_PIXEL_FORMAT_RGBA_8888_PRE,
                                          4, /* rowstride */
                                          (uint8_t *) &color,
                                          NULL);

  return COGL_TEXTURE (tex_2d);
}

gboolean
cogl_test_verbose (void)
{
  return cogl_test_is_verbose;
}

static void
set_auto_mipmap_cb (CoglTexture *sub_texture,
                    const float *sub_texture_coords,
                    const float *meta_coords,
                    void *user_data)
{
  cogl_primitive_texture_set_auto_mipmap (COGL_PRIMITIVE_TEXTURE (sub_texture),
                                          FALSE);
}

CoglTexture *
test_utils_texture_new_with_size (CoglContext *ctx,
                                  int width,
                                  int height,
                                  TestUtilsTextureFlags flags,
                                  CoglTextureComponents components)
{
  CoglTexture *tex;
  GError *skip_error = NULL;

  /* First try creating a fast-path non-sliced texture */
  tex = COGL_TEXTURE (cogl_texture_2d_new_with_size (ctx, width, height));

  cogl_texture_set_components (tex, components);

  if (!cogl_texture_allocate (tex, &skip_error))
    {
      g_error_free (skip_error);
      cogl_object_unref (tex);
      tex = NULL;
    }

  if (!tex)
    {
      /* If it fails resort to sliced textures */
      int max_waste = flags & TEST_UTILS_TEXTURE_NO_SLICING ?
        -1 : COGL_TEXTURE_MAX_WASTE;
      CoglTexture2DSliced *tex_2ds =
        cogl_texture_2d_sliced_new_with_size (ctx,
                                              width,
                                              height,
                                              max_waste);
      tex = COGL_TEXTURE (tex_2ds);

      cogl_texture_set_components (tex, components);
    }

  if (flags & TEST_UTILS_TEXTURE_NO_AUTO_MIPMAP)
    {
      /* To be able to iterate the slices of a #CoglTexture2DSliced we
       * need to ensure the texture is allocated... */
      cogl_texture_allocate (tex, NULL); /* don't catch exceptions */

      cogl_meta_texture_foreach_in_region (COGL_META_TEXTURE (tex),
                                           0, 0, 1, 1,
                                           COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE,
                                           COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE,
                                           set_auto_mipmap_cb,
                                           NULL); /* don't catch exceptions */
    }

  cogl_texture_allocate (tex, NULL);

  return tex;
}

CoglTexture *
test_utils_texture_new_from_bitmap (CoglBitmap *bitmap,
                                    TestUtilsTextureFlags flags,
                                    gboolean premultiplied)
{
  CoglAtlasTexture *atlas_tex;
  CoglTexture *tex;
  GError *internal_error = NULL;

  if (!flags)
    {
      /* First try putting the texture in the atlas */
      atlas_tex = cogl_atlas_texture_new_from_bitmap (bitmap);

      cogl_texture_set_premultiplied (COGL_TEXTURE (atlas_tex), premultiplied);

      if (cogl_texture_allocate (COGL_TEXTURE (atlas_tex), &internal_error))
        return COGL_TEXTURE (atlas_tex);

      cogl_object_unref (atlas_tex);
    }

  g_clear_error (&internal_error);

  /* If that doesn't work try a fast path 2D texture */
  tex = COGL_TEXTURE (cogl_texture_2d_new_from_bitmap (bitmap));

  cogl_texture_set_premultiplied (tex, premultiplied);

  if (g_error_matches (internal_error,
                       COGL_SYSTEM_ERROR,
                       COGL_SYSTEM_ERROR_NO_MEMORY))
    {
      g_assert_not_reached ();
      return NULL;
    }

  g_clear_error (&internal_error);

  if (!tex)
    {
      /* Otherwise create a sliced texture */
      int max_waste = flags & TEST_UTILS_TEXTURE_NO_SLICING ?
        -1 : COGL_TEXTURE_MAX_WASTE;
      CoglTexture2DSliced *tex_2ds =
        cogl_texture_2d_sliced_new_from_bitmap (bitmap, max_waste);
      tex = COGL_TEXTURE (tex_2ds);

      cogl_texture_set_premultiplied (tex, premultiplied);
    }

  if (flags & TEST_UTILS_TEXTURE_NO_AUTO_MIPMAP)
    {
      cogl_meta_texture_foreach_in_region (COGL_META_TEXTURE (tex),
                                           0, 0, 1, 1,
                                           COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE,
                                           COGL_PIPELINE_WRAP_MODE_CLAMP_TO_EDGE,
                                           set_auto_mipmap_cb,
                                           NULL); /* don't catch exceptions */
    }

  cogl_texture_allocate (tex, NULL);

  return tex;
}

CoglTexture *
test_utils_texture_new_from_data (CoglContext *ctx,
                                  int width,
                                  int height,
                                  TestUtilsTextureFlags flags,
                                  CoglPixelFormat format,
                                  int rowstride,
                                  const uint8_t *data)
{
  CoglBitmap *bmp;
  CoglTexture *tex;

  g_assert_cmpint (format, !=, COGL_PIXEL_FORMAT_ANY);
  g_assert (data != NULL);

  /* Wrap the data into a bitmap */
  bmp = cogl_bitmap_new_for_data (ctx,
                                  width, height,
                                  format,
                                  rowstride,
                                  (uint8_t *) data);

  tex = test_utils_texture_new_from_bitmap (bmp, flags, TRUE);

  cogl_object_unref (bmp);

  return tex;
}
