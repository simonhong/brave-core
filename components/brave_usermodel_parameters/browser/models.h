/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_MODELS_H_
#define BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_MODELS_H_

#include <string>
#include <vector>

#include "base/optional.h"

namespace brave_usermodel_parameters {

struct ModelInfo {
  std::string id;
  std::string name;
};

static const std::vector<ModelInfo> _models = {
    {"loighdbokjikidnmlfddgidbkhodedgm", "purchase-intent-classifier"},
    {"djcjjghgcjchoepkmgnlijcgbipfgoab", "contextual-page-classifier"},
};

class Models {
 public:
  Models();
  ~Models();

  static base::Optional<ModelInfo> GetModelInfo(
      const std::string& id);
};

}  // namespace brave_usermodel_parameters

#endif  // BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_MODELS_H_
