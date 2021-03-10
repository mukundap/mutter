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
 *   Naveen Kumar <naveen1.kumar@intel.com>
 *
 */

#ifndef META_COLOR_SPACE_H
#define META_COLOR_SPACE_H

#include <glib.h>
#include <glib-object.h>
#include <stdint.h>

#include "wayland/meta-wayland-types.h"

typedef struct _MetaVector
{
  uint16_t x;
  uint16_t y;
} MetaVector;

typedef struct _MetaColorPrimaries
{
  MetaVector r;
  MetaVector g;
  MetaVector b;
  MetaVector white_point;
} MetaColorPrimaries;

typedef struct _MetaColorSpace
{
  MetaColorPrimaries primaries;
  const char *name;
  const char *whitepoint_name;
} MetaColorSpace;

typedef enum _MetaColorSpaceEnums
{
  META_CS_UNKNOWN,
  META_CS_BT601_525_LINE,
  META_CS_BT601_625_LINE,
  META_CS_SMPTE170M,
  META_CS_BT709,
  META_CS_BT2020,
  META_CS_SRGB,
  META_CS_DISPLAYP3,
  META_CS_ADOBERGB
} MetaColorSpaceEnums;

const struct MetaColorSpace *
meta_color_space_lookup(MetaColorSpaceEnums color_space);


#endif /* META_COLOR_SPACE_H */
