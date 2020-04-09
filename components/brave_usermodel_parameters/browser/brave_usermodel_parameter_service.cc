/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_usermodel_parameters/browser/brave_usermodel_parameter_service.h"

#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>  // TODO(Moritz Haller): remove

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"

#include "brave/components/brave_ads/browser/locale_helper.h"
#include "brave/components/brave_usermodel_parameters/browser/regions.h"

// TOOD(Moritz Haller): Rename to |Brave Usermodel Updater|
namespace brave_usermodel_parameters {
constexpr char kSchemaVersionPath[] = "schemaVersion";
constexpr char kModelsPath[] = "models";
constexpr char kModelsIdPath[] = "id";
constexpr char kModelsFilenamePath[] = "filename";
constexpr char kModelsVersionPath[] = "version";
constexpr char kComponentName[] = "Brave Usermodel Updater (%s)";
constexpr base::FilePath::CharType kManifestFile[] =
    FILE_PATH_LITERAL("models.json");

UsermodelParameterService::UsermodelParameterService(
    Delegate* delegate)
    : brave_component_updater::BraveComponent(delegate),
      parameters_(new Parameters()) {
  // TODO(Moritz Haller): Remove and test w/ mock/fake object?
  if (!delegate)
    return;

  const std::string locale =
      brave_ads::LocaleHelper::GetInstance()->GetLocale();
  const std::string country_code =
      brave_ads::LocaleHelper::GetCountryCode(locale);
  const auto& info = Regions::GetRegionInfo(country_code);

  if (!info) {
    // TODO(Moritz Haller): Right log channel/level?
    DVLOG(2) << "No client parameter component for " << country_code;
    return;
  }

  // TODO(Moritz Haller): only register when rewards enabled
  Register(
      base::StringPrintf(kComponentName, country_code.c_str()),
      info->component_id,
      info->component_base64_public_key);
}

// TODO(Moritz Haller): will parameters_ object be destroyed?
UsermodelParameterService::~UsermodelParameterService() = default;

std::string GetManifest(
    const base::FilePath& manifest_path) {
  std::string manifest_json;
  bool success = base::ReadFileToString(manifest_path, &manifest_json);
  if (!success || manifest_json.empty()) {
    DVLOG(2) << __func__ << ": cannot read manifest file " << manifest_path;
    return manifest_json;
  }

  return manifest_json;
}

void UsermodelParameterService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  base::PostTaskAndReplyWithResult(
      FROM_HERE, {base::ThreadPool(), base::MayBlock()},
      base::BindOnce(&GetManifest, install_dir.Append(kManifestFile)),
      base::BindOnce(&UsermodelParameterService::OnGetManifest,
          weak_factory_.GetWeakPtr(), install_dir));

  std::cout << "*** DEBUG 1: OnComponentReady\n";
}

void UsermodelParameterService::OnGetManifest(
    const base::FilePath& install_dir,
    const std::string& manifest_json) {
  // TODO(Moritz Haller): On every scheduled event reset parameteres! Start
  // caching items or investigate how it relates to delta updates
  // parameters_.reset(new Parameters());

  // TODO(Moritz Haller): Maybe run outside origin sequence/main thread?
  base::Optional<base::Value> manifest = base::JSONReader::Read(manifest_json);
  if (!manifest) {
    DVLOG(2) << "Reading manifest json failed";
    return;
  }

  // TODO(Moritz Haller): Add schema version check
  if (base::Optional<int> version = manifest->FindIntPath(kSchemaVersionPath)) {
    parameters_->AddSchemaVersion(*version);
  }

  if (auto* models = manifest->FindListPath(kModelsPath)) {
    for (const auto& model : models->GetList()) {
      ParametersInfo incoming_info;
      if (auto* maybe_id = model.FindStringPath(kModelsIdPath)) {
        incoming_info.model_id = *maybe_id;
      }

      if (base::Optional<int> maybe_version =
          model.FindIntPath(kModelsVersionPath)) {
        incoming_info.version = *maybe_version;
      }

      if (auto* maybe_path = model.FindStringPath(kModelsFilenamePath)) {
        incoming_info.parameter_file = install_dir.AppendASCII(*maybe_path);
        std::cout << "*** DEBUG 1 " << incoming_info.parameter_file << "\n";
      }

      // TODO(Moritz Haller): results in SIG6 abbort
      // ParametersInfo current_info = parameters_->GetParametersInfo(
      //     incoming_info.model_id);
      // TODO(Moritz Haller): Is readable? current.version inits with 0 if
      // model doesn't exist, hence conditional also serves as "existence check"
      // if (current_info.version > incoming_info.version) {
      //   continue;
      // }

      // parameters_->AddOrUpdateParametersInfo(incoming_info);
      NotifyObservers(incoming_info.model_id, incoming_info.parameter_file);
    }
  }
}

void UsermodelParameterService::NotifyObservers(
    const std::string& model_id,
    const base::FilePath& model_path) {
  for (Observer& observer : observers_) {
    observer.OnUserModelUpdated(model_id, model_path);
    std::cout << "*** DEBUG 2: Loaded and parsed manifest -> Notify observer\n";
  }
}

void UsermodelParameterService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void UsermodelParameterService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

base::FilePath UsermodelParameterService::GetModelPath(
    const std::string& model_id) {
  ParametersInfo info = parameters_->GetParametersInfo(model_id);
  return info.parameter_file;
}

ParametersInfo UsermodelParameterService::GetParametersFilePathForModel(
    const std::string& model_id) {
  ParametersInfo info;

  if (model_id.empty()) {
    return info;
  }

  return parameters_->GetParametersInfo(model_id);
}

}  // namespace brave_usermodel_parameters
