// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/indexeddb/indexed_db_dispatcher.h"

#include <utility>

#include "third_party/blink/renderer/modules/indexeddb/idb_key_range.h"
#include "third_party/blink/renderer/modules/indexeddb/web_idb_cursor_impl.h"
#include "third_party/blink/renderer/modules/indexeddb/web_idb_database_callbacks.h"
#include "third_party/blink/renderer/modules/indexeddb/web_idb_observation.h"

namespace blink {
// static
IndexedDBDispatcher* IndexedDBDispatcher::GetInstanceForCurrentThread() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(ThreadSpecific<IndexedDBDispatcher>,
                                  thread_specific_instance, ());
  return thread_specific_instance;
}

IndexedDBDispatcher::IndexedDBDispatcher() = default;

// static
void IndexedDBDispatcher::RegisterCursor(WebIDBCursorImpl* cursor) {
  IndexedDBDispatcher* this_ptr = GetInstanceForCurrentThread();
  DCHECK(!this_ptr->cursors_.Contains(cursor));
  this_ptr->cursors_.insert(cursor);
}

// static
void IndexedDBDispatcher::UnregisterCursor(WebIDBCursorImpl* cursor) {
  IndexedDBDispatcher* this_ptr = GetInstanceForCurrentThread();
  DCHECK(this_ptr->cursors_.Contains(cursor));
  this_ptr->cursors_.erase(cursor);
}

// static
void IndexedDBDispatcher::ResetCursorPrefetchCaches(
    int64_t transaction_id,
    WebIDBCursorImpl* except_cursor) {
  IndexedDBDispatcher* this_ptr = GetInstanceForCurrentThread();
  for (WebIDBCursorImpl* cursor : this_ptr->cursors_) {
    if (cursor != except_cursor && cursor->transaction_id() == transaction_id)
      cursor->ResetPrefetchCache();
  }
}

}  // namespace blink
