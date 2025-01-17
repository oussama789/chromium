// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_SYNC_UTILS_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_SYNC_UTILS_H_

namespace autofill {

enum AutofillSyncSigninState {
  // The user is not signed in to Chromium.
  kSignedOut,
  // The user is signed in to Chromium.
  kSignedIn,
  // The user is signed in to Chromium and sync transport is active for Wallet
  // data.
  kSignedInAndWalletSyncTransportEnabled,
  // The user is signed in, has enabled the sync feature and has not disabled
  // Wallet sync.
  kSignedInAndSyncFeature,
  kNumSyncStates,
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_SYNC_UTILS_H_
