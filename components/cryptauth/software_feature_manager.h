// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_MANAGER_H_
#define COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_MANAGER_H_

#include "base/callback.h"
#include "chromeos/components/multidevice/software_feature.h"
#include "components/cryptauth/network_request_error.h"

namespace cryptauth {

// Queries for eligible MultiDevice hosts and sets/changes/unsets the current
// MultiDevice host for the logged-in account.
class SoftwareFeatureManager {
 public:
  virtual ~SoftwareFeatureManager() {}

  // Enables or disables |software_feature| for the device with public key
  // |public_key|. If |enabled| and |is_exclusive| are both true, then all other
  // devices associated with this account will have |sofware_feature| disabled.
  // |is_exclusive| is ignored if |enabled| is false.
  //
  // Note: In the special case of passing |software_feature| =
  // SoftwareFeature::EASY_UNLOCK_HOST and |enabled| = false, |public_key| is
  // ignored.
  virtual void SetSoftwareFeatureState(
      const std::string& public_key,
      chromeos::multidevice::SoftwareFeature software_feature,
      bool enabled,
      const base::Closure& success_callback,
      const base::Callback<void(NetworkRequestError)>& error_callback,
      bool is_exclusive = false) = 0;

  // Finds eligible devices associated with the logged-in account which support
  // |software_feature|.
  virtual void FindEligibleDevices(
      chromeos::multidevice::SoftwareFeature software_feature,
      const base::Callback<void(const std::vector<ExternalDeviceInfo>&,
                                const std::vector<IneligibleDevice>&)>&
          success_callback,
      const base::Callback<void(NetworkRequestError)>& error_callback) = 0;
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_SOFTWARE_FEATURE_MANAGER_H_
