// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.media.router.caf;

import android.content.Intent;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.metrics.MediaNotificationUma;
import org.chromium.chrome.browser.tab.Tab;

/** NotificationController implementation for presentation. */
public class CafNotificationController extends BaseNotificationController {
    public CafNotificationController(BaseSessionController sessionController) {
        super(sessionController);
    }

    @Override
    public Intent createContentIntent() {
        Intent contentIntent =
                Tab.createBringTabToFrontIntent(mSessionController.getRouteCreationInfo().tabId);
        if (contentIntent != null) {
            contentIntent.putExtra(MediaNotificationUma.INTENT_EXTRA_NAME,
                    MediaNotificationUma.Source.PRESENTATION);
        }
        return contentIntent;
    }

    @Override
    public int getNotificationId() {
        return R.id.presentation_notification;
    }
}
