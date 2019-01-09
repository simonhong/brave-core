/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/local_data_files_service.h"

#include <algorithm>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/base_local_data_files_observer.h"

namespace brave_shields {

std::string LocalDataFilesService::g_local_data_files_component_id_(
  kLocalDataFilesComponentId);
std::string LocalDataFilesService::g_local_data_files_component_base64_public_key_(
  kLocalDataFilesComponentBase64PublicKey);

LocalDataFilesService::LocalDataFilesService()
  : initialized_(false),
    observers_already_called_(false),
    weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

LocalDataFilesService::~LocalDataFilesService() { }

bool LocalDataFilesService::Start() {
  if (initialized_)
    return true;
  Register(kLocalDataFilesComponentName,
           g_local_data_files_component_id_,
           g_local_data_files_component_base64_public_key_);
  initialized_ = true;
  return true;
}

void LocalDataFilesService::AddObserver(BaseLocalDataFilesObserver* observer) {
  if (observers_already_called_) {
    // OnComponentReady has already been called, so call back this observer
    // immediately
    observer->OnComponentReady(component_id_, install_dir_, manifest_);
  } else {
    // not ready to call this observer yet, so add it to the queue and we'll
    // call it later
    observers_.push_back(observer);
  }
}

void LocalDataFilesService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  component_id_ = component_id;
  install_dir_ = install_dir;
  manifest_ = manifest;
  for (BaseLocalDataFilesObserver* observer : observers_)
    observer->OnComponentReady(component_id_, install_dir_, manifest_);
  observers_already_called_ = true;
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<LocalDataFilesService> LocalDataFilesServiceFactory() {
  return std::make_unique<LocalDataFilesService>();
}

}  // namespace brave_shields
