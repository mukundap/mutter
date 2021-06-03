/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2020-21 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;
 *
 * Authors:
 *	 Uday Kiran Pichika <pichika.uday.kiran@intel.com>
 *
 * Some of the shader codes have been taken from Harish Krupo weston's branch
 * https://gitlab.freedesktop.org/harishkrupo/weston/-/commit/c465f7fdf604aae39d9209fae6f6246641db32d2
 */

/* When converting from YUV BT.709 to BT.2020, below stages will be involved
 * Stage1 : YUV to RGB --> After this conversion, RGB is in non-linear fashion
 * Stage2 : Apply EOTF curve --> non-linear to Linear conversion
 * Stage3 : convert BT.709 RGB colorspace to BT.2020 colorspace
 * Stage4 : Apply OETF curve --> Linear to non-linear conversion
 *
 */
 
 #include "compositor/meta-gl-shaders.h"
#include <string.h>
 
 G_DEFINE_TYPE (MetaGLShaders, meta_gl_shaders, G_TYPE_OBJECT);
 
 /* eotfs -- Non-linearRGB to Liner RGB*/
static const char eotf_srgb[] =
    "float eotf_srgb_single(float c) {\n"
    "    return c < 0.04045 ? c / 12.92 : pow(((c + 0.055) / 1.055), 2.4);\n"
    "}\n"
    "\n"
    "vec3 eotf_srgb(vec3 color) {\n"
    "    float r = eotf_srgb_single(color.r);\n"
    "    float g = eotf_srgb_single(color.g);\n"
    "    float b = eotf_srgb_single(color.b);\n"
    "    return vec3(r, g, b);\n"
    "}\n"
    "\n"
    "vec3 eotf(vec3 color) {\n"
    "    return sign(color) * eotf_srgb(abs(color.rgb));\n"
    "}\n"
    "\n";

/* converstion from Linear BT.709 to BT.2020 colorspace */
static const char bt709_to_bt2020_srgb[] =
    "vec3 bt709_to_bt2020_srgb(vec3 color) {\n"
    "  float r = 0.6274 * color.r + 0.3293 * color.g  + 0.0433 * color.b;\n"
    "  float g = 0.0691 * color.r + 0.9195 * color.g  + 0.0114 * color.b;\n"
    "  float b = 0.0164 * color.r + 0.0880 * color.g  + 0.8956 * color.b;\n"
    "  return vec3(r,g,b);\n"
    "}\n";

/* converstion from Linear BT.2020 to BT.709 colorspace */
static const char bt2020_to_bt709_srgb[] =
    "vec3 bt709_to_bt2020_srgb(vec3 color) {\n"
    "  float r = 1.6605 * color.r - 0.5876 * color.g  - 0.0728 * color.b;\n"
    "  float g = -0.1246 * color.r + 1.1329 * color.g  - 0.0083 * color.b;\n"
    "  float b = -0.0182 * color.r - 0.1006 * color.g  + 1.1187 * color.b;\n"
    "  return vec3(r,g,b);\n"
    "}\n";

/* oetfs -- Linear RGB to Non-Linear RGB*/
static const char oetf_srgb[] =
    "float oetf_srgb_single(float c) {\n"
    "    float ret = 0.0;\n"
    "    if (c < 0.0031308) {\n"
    "        ret = 12.92 * c;\n"
    "    } else {\n"
    "        ret = 1.055 * pow(c, 1.0 / 2.4) - 0.055;\n"
    "    }\n"
    "    return ret;\n"
    "}\n"
    "\n"
    "vec3 oetf_srgb(vec3 color) {\n"
    "    float r = oetf_srgb_single(color.r);\n"
    "    float g = oetf_srgb_single(color.g);\n"
    "    float b = oetf_srgb_single(color.b);\n"
    "    return vec3(r, g, b);\n"
    "}\n"
    "\n"
    "vec3 oetf(vec3 linear) {\n"
    "    return sign(linear) * oetf_srgb(abs(linear.rgb));\n"
    "}\n"
    "\n";

static const char layer_func[] =
    " vec4 layer_func(vec2 st) {\n"
    "{\n"
    "    vec3 rgb =texture2D (cogl_sampler0, UV).rgb;\n"
    "    vec3 linear = eotf(rgb);\n"
    "    vec3 csc = bt709_to_bt2020_srgb(linear);\n"
    "    vec3 nonlinear = oetf(csc);\n"
    "    return vec4(nonlinear,1.0);\n"
    "}\n"
    "\n";

static const char bt709_to_bt2020_full_shader[] =
    "float eotf_srgb_single(float c) {\n"
    "    return c < 0.04045 ? c / 12.92 : pow(((c + 0.055) / 1.055), 2.4);\n"
    "}\n"
    "\n"
    "vec3 eotf_srgb(vec3 color) {\n"
    "    float r = eotf_srgb_single(color.r);\n"
    "    float g = eotf_srgb_single(color.g);\n"
    "    float b = eotf_srgb_single(color.b);\n"
    "    return vec3(r, g, b);\n"
    "}\n"
    "\n"
    "vec3 eotf(vec3 color) {\n"
    "    return sign(color) * eotf_srgb(abs(color.rgb));\n"
    "}\n"
    "\n"
    "vec3 bt709_to_bt2020_srgb(vec3 color) {\n"
    "  float r = 0.6274 * color.r + 0.3293 * color.g  + 0.0433 * color.b;\n"
    "  float g = 0.0691 * color.r + 0.9195 * color.g  + 0.0114 * color.b;\n"
    "  float b = 0.0164 * color.r + 0.0880 * color.g  + 0.8956 * color.b;\n"
    "  return vec3(r,g,b);\n"
    "}\n"
    "float oetf_srgb_single(float c) {\n"
    "    float ret = 0.0;\n"
    "    if (c < 0.0031308) {\n"
    "        ret = 12.92 * c;\n"
    "    } else {\n"
    "        ret = 1.055 * pow(c, 1.0 / 2.4) - 0.055;\n"
    "    }\n"
    "    return ret;\n"
    "}\n"
    "\n"
    "vec3 oetf_srgb(vec3 color) {\n"
    "    float r = oetf_srgb_single(color.r);\n"
    "    float g = oetf_srgb_single(color.g);\n"
    "    float b = oetf_srgb_single(color.b);\n"
    "    return vec3(r, g, b);\n"
    "}\n"
    "\n"
    "vec3 oetf(vec3 linear) {\n"
    "    return sign(linear) * oetf_srgb(abs(linear.rgb));\n"
    "}\n"
    "\n"
    " vec4 layer_func(vec2 st) \n"
    "{\n"
    "    vec3 pqr = texture2D (cogl_sampler0, st).rgb;\n"
    "    vec3 linear = eotf(pqr);\n"
    "    vec3 csc = bt709_to_bt2020_srgb(linear);\n"
    "    vec3 nonlinear = oetf(csc);\n"
    "    return vec4(nonlinear,1.0);\n"
    "}\n"
    "\n";

static const char bt2020_to_bt709_full_shader[] =
    "float eotf_srgb_single(float c) {\n"
    "    return c < 0.04045 ? c / 12.92 : pow(((c + 0.055) / 1.055), 2.4);\n"
    "}\n"
    "\n"
    "vec3 eotf_srgb(vec3 color) {\n"
    "    float r = eotf_srgb_single(color.r);\n"
    "    float g = eotf_srgb_single(color.g);\n"
    "    float b = eotf_srgb_single(color.b);\n"
    "    return vec3(r, g, b);\n"
    "}\n"
    "\n"
    "vec3 eotf(vec3 color) {\n"
    "    return sign(color) * eotf_srgb(abs(color.rgb));\n"
    "}\n"
    "\n"
    "vec3 bt2020_to_bt079_srgb(vec3 color) {\n"
    "  float r = 1.6605 * color.r - 0.5876 * color.g  - 0.0728 * color.b;\n"
    "  float g = -0.1246 * color.r + 1.1329 * color.g  - 0.0083 * color.b;\n"
    "  float b = -0.0182 * color.r - 0.1006 * color.g  + 1.1187 * color.b;\n"
    "  return vec3(r,g,b);\n"
    "}\n"
    "float oetf_srgb_single(float c) {\n"
    "    float ret = 0.0;\n"
    "    if (c < 0.0031308) {\n"
    "        ret = 12.92 * c;\n"
    "    } else {\n"
    "        ret = 1.055 * pow(c, 1.0 / 2.4) - 0.055;\n"
    "    }\n"
    "    return ret;\n"
    "}\n"
    "\n"
    "vec3 oetf_srgb(vec3 color) {\n"
    "    float r = oetf_srgb_single(color.r);\n"
    "    float g = oetf_srgb_single(color.g);\n"
    "    float b = oetf_srgb_single(color.b);\n"
    "    return vec3(r, g, b);\n"
    "}\n"
    "\n"
    "vec3 oetf(vec3 linear) {\n"
    "    return sign(linear) * oetf_srgb(abs(linear.rgb));\n"
    "}\n"
    "\n"
    " vec4 layer_func(vec2 st) \n"
    "{\n"
    "    vec3 pqr = texture2D (cogl_sampler0, st).rgb;\n"
    "    vec3 linear = eotf(pqr);\n"
    "    vec3 csc = bt2020_to_bt079_srgb(linear);\n"
    "    vec3 nonlinear = oetf(csc);\n"
    "    return vec4(nonlinear,1.0);\n"
    "}\n"
    "\n";

MetaGLShaders *
meta_gl_shaders_new ()
{
  MetaGLShaders *gl_shaders = g_object_new(META_TYPE_GL_SHADERS, NULL);

  uint32_t key_requirements =
		SHADER_KEY_VARIANT_DEGAMMA_MASK |
		SHADER_KEY_VARIANT_CSC_MATRIX |
		SHADER_KEY_VARIANT_GAMMA_MASK;
 
  gl_shaders->shader_requirements &= ~key_requirements;

  return gl_shaders;
}

CoglSnippet *
meta_gl_shaders_get_vertex_shader_snippet_test()
{
  const char *vertex_hook;
  CoglSnippet *snippet;

  // TODO create the vertex shader string
  vertex_hook = NULL;

  // Create the Vertex snippet based on the shader string and return
  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_VERTEX_GLOBALS,
                                NULL,
                                vertex_hook);
  return snippet;
}

CoglSnippet *
meta_gl_shaders_get_fragment_shader_snippet(uint32_t shader_requirements)
{
  const char *fragment_hook;
  CoglSnippet *snippet;

  // TODO need to create the fragment shader hook based on
  // input colorspace and output colorspace.
  // Shader string should be created dynamically and assigned here
  if(shader_requirements & SHADER_KEY_VARIANT_CSC_BT2020_TO_BT709)
	fragment_hook = bt2020_to_bt709_full_shader;
  else if (shader_requirements & SHADER_KEY_VARIANT_CSC_BT709_TO_BT2020)
    fragment_hook = bt709_to_bt2020_full_shader;

  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_FRAGMENT_GLOBALS,
                                fragment_hook,
                                NULL);
  return snippet;
}

CoglSnippet *
meta_gl_shaders_get_layer_snippet()
{
  const char *layer_hook;
  CoglSnippet *snippet;

  // TODO double check here
  layer_hook = "  cogl_layer = layer_func(cogl_tex_coord0_in.st);\n";

  snippet = cogl_snippet_new (COGL_SNIPPET_HOOK_LAYER_FRAGMENT,
                                NULL,
                                layer_hook);
  return snippet;

}

static void
meta_gl_shaders_finalize (GObject *object)
{
  MetaGLShaders *manager = META_GL_SHADERS(object);

  G_OBJECT_CLASS (meta_gl_shaders_parent_class)->finalize (object);
}

static void
meta_gl_shaders_init (MetaGLShaders *gl_shaders)
{
}

static void
meta_gl_shaders_class_init (MetaGLShadersClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = meta_gl_shaders_finalize;
}