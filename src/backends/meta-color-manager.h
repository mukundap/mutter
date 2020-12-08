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

#ifndef META_COLOR_MANAGER_H
#define META_COLOR_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>
#include <meta/types.h>

#define META_TYPE_COLOR_MANAGER (meta_color_manager_get_type ())

G_DECLARE_FINAL_TYPE (MetaColorManager, meta_color_manager,
                      META, COLOR_MANAGER, GObject)

typedef enum _MetaColorTransformPath
{
  META_COLOR_TRANSFORM_DISPLAY,
  META_COLOR_TRANSFORM_GLSL,
  META_COLOR_TRANSFORM_MEDIA,
} MetaColorTransformPath;

MetaColorManager * meta_color_manager_new (MetaBackend *backend);

uint16_t meta_color_manager_get_target_colorspace(MetaBackend *backend);

void meta_color_manager_perform_csc(uint32_t client_color_space);

#endif /* META_COLOR_MANAGER_H */
