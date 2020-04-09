/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_PARAMETERS_H_
#define BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_PARAMETERS_H_

#include <string>
#include <stdint.h>
#include <map>

#include "base/optional.h"
#include "base/files/file_path.h"

namespace brave_usermodel_parameters {

struct ParametersInfo {
  std::string model_id;
  uint16_t version;
  base::FilePath parameter_file;
};

class Parameters {
 public:
  Parameters();
  ~Parameters();

  uint16_t GetSchemaVersion();
  void AddSchemaVersion(uint16_t);
  ParametersInfo GetParametersInfo(
      const std::string& model_id);
  bool AddParametersInfo(
      const ParametersInfo& info);
  bool AddOrUpdateParametersInfo(
      const ParametersInfo& info);

 private:
  uint16_t schema_version_;
  std::map<std::string, ParametersInfo> parameters_;
};

}  // namespace brave_usermodel_parameters

#endif  // BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_PARAMETERS_H_
