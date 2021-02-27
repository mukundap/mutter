#ifndef META_GL_SHADERS_H
#define META_GL_SHADERS_H

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>

#include "config.h"
#include "cogl/cogl.h"

#define META_TYPE_GL_SHADERS (meta_gl_shaders_get_type ())

typedef enum
{
  SHADER_KEY_VARIANT_RGBX = 1 << 0,
  SHADER_KEY_VARIANT_RGBA = 1 << 1,
  SHADER_KEY_VARIANT_Y_U_V = 1 << 2,
  SHADER_KEY_VARIANT_Y_UV = 1 << 3,
  SHADER_KEY_VARIANT_Y_XUXV = 1 << 4,
  SHADER_KEY_VARIANT_SOLID = 1 << 5,
  SHADER_KEY_VARIANT_COLOR_INVERT = 1 << 6,
  SHADER_KEY_VARIANT_EXTERNAL = 1 << 7,
  SHADER_KEY_VARIANT_MASK = 0xFF,

  /* 8 for Debugging */
  SHADER_KEY_VARIANT_DEBUG = 1 << 8,

  /* 9 for csc matrix */
  SHADER_KEY_VARIANT_CSC_MATRIX = 1 << 9,
  SHADER_KEY_VARIANT_CSC = 1 << 10,

  /* [12..15] for degamma curve */
  SHADER_KEY_VARIANT_DEGAMMA_LINEAR = 1 << 12,
  SHADER_KEY_VARIANT_DEGAMMA_SRGB = 1 << 13,
  SHADER_KEY_VARIANT_DEGAMMA_PQ = 1 << 14,
  SHADER_KEY_VARIANT_DEGAMMA_HLG = 1 << 15,
  SHADER_KEY_VARIANT_DEGAMMA_MASK = 0xF000,
  SHADER_KEY_VARIANT_DEGAMMA_SHIFT = 12,

  /* [16..19] for gamma curves */
  SHADER_KEY_VARIANT_GAMMA_LINEAR = 1 << 16,
  SHADER_KEY_VARIANT_GAMMA_SRGB = 1 << 17,
  SHADER_KEY_VARIANT_GAMMA_PQ = 1 << 18,
  SHADER_KEY_VARIANT_GAMMA_HLG = 1 << 19,
  SHADER_KEY_VARIANT_GAMMA_MASK = 0xF0000,
  SHADER_KEY_VARIANT_GAMMA_SHIFT = 16,

  /* [20..23] for Tone mapping requirements */
  SHADER_KEY_VARIANT_TONE_MAP_NONE = 1 << 20,
  SHADER_KEY_VARIANT_TONE_MAP_HDR_TO_SDR = 1 << 21,
  SHADER_KEY_VARIANT_TONE_MAP_SDR_TO_HDR = 1 << 22,
  SHADER_KEY_VARIANT_TONE_MAP_HDR_TO_HDR = 1 << 23,
  SHADER_KEY_VARIANT_TONE_MAP_MASK = 0xF00000,
}MetaGLShaderKeyVariant;

typedef struct _MetaGLShaders
{
  GObject parent;

  char* fragment_source;
  uint32_t frag_shader_alloc;
  uint32_t frag_shader_size;
  uint32_t shader_requirements;

  MetaGLShaderKeyVariant shader_key_variant;

} MetaGLShaders;

G_DECLARE_FINAL_TYPE (MetaGLShaders, meta_gl_shaders,
                      META, GL_SHADERS, GObject);

MetaGLShaders *meta_gl_shaders_new (MetaGLShaderKeyVariant shader_variant);
CoglSnippet * meta_gl_shaders_get_vertex_shader_snippet();
CoglSnippet * meta_gl_shaders_get_fragment_shader_snippet();
CoglSnippet * meta_gl_shaders_get_layer_snippet();

#endif