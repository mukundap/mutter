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
 *   Naveen Kumar <naveen1.kumar@intel.com>
 *   Uday Kiran Pichika <pichika.uday.kiran@intel.com>
 *
 */

#include "config.h"

#include <drm_fourcc.h>
#include "backends/meta-backend-private.h"
#include "meta/meta-backend.h"
#include "wayland/meta-wayland-buffer.h"
#include "wayland/meta-wayland-private.h"
#include "wayland/meta-wayland-versions.h"
#include "wayland/meta-wayland-surface.h"
#include "wayland/meta-wayland-color-management.h"

#include "color-management-unstable-v1-server-protocol.h"

static void
meta_wayland_surface_state_set_color_space (
               MetaWaylandSurfaceState *pending_state, uint32_t color_space)
{
  if (pending_state->color_space == color_space)
    return;

  pending_state->color_space = color_space;
}

static uint32_t
meta_wayland_color_space_create (uint32_t eotf, uint32_t chromaticity,
                                 uint32_t whitepoint)
{
  static uint32_t color_space_names[] = {
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_UNKNOWN] = META_CS_UNKNOWN,
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_BT601_525_LINE] = META_CS_BT601_525_LINE,
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_BT601_625_LINE] = META_CS_BT601_625_LINE,
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_SMPTE170M] = META_CS_SMPTE170M,
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_BT709] = META_CS_BT709,
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_BT2020] = META_CS_BT2020,
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_SRGB] = META_CS_SRGB,
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_DISPLAYP3] = META_CS_DISPLAYP3,
    [ZWP_COLOR_MANAGER_V1_CHROMATICITY_NAMES_ADOBERGB] = META_CS_ADOBERGB,
  };

  return color_space_names[chromaticity];
}

static void
meta_wayland_color_space_send_names (struct wl_resource *resource,
              uint32_t eotf, uint32_t chromaticity, uint32_t whitepoint)
{
  zwp_color_space_v1_send_names(resource, eotf, chromaticity, whitepoint);
}

static void
destroy_color_space (struct wl_resource *resource)
{
  uint32_t color_space = wl_resource_get_user_data(resource);

  color_space = META_CS_UNKNOWN;
}

static void
color_space_destroy (struct wl_client *client,
                     struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}

struct zwp_color_space_v1_interface
color_space_implementation = {
  color_space_destroy
};

static void
color_management_output_get_color_space (struct wl_client *client,
                                         struct wl_resource *cm_resource,
                                         uint32_t id)
{
  MetaWaylandSurface *surface = wl_resource_get_user_data (cm_resource);
  uint32_t color_space;
  struct wl_resource *resource;

  /* if color management is enabled every surface is expected to carry
   a color space */
  color_space = surface->color_space;

  resource = wl_resource_create (client,
                             &zwp_color_space_v1_interface,
                             wl_resource_get_version(cm_resource), id);

  wl_resource_set_implementation (resource,
                              &color_space_implementation,
                              color_space, destroy_color_space);
  return;
}

static void
color_management_output_destroy (struct wl_client *client,
                                 struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}

struct zwp_color_management_output_v1_interface
color_management_output_implementation = {
  color_management_output_get_color_space,
  color_management_output_destroy
};

static void
color_management_surface_set_alpha_mode (struct wl_client *client,
					 struct wl_resource *resource,
					 uint32_t alpha_mode)
{
  MetaWaylandSurface *surface = wl_resource_get_user_data (resource);
  // TODO: later
}

static void
color_management_surface_set_extended_dynamic_range (struct wl_client *client,
					 struct wl_resource *resource,
					 uint32_t render_intent)
{
  MetaWaylandSurface *surface = wl_resource_get_user_data (resource);
  // TODO: later
}

static void
color_management_surface_set_color_space (struct wl_client *client,
                                          struct wl_resource *resource,
                                          struct wl_resource *cs_resource,
                                          uint32_t render_intent)
{
  MetaWaylandSurface *surface = wl_resource_get_user_data (resource);
  uint32_t client_color_space = wl_resource_get_user_data(cs_resource);

  meta_verbose ("%s:%s  color_space = %d \n", __FILE__, __func__,client_color_space);

  surface->color_space = client_color_space;
  meta_wayland_surface_state_set_color_space (&surface->pending_state,
                                             client_color_space);

  if(client_color_space != META_CS_UNKNOWN)
    {
      meta_color_manager_perform_csc(client_color_space);
    }
  // surface->pending.render_intent = render_intent;
}

static void
color_management_surface_destroy (struct wl_client *client,
                                  struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}

static void
destroy_color_management_surface (struct wl_resource *resource)
{
  wl_list_remove (wl_resource_get_link (resource));
}

static const struct zwp_color_management_surface_v1_interface
color_management_surface_implementation = {
  color_management_surface_set_alpha_mode,
  color_management_surface_set_extended_dynamic_range,
  color_management_surface_set_color_space,
  color_management_surface_destroy
};

static void
color_manager_create_color_space_from_icc (struct wl_client *client,
                                           struct wl_resource *resource,
                                           uint32_t id,
                                           int32_t icc)
{
  // TODO: later
}

static void
color_manager_create_color_space_from_names (struct wl_client *client,
                                             struct wl_resource *cm_resource,
                                             uint32_t id,
                                             uint32_t eotf,
                                             uint32_t chromaticity,
                                             uint32_t whitepoint)
{
  uint32_t color_space;
  struct wl_resource *resource;

  color_space = meta_wayland_color_space_create (eotf, chromaticity, whitepoint);

  resource = wl_resource_create (client,
                       &zwp_color_space_v1_interface,
                       wl_resource_get_version (cm_resource), id);
  wl_resource_set_implementation (resource,
                              &color_space_implementation,
                              color_space, destroy_color_space);

  meta_wayland_color_space_send_names(resource,
                 eotf,
                 chromaticity,
                 whitepoint);
}

static void
color_manager_create_color_space_from_params (struct wl_client *client,
                struct wl_resource *resource, uint32_t id, uint32_t eotf,
                uint32_t primary_r_x, uint32_t primary_r_y,
                uint32_t primary_g_x, uint32_t primary_g_y,
                uint32_t primary_b_x, uint32_t primary_b_y,
                uint32_t white_point_x, uint32_t white_point_y)
{
  // TODO: later
}

static void
color_manager_get_color_management_output (struct wl_client *client,
                                           struct wl_resource *cm_resource,
                                           uint32_t id,
                                           struct wl_resource *output_resource)
{
  struct wl_resource *resource;
  MetaWaylandSurface *surface = wl_resource_get_user_data (output_resource);
  resource = wl_resource_create (client,
                 &zwp_color_management_output_v1_interface,
                 wl_resource_get_version (cm_resource), id);
  wl_resource_set_implementation (resource,
                 &color_management_output_implementation,
                 surface,
                 destroy_color_management_surface);
  // TODO: later
}

static void
color_manager_get_color_management_surface (struct wl_client   *client,
                                     struct wl_resource *cm_resource,
                                     uint32_t id,
                                     struct wl_resource *surface_resource)
{
  MetaWaylandSurface *surface = wl_resource_get_user_data (surface_resource);
  struct wl_resource *resource;

  resource = wl_resource_create (client,
                                 &zwp_color_management_surface_v1_interface,
                                 wl_resource_get_version(cm_resource), id);
  wl_resource_set_implementation (resource,
                                  &color_management_surface_implementation,
                                  surface,
                                  destroy_color_management_surface);
  wl_list_insert (&surface->cm_surface_resources,
                    wl_resource_get_link(resource));
  return;
}

static void
color_manager_destroy (struct wl_client   *client,
                       struct wl_resource *resource)
{
  wl_resource_destroy (resource);
}

static const struct zwp_color_manager_v1_interface
color_manager_implementation =
{
  color_manager_create_color_space_from_icc,
  color_manager_create_color_space_from_names,
  color_manager_create_color_space_from_params,
  color_manager_get_color_management_output,
  color_manager_get_color_management_surface,
  color_manager_destroy
};

static void
color_manager_bind (struct wl_client *client,
                    void             *data,
                    uint32_t          version,
                    uint32_t          id)
{
  MetaWaylandCompositor *compositor = data;
  struct wl_resource *resource;

  resource = wl_resource_create (client, &zwp_color_manager_v1_interface,
                                 version, id);
  wl_resource_set_implementation (resource, &color_manager_implementation,
                                  compositor, NULL);
}

/**
 * meta_wayland_color_management_init:
 * @compositor: The #MetaWaylandCompositor
 *
 * Creates the global Wayland object that exposes the color-management protocol.
 *
 * Returns: Whether the initialization was succesfull. If this is %FALSE,
 * clients won't be able to use the color-management protocol for color space conversion.
 */
gboolean
meta_wayland_color_management_init (MetaWaylandCompositor *compositor)
{
  if (!wl_global_create (compositor->wayland_display,
                         &zwp_color_manager_v1_interface,
                         META_ZWP_COLOR_MANAGER_V1_VERSION,
                         compositor,
                         color_manager_bind))
    return FALSE;

  return TRUE;
}
