/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */

/*
 * Copyright (C) 2017 Red Hat, Inc.
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "backends/meta-monitor-manager-private.h"
#include "backends/meta-crtc.h"
#include "backends/meta-output.h"
#include "core/display-private.h"
#include "meta-test/meta-context-test.h"
#include "tests/meta-monitor-manager-test.h"

static void
meta_test_headless_start (void)
{
  MetaBackend *backend = meta_get_backend ();
  MetaMonitorManager *monitor_manager =
    meta_backend_get_monitor_manager (backend);
  GList *gpus;
  MetaGpu *gpu;

  gpus = meta_backend_get_gpus (backend);
  g_assert_cmpint ((int) g_list_length (gpus), ==, 1);

  gpu = gpus->data;
  g_assert_null (meta_gpu_get_modes (gpu));
  g_assert_null (meta_gpu_get_outputs (gpu));
  g_assert_null (meta_gpu_get_crtcs (gpu));
  g_assert_null (monitor_manager->monitors);
  g_assert_null (monitor_manager->logical_monitors);

  g_assert_cmpint (monitor_manager->screen_width,
                   ==,
                   META_MONITOR_MANAGER_MIN_SCREEN_WIDTH);
  g_assert_cmpint (monitor_manager->screen_height,
                   ==,
                   META_MONITOR_MANAGER_MIN_SCREEN_HEIGHT);
}

static void
meta_test_headless_monitor_getters (void)
{
  MetaDisplay *display;
  int index;

  display = meta_get_display ();

  index = meta_display_get_monitor_index_for_rect (display,
                                                   &(MetaRectangle) { 0 });
  g_assert_cmpint (index, ==, -1);
}

static void
meta_test_headless_monitor_connect (void)
{
  MetaBackend *backend = meta_get_backend ();
  MetaMonitorManager *monitor_manager =
    meta_backend_get_monitor_manager (backend);
  MetaMonitorManagerTest *monitor_manager_test =
    META_MONITOR_MANAGER_TEST (monitor_manager);
  MetaMonitorTestSetup *test_setup;
  MetaCrtcMode **modes;
  g_autoptr (MetaCrtcModeInfo) crtc_mode_info = NULL;
  MetaCrtcMode *crtc_mode;
  MetaGpu *gpu;
  MetaCrtc *crtc;
  MetaCrtc **possible_crtcs;
  g_autoptr (MetaOutputInfo) output_info = NULL;
  MetaOutput *output;
  GList *logical_monitors;
  ClutterActor *stage;

  test_setup = g_new0 (MetaMonitorTestSetup, 1);

  crtc_mode_info = meta_crtc_mode_info_new ();
  crtc_mode_info->width = 1024;
  crtc_mode_info->height = 768;
  crtc_mode_info->refresh_rate = 60.0;

  crtc_mode = g_object_new (META_TYPE_CRTC_MODE,
                            "id", (uint64_t) 1,
                            "info", crtc_mode_info,
                            NULL);
  test_setup->modes = g_list_append (NULL, crtc_mode);

  gpu = META_GPU (meta_backend_get_gpus (meta_get_backend ())->data);
  crtc = g_object_new (META_TYPE_CRTC_TEST,
                       "id", (uint64_t) 1,
                       "gpu", gpu,
                       NULL);
  test_setup->crtcs = g_list_append (NULL, crtc);

  modes = g_new0 (MetaCrtcMode *, 1);
  modes[0] = crtc_mode;

  possible_crtcs = g_new0 (MetaCrtc *, 1);
  possible_crtcs[0] = g_list_first (test_setup->crtcs)->data;

  output_info = meta_output_info_new ();

  output_info->name = g_strdup ("DP-1");
  output_info->vendor = g_strdup ("MetaProduct's Inc.");
  output_info->product = g_strdup ("MetaMonitor");
  output_info->serial = g_strdup ("0x987654");
  output_info->preferred_mode = modes[0];
  output_info->n_modes = 1;
  output_info->modes = modes;
  output_info->n_possible_crtcs = 1;
  output_info->possible_crtcs = possible_crtcs;
  output_info->connector_type = META_CONNECTOR_TYPE_DisplayPort;

  output = g_object_new (META_TYPE_OUTPUT_TEST,
                         "id", (uint64_t) 1,
                         "gpu", gpu,
                         "info", output_info,
                         NULL);

  test_setup->outputs = g_list_append (NULL, output);

  meta_monitor_manager_test_emulate_hotplug (monitor_manager_test, test_setup);

  logical_monitors =
    meta_monitor_manager_get_logical_monitors (monitor_manager);
  g_assert_cmpint (g_list_length (logical_monitors), ==, 1);

  g_assert_cmpint (monitor_manager->screen_width, ==, 1024);
  g_assert_cmpint (monitor_manager->screen_height, ==, 768);

  stage = meta_backend_get_stage (backend);
  g_assert_cmpint (clutter_actor_get_width (stage), ==, 1024);
  g_assert_cmpint (clutter_actor_get_height (stage), ==, 768);
}

static MetaMonitorTestSetup *
create_headless_test_setup (MetaBackend *backend)
{
  return g_new0 (MetaMonitorTestSetup, 1);
}

static void
init_tests (void)
{
  meta_init_monitor_test_setup (create_headless_test_setup);

  g_test_add_func ("/headless-start/start", meta_test_headless_start);
  g_test_add_func ("/headless-start/monitor-getters",
                   meta_test_headless_monitor_getters);
  g_test_add_func ("/headless-start/connect",
                   meta_test_headless_monitor_connect);
}

int
main (int argc, char *argv[])
{
  g_autoptr (MetaContext) context = NULL;

  context = meta_create_test_context (META_CONTEXT_TEST_TYPE_NESTED,
                                      META_CONTEXT_TEST_FLAG_NO_X11);
  g_assert (meta_context_configure (context, &argc, &argv, NULL));

  init_tests ();

  return meta_context_test_run_tests (META_CONTEXT_TEST (context),
                                      META_TEST_RUN_FLAG_NONE);
}
