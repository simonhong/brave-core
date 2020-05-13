/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_settings_agent_impl_helper.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"

#define BRAVE_IS_POINT_IN_PATH                                 \
  LocalDOMWindow* window = LocalDOMWindow::From(script_state); \
  if (window && !AllowFingerprinting(window->GetFrame())) {    \
    return false;                                              \
  }

#define BRAVE_IS_POINT_IN_STROKE BRAVE_IS_POINT_IN_PATH

#define BRAVE_GET_IMAGE_DATA                                         \
  LocalDOMWindow* window = LocalDOMWindow::From(script_state);       \
  if (window) {                                                      \
    snapshot = brave::BraveSessionCache::From(*(window->document())) \
      .PerturbPixels(window->document()->GetFrame(), snapshot);      \
  }

#include "../../../../../../../third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.cc"

#undef BRAVE_GET_IMAGE_DATA
