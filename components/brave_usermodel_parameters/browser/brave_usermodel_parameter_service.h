/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_BRAVE_USERMODEL_PARAMETER_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_BRAVE_USERMODEL_PARAMETER_SERVICE_H_

#include <memory>
#include <string>
#include <map>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_component_updater/browser/brave_component.h"
#include "brave/components/brave_usermodel_parameters/browser/parameters.h"

using brave_component_updater::BraveComponent;

namespace brave_usermodel_parameters {

class UsermodelParameterService : public BraveComponent {
 public:
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnUserModelUpdated(
        const std::string& model_id,
        const base::FilePath& model_path) = 0;

   protected:
    ~Observer() override = default;
  };

  explicit UsermodelParameterService(Delegate* delegate);
  ~UsermodelParameterService() override;

  UsermodelParameterService(const UsermodelParameterService&) = delete;
  UsermodelParameterService& operator=(const UsermodelParameterService&) =
      delete;

  void AddObserver(
      Observer* observer);
  void RemoveObserver(
      Observer* observer);
  void NotifyObservers(
      const std::string& model_id,
      const base::FilePath& model_path);
  base::FilePath GetModelPath(
      const std::string& model_id);
  ParametersInfo GetParametersFilePathForModel(
      const std::string& model_id);

 private:
  void OnComponentReady(
      const std::string& component_id,
      const base::FilePath& install_dir,
      const std::string& manifest) override;
  void OnGetManifest(
      const base::FilePath& install_dir,
      const std::string& manifest_json);

  base::ObserverList<Observer> observers_;
  std::unique_ptr<Parameters> parameters_;
  base::WeakPtrFactory<UsermodelParameterService> weak_factory_{this};
};

}  // namespace brave_usermodel_parameters

#endif  // BRAVE_COMPONENTS_BRAVE_USERMODEL_PARAMETERS_BRAVE_USERMODEL_PARAMETER_SERVICE_H_
