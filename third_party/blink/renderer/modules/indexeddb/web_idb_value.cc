// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/indexeddb/web_idb_value.h"

#include "third_party/blink/public/platform/web_blob_info.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_vector.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_key.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_key_path.h"
#include "third_party/blink/renderer/modules/indexeddb/idb_value.h"
#include "third_party/blink/renderer/modules/indexeddb/web_idb_key_path.h"

namespace blink {

WebIDBValue::WebIDBValue(const WebData& data,
                         const WebVector<WebBlobInfo>& blob_info)
    : private_(IDBValue::Create(data, blob_info)) {
#if DCHECK_IS_ON()
  private_->SetIsOwnedByWebIDBValue(true);
#endif  // DCHECK_IS_ON()
}

WebIDBValue::WebIDBValue(WebIDBValue&&) noexcept = default;
WebIDBValue& WebIDBValue::operator=(WebIDBValue&&) noexcept = default;

WebIDBValue::~WebIDBValue() noexcept = default;

void WebIDBValue::SetInjectedPrimaryKey(std::unique_ptr<IDBKey> primary_key,
                                        const WebIDBKeyPath& primary_key_path) {
  private_->SetInjectedPrimaryKey(std::move(primary_key),
                                  IDBKeyPath(primary_key_path));
}

WebVector<WebBlobInfo> WebIDBValue::BlobInfoForTesting() const {
  return private_->BlobInfo();
}

std::unique_ptr<IDBValue> WebIDBValue::ReleaseIdbValue() noexcept {
#if DCHECK_IS_ON()
  ReleaseIdbValueOwnership();
#endif  // DCHECK_IS_ON()
  return std::move(private_);
}

#if DCHECK_IS_ON()

void WebIDBValue::ReleaseIdbValueOwnership() {
  private_->SetIsOwnedByWebIDBValue(false);
}

#endif  // DCHECK_IS_ON()

}  // namespace blink
