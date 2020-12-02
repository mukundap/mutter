/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/*
 * Copyright (C) 2020 Intel Corporation.
 * Copyright (C) 2020 Red Hat
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
 *   Uday Kiran Pichika <pichika.uday.kiran@intel.com>
 *   Naveen Kumar <naveen1.kumar@intel.com>
 *   Jonas Adhal <jadahl@redhat.com>
 *
 */

/*
 * MetaColorManager : This class will perform the Color Management operations
 * like invoking the methods to convert input surface colorspace to the target colorspace.
 * Target colorspace can be readble from MetaOutput(read from Edid parsing).
 *
 * MetaColorManager will take the decision to perform the  blending either using
 * Display HW path/GL Shader path/ Media Engine path.
 *
 * If the Display HW does not support the selected target colorspace, then the conversion
 * will happen through GL Shaders
 *
 * TODO : Need to check how the arbitrary colorspace blending will happen by default
 *
 * TODO : Need to decide the ColorTransformPath 
 *
 *
 */
#include "config.h"

#include "backends/meta-color-manager.h"
#include "backends/meta-output.h"
#include "meta/util.h"

typedef struct _MetaColorManager
{
  GObject parent;
  MetaBackend *backend;

  MetaColorTransformPath path;

  gboolean gl_extn_support;

} MetaColorManager;

G_DEFINE_TYPE (MetaColorManager, meta_color_manager, G_TYPE_OBJECT);

MetaColorManager *
meta_color_manager_new (MetaBackend *backend)
{
  MetaColorManager *cm;

  cm = g_object_new (META_TYPE_COLOR_MANAGER, NULL);
  cm->backend = backend;
  cm->gl_extn_support = FALSE;

  return cm;
}

gboolean
meta_color_manager_check_gl_extn_support(MetaBackend *backend)
{
  MetaEgl *egl = meta_backend_get_egl (backend);
  ClutterBackend *clutter_backend = meta_backend_get_clutter_backend (backend);
  CoglContext *cogl_context = clutter_backend_get_cogl_context (clutter_backend);
  EGLDisplay egl_display = cogl_egl_context_get_egl_display (cogl_context);

  g_assert (backend && egl && clutter_backend && cogl_context && egl_display);

  if (!meta_egl_has_extensions (egl, egl_display, NULL,
                                "GL_OES_EGL_image_external", NULL))
    return FALSE;

  g_print("UDAY --- system supports GL_OES_EGL_IMAGE_Extenstion  \n");
  return TRUE;
}

uint16_t
meta_color_manager_get_target_colorspace(MetaBackend *backend)
{
  MetaOutput *output;
  GList *gpus;
  GList *outputs;
  uint16_t supported_colorspaces;

  /* TODO : Below changes are to handle the primary monitor for timebeing.
     Need to do the changes if multiples GPU and Outputs are present */
  gpus = meta_backend_get_gpus (backend);
  outputs = meta_gpu_get_outputs (g_list_first (gpus)->data);
  output = g_list_first(outputs)->data;

  supported_colorspaces = meta_output_get_supported_colorspaces(output);

  /* TODO : make the decision here on colortransform path. But more
   * brainstorming is needed on how to get the suface(s) details here.
   * May be it would be easy if the decision to be taken at
   * MetaWaylandColorManagement, because it is having the surface details */

  meta_verbose("%s:%s -- supported_colorspaces = %u ", __FILE__,__func__, supported_colorspaces);

  return supported_colorspaces;
}

static void
meta_color_manager_finalize (GObject *object)
{
  MetaColorManager *manager = META_COLOR_MANAGER(object);

  G_OBJECT_CLASS (meta_color_manager_parent_class)->finalize (object);
}

static void
meta_color_manager_init (MetaColorManager *color_manager)
{
}

static void
meta_color_manager_class_init (MetaColorManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = meta_color_manager_finalize;
}
