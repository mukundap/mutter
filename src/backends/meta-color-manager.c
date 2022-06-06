/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2021 Intel Corporation.
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
 */

/*
 * MetaColorManager : Class works as an interface to perform the Color Space operations
 * to convert input surface colorspace to the target colorspace.
 *
 * MetaColorManager will take the decision to perform colorSpace conversion
 * using Display HW path/GL Shader path/ Media Engine path.
 *
 * If the Display HW does not support the selected target colorspace, then the
 * conversion will happen through GL Shaders
 */

#include "config.h"

#include "backends/meta-color-manager.h"
#include "backends/meta-output.h"
#include "meta/util.h"
#include "meta/meta-color-space.h"

typedef struct _MetaColorManager
{
  GObject parent;
  MetaBackend *backend;

  MetaColorTransformPath path;

  gboolean gl_extn_support;
  gboolean display_supports_colorspace;
  gboolean use_gl_shaders;
  uint32_t client_colorspace;
  uint16_t target_colorspace;
  gboolean needs_csc;
  MetaGLShaders *gl_shaders;

} MetaColorManager;

G_DEFINE_TYPE (MetaColorManager, meta_color_manager, G_TYPE_OBJECT);

MetaColorManager *
meta_color_manager_new (MetaBackend *backend)
{
  MetaColorManager *cm;

  cm = g_object_new (META_TYPE_COLOR_MANAGER, NULL);
  cm->backend = backend;
  cm->gl_extn_support = FALSE;

  MetaGLShaders *gl_shaders = meta_gl_shaders_new ();
  cm->gl_shaders = gl_shaders;

  return cm;
}

/* TODO Getting the target colorspace should be implemented by
 * considering multi monitor and multi gpu.
 * For time being, it is implemented by considering single monitor
 */
uint16_t
meta_color_manager_get_target_colorspace (MetaBackend *backend)
{
  MetaColorManager *color_manager = meta_backend_get_color_manager (backend);
  MetaOutput *output;
  GList *gpus;
  GList *outputs;
  uint16_t supported_colorspaces;
  uint16_t target_colorspace = META_COLORSPACE_TYPE_xvYCC709;
  gboolean display_supports_colorspace = FALSE;

  /* TODO : Below changes are to handle the primary monitor for timebeing.
     Need to do the changes if multiples GPU and Outputs are present */
  gpus = meta_backend_get_gpus (backend);
  outputs = meta_gpu_get_outputs (g_list_first (gpus)->data);
  output = g_list_first (outputs)->data;

  display_supports_colorspace =
         meta_output_get_display_supports_colorspace (output);
  color_manager->display_supports_colorspace = display_supports_colorspace;

  supported_colorspaces = meta_output_get_supported_colorspaces (output);
  if (!supported_colorspaces) {
    meta_verbose("Monitor does not support any extended color spaces. Considering BT709 by default\n");
    return target_colorspace;
  }

  if (supported_colorspaces & META_COLORSPACE_TYPE_BT2020RGB)
    target_colorspace = META_COLORSPACE_TYPE_BT2020RGB;
  else if(supported_colorspaces & META_COLORSPACE_TYPE_BT2020YCC)
    target_colorspace = META_COLORSPACE_TYPE_BT2020YCC;
  else if(supported_colorspaces & META_COLORSPACE_TYPE_xvYCC709)
    target_colorspace = META_COLORSPACE_TYPE_xvYCC709;

  return target_colorspace;
}

gboolean
meta_color_manager_get_use_glshaders (void)
{
  MetaColorManager *color_manager =
         meta_backend_get_color_manager (meta_get_backend ());

  return color_manager->use_gl_shaders;
}

gboolean
meta_color_manager_maybe_needs_csc (void)
{
  MetaColorManager *color_manager =
         meta_backend_get_color_manager (meta_get_backend ());

  return color_manager->needs_csc;
}

void
meta_color_manager_get_colorspaces (uint32_t *client_colorspace,
                                    uint16_t *target_colorspace)
{
  MetaColorManager *color_manager =
         meta_backend_get_color_manager (meta_get_backend ());
  *client_colorspace = color_manager->client_colorspace;
  *target_colorspace = color_manager->target_colorspace;
}

uint16_t
meta_color_manager_map_targetCS_to_clientCS (uint16_t target_colorspace)
{
  if ((target_colorspace & META_COLORSPACE_TYPE_BT2020RGB) ||
      (target_colorspace & META_COLORSPACE_TYPE_BT2020YCC))
      return META_CS_BT2020;
  else if (target_colorspace & META_COLORSPACE_TYPE_xvYCC709)
      return META_CS_BT709;
  else
      return META_CS_UNKNOWN;
}

void
meta_color_manager_perform_csc (uint32_t client_colorspace)
{
  MetaBackend *backend = meta_get_backend ();
  MetaColorManager *color_manager = meta_backend_get_color_manager (backend);
  uint16_t target_colorspace;
  uint16_t target_cs;
  gboolean needs_csc = FALSE;
  gboolean display_supports_colorspace = FALSE;

  target_cs = meta_color_manager_get_target_colorspace (backend);
  target_colorspace = meta_color_manager_map_targetCS_to_clientCS (target_cs);

  needs_csc = (target_colorspace != META_CS_UNKNOWN) &&
                   (target_colorspace != client_colorspace);

  color_manager->client_colorspace = client_colorspace;
  color_manager->target_colorspace = target_colorspace;
  color_manager->needs_csc = needs_csc;

  meta_verbose("%s: needs_csc = %s, target_colorspace = %d\n", __func__,
                          needs_csc ? "TRUE" : "FALSE", target_colorspace);

  display_supports_colorspace = color_manager->display_supports_colorspace;

  if (!needs_csc)
    return;

  if (display_supports_colorspace)
    {
      color_manager->use_gl_shaders = FALSE;
      meta_verbose("Using Display DRM CSC Path \n");
    }
  else
    {
      color_manager->use_gl_shaders = TRUE;
      meta_verbose("Using GL Shaders CSC Path \n");
    }
  return;
}

static void
meta_color_manager_finalize (GObject *object)
{
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
