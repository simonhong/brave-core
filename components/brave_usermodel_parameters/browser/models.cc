/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_usermodel_parameters/browser/models.h"

namespace brave_usermodel_parameters {

base::Optional<ModelInfo> Models::GetModelInfo(
    const std::string& id) {
  for (const auto& info : _models) {
    if (info.id == id)
      return info;
  }

  return base::nullopt;
}

}  // namespace brave_usermodel_parameters
