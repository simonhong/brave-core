/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include <iostream>

#include "brave/components/brave_usermodel_parameters/browser/parameters.h"
#include "brave/components/brave_usermodel_parameters/browser/models.h"

namespace brave_usermodel_parameters {

Parameters::Parameters() = default;
Parameters::~Parameters() = default;

uint16_t Parameters::GetSchemaVersion() {
  return schema_version_;
}

void Parameters::AddSchemaVersion(uint16_t version) {
  schema_version_ = version;
}

// TODO(Moritz Haller): merge with |models| and unittest _ALLOFIT_!
ParametersInfo Parameters::GetParametersInfo(
    const std::string& model_id) {
  ParametersInfo info;
  if (auto maybe_model = Models::GetModelInfo(model_id)) {
    info = parameters_.at(model_id);
    return info;
  }

  return info;
}

bool Parameters::AddParametersInfo(
    const ParametersInfo& info) {
  // If model on whitelist
  // not v readable
  if (auto maybe_model = Models::GetModelInfo(info.model_id)) {
    auto rval = parameters_.insert({info.model_id, info});
    // Returns false if key exists already
    return rval.second;
  }

  // model wasn't on whitelist
  return false;
}

bool Parameters::AddOrUpdateParametersInfo(
    const ParametersInfo& incoming_info) {
  // std::map<std::string, ParametersInfo>::iterator
  // Update existing info
  auto it = parameters_.find(incoming_info.model_id);
  if (it != parameters_.end()) {
    it->second = incoming_info;
    return true;
  } else {
  // Add new info
    bool success = AddParametersInfo(incoming_info);
    return success;
  }

  return false;
}

}  // namespace brave_usermodel_parameters
