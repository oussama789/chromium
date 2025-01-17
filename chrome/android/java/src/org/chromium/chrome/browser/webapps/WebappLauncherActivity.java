// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.webapps;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.StrictMode;
import android.os.SystemClock;
import android.support.annotation.NonNull;
import android.text.TextUtils;
import android.util.Base64;

import org.chromium.base.ApiCompatibilityUtils;
import org.chromium.base.ApplicationStatus;
import org.chromium.base.ContextUtils;
import org.chromium.base.Log;
import org.chromium.chrome.browser.IntentHandler;
import org.chromium.chrome.browser.ShortcutHelper;
import org.chromium.chrome.browser.ShortcutSource;
import org.chromium.chrome.browser.document.ChromeLauncherActivity;
import org.chromium.chrome.browser.metrics.LaunchMetrics;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.util.IntentUtils;
import org.chromium.webapk.lib.client.WebApkValidator;
import org.chromium.webapk.lib.common.WebApkConstants;

import java.lang.ref.WeakReference;

/**
 * Launches web apps.  This was separated from the ChromeLauncherActivity because the
 * ChromeLauncherActivity is not allowed to be excluded from Android's Recents: crbug.com/517426.
 */
public class WebappLauncherActivity extends Activity {
    /**
     * Action fired when an Intent is trying to launch a WebappActivity.
     * Never change the package name or the Intents will fail to launch.
     */
    public static final String ACTION_START_WEBAPP =
            "com.google.android.apps.chrome.webapps.WebappManager.ACTION_START_WEBAPP";

    /**
     * Delay in ms for relaunching WebAPK as a result of getting intent with extra
     * {@link WebApkConstants.EXTRA_RELAUNCH}. The delay was chosen arbirtarily and seems to
     * work.
     */
    private static final int WEBAPK_LAUNCH_DELAY_MS = 20;

    private static final String TAG = "webapps";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        long createTimestamp = SystemClock.elapsedRealtime();

        ChromeWebApkHost.init();
        Intent intent = getIntent();
        WebappInfo webappInfo = tryCreateWebappInfo(intent);

        if (shouldRelaunchWebApk(intent, webappInfo)) {
            relaunchWebApk(this, intent, webappInfo);
            return;
        }

        if (shouldLaunchWebapp(intent, webappInfo)) {
            launchWebapp(this, intent, webappInfo, createTimestamp);
            return;
        }

        launchInTab(this, intent, webappInfo);
    }

    private static boolean shouldLaunchWebapp(Intent intent, WebappInfo webappInfo) {
        // {@link WebApkInfo#create()} and {@link WebappInfo#create()} return null if the intent
        // does not specify required values such as the uri.
        if (webappInfo == null) return false;

        String webappUrl = webappInfo.uri().toString();
        String webappMac = IntentUtils.safeGetStringExtra(intent, ShortcutHelper.EXTRA_MAC);

        return (webappInfo.isForWebApk() || isValidMacForUrl(webappUrl, webappMac)
                || wasIntentFromChrome(intent));
    }

    private static void launchWebapp(Activity launchingActivity, Intent intent,
            @NonNull WebappInfo webappInfo, long createTimestamp) {
        String webappUrl = webappInfo.uri().toString();
        int webappSource = webappInfo.source();

        // Retrieves the source of the WebAPK from WebappDataStorage if it is unknown. The
        // {@link webappSource} is only known in the cases of an external intent or a
        // notification that launches a WebAPK. Otherwise, it's not trustworthy and we must read
        // the SharedPreference to get the installation source.
        if (webappInfo.isForWebApk() && webappSource == ShortcutSource.UNKNOWN) {
            webappSource = getWebApkSource(webappInfo);
        }
        LaunchMetrics.recordHomeScreenLaunchIntoStandaloneActivity(
                webappUrl, webappSource, webappInfo.displayMode());

        // Add all information needed to launch WebappActivity without {@link
        // WebappActivity#sWebappInfoMap} to launch intent. When the Android OS has killed a
        // WebappActivity and the user selects the WebappActivity from "Android Recents" the
        // WebappActivity is launched without going through WebappLauncherActivity first.
        WebappActivity.addWebappInfo(webappInfo.id(), webappInfo);
        Intent launchIntent = createWebappLaunchIntent(webappInfo);
        IntentHandler.addTimestampToIntent(launchIntent, createTimestamp);
        // Pass through WebAPK shell launch timestamp to the new intent.
        long shellLaunchTimestamp = IntentHandler.getWebApkShellLaunchTimestampFromIntent(intent);
        IntentHandler.addShellLaunchTimestampToIntent(launchIntent, shellLaunchTimestamp);

        IntentUtils.safeStartActivity(launchingActivity, launchIntent);
        if (IntentUtils.isIntentForNewTaskOrNewDocument(launchIntent)) {
            ApiCompatibilityUtils.finishAndRemoveTask(launchingActivity);
        } else {
            launchingActivity.finish();
        }
    }

    // Gets the source of a WebAPK from the WebappDataStorage if the source has been stored before.
    private static int getWebApkSource(WebappInfo webappInfo) {
        WebappDataStorage storage = null;

        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskReads();
        try {
            WebappRegistry.warmUpSharedPrefsForId(webappInfo.id());
            storage = WebappRegistry.getInstance().getWebappDataStorage(webappInfo.id());
        } finally {
            StrictMode.setThreadPolicy(oldPolicy);
        }

        if (storage != null) {
            int source = storage.getSource();
            if (source != ShortcutSource.UNKNOWN) {
                return source;
            }
        }
        return ShortcutSource.WEBAPK_UNKNOWN;
    }

    /**
     * Returns whether {@link sourceIntent} was sent by a WebAPK to relaunch itself.
     *
     * A WebAPK sends an intent to Chrome to get relaunched when it knows it is about to get killed
     * as result of a call to PackageManager#setComponentEnabledSetting().
     */
    private static boolean shouldRelaunchWebApk(Intent sourceIntent, WebappInfo webappInfo) {
        return webappInfo != null && webappInfo.isForWebApk()
                && sourceIntent.hasExtra(WebApkConstants.EXTRA_RELAUNCH);
    }

    /** Relaunches WebAPK. */
    private static void relaunchWebApk(
            Activity launchingActivity, Intent sourceIntent, @NonNull WebappInfo info) {
        Intent launchIntent = new Intent(Intent.ACTION_VIEW, info.uri());
        launchIntent.setPackage(info.webApkPackageName());
        launchIntent.setFlags(
                Intent.FLAG_ACTIVITY_NEW_TASK | ApiCompatibilityUtils.getActivityNewDocumentFlag());
        Bundle extras = sourceIntent.getExtras();
        if (extras != null) {
            launchIntent.putExtras(extras);
        }

        launchAfterDelay(
                launchingActivity.getApplicationContext(), launchIntent, WEBAPK_LAUNCH_DELAY_MS);
        ApiCompatibilityUtils.finishAndRemoveTask(launchingActivity);
    }

    /** Extracts start URL from source intent and launches URL in Chrome tab. */
    private static void launchInTab(
            Activity launchingActivity, Intent sourceIntent, WebappInfo webappInfo) {
        Context appContext = ContextUtils.getApplicationContext();
        String webappUrl = IntentUtils.safeGetStringExtra(sourceIntent, ShortcutHelper.EXTRA_URL);
        int webappSource = (webappInfo == null) ? ShortcutSource.UNKNOWN : webappInfo.source();

        if (TextUtils.isEmpty(webappUrl)) return;

        Intent launchIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(webappUrl));
        launchIntent.setClassName(
                appContext.getPackageName(), ChromeLauncherActivity.class.getName());
        launchIntent.putExtra(ShortcutHelper.REUSE_URL_MATCHING_TAB_ELSE_NEW_TAB, true);
        launchIntent.putExtra(ShortcutHelper.EXTRA_SOURCE, webappSource);
        launchIntent.setFlags(
                Intent.FLAG_ACTIVITY_NEW_TASK | ApiCompatibilityUtils.getActivityNewDocumentFlag());

        Log.e(TAG, "Shortcut (%s) opened in Chrome.", webappUrl);

        IntentUtils.safeStartActivity(appContext, launchIntent);
        ApiCompatibilityUtils.finishAndRemoveTask(launchingActivity);
    }

    /**
     * Checks whether or not the MAC is present and valid for the web app shortcut.
     *
     * The MAC is used to prevent malicious apps from launching Chrome into a full screen
     * Activity for phishing attacks (among other reasons).
     *
     * @param url The URL for the web app.
     * @param mac MAC to compare the URL against.  See {@link WebappAuthenticator}.
     * @return Whether the MAC is valid for the URL.
     */
    private static boolean isValidMacForUrl(String url, String mac) {
        return mac != null
                && WebappAuthenticator.isUrlValid(url, Base64.decode(mac, Base64.DEFAULT));
    }

    private static boolean wasIntentFromChrome(Intent intent) {
        return IntentHandler.wasIntentSenderChrome(intent);
    }

    /**
     * Creates an Intent to launch the web app.
     * @param info     Information about the web app.
     */
    private static Intent createWebappLaunchIntent(WebappInfo info) {
        String activityName = info.isForWebApk() ? WebApkActivity.class.getName()
                                                 : WebappActivity.class.getName();
        boolean newTask = true;
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
            // Specifically assign the app to a particular WebappActivity instance.
            int namespace = info.isForWebApk()
                    ? ActivityAssigner.ActivityAssignerNamespace.WEBAPK_NAMESPACE
                    : ActivityAssigner.ActivityAssignerNamespace.WEBAPP_NAMESPACE;
            int activityIndex = ActivityAssigner.instance(namespace).assign(info.id());
            activityName += String.valueOf(activityIndex);

            // Finishes the old activity if it has been assigned to a different WebappActivity. See
            // crbug.com/702998.
            for (WeakReference<Activity> activityRef : ApplicationStatus.getRunningActivities()) {
                Activity activity = activityRef.get();
                if (!(activity instanceof WebappActivity)
                        || !activity.getClass().getName().equals(activityName)) {
                    continue;
                }
                WebappActivity webappActivity = (WebappActivity) activity;
                if (!TextUtils.equals(webappActivity.getWebappInfo().id(), info.id())) {
                    activity.finish();
                }
                break;
            }
        } else {
            if (info.isForWebApk() && info.useTransparentSplash()) {
                activityName = TransparentSplashWebApkActivity.class.getName();
                newTask = false;
            }
        }

        // Create an intent to launch the Webapp in an unmapped WebappActivity.
        Intent launchIntent = new Intent();
        launchIntent.setClassName(ContextUtils.getApplicationContext(), activityName);
        info.setWebappIntentExtras(launchIntent);

        // On L+, firing intents with the exact same data should relaunch a particular
        // Activity.
        launchIntent.setAction(Intent.ACTION_VIEW);
        launchIntent.setData(Uri.parse(WebappActivity.WEBAPP_SCHEME + "://" + info.id()));

        // Setting FLAG_ACTIVITY_CLEAR_TOP handles 2 edge cases:
        // - If a legacy PWA is launching from a notification, we want to ensure that the URL being
        // launched is the URL in the intent. If a paused WebappActivity exists for this id,
        // then by default it will be focused and we have no way of sending the desired URL to
        // it (the intent is swallowed). As a workaround, set the CLEAR_TOP flag to ensure that
        // the existing Activity handles an update via onNewIntent().
        // - If a WebAPK is having a CustomTabActivity on top of it in the same Task, and user
        // clicks a link to takes them back to the scope of a WebAPK, we want to destroy the
        // CustomTabActivity activity and go back to the WebAPK activity. It is intentional that
        // Custom Tab will not be reachable with a back button.

        // In addition FLAG_ACTIVITY_NEW_DOCUMENT is required otherwise on Samsung Lollipop devices
        // an Intent to an existing top Activity (such as sent from the Webapp Actions Notification)
        // will trigger a new WebappActivity to be launched and onCreate called instead of
        // onNewIntent of the existing WebappActivity being called.
        // TODO(pkotwicz): Route Webapp Actions Notification actions through new intent filter
        //                 instead of WebappLauncherActivity. http://crbug.com/894610
        if (newTask) {
            launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                    | ApiCompatibilityUtils.getActivityNewDocumentFlag()
                    | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        } else {
            launchIntent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
        }
        return launchIntent;
    }

    /**
     * Brings a live WebappActivity back to the foreground if one exists for the given tab ID.
     * @param tabId ID of the Tab to bring back to the foreground.
     * @return True if a live WebappActivity was found, false otherwise.
     */
    public static boolean bringWebappToFront(int tabId) {
        if (tabId == Tab.INVALID_TAB_ID) return false;

        for (WeakReference<Activity> activityRef : ApplicationStatus.getRunningActivities()) {
            Activity activity = activityRef.get();
            if (activity == null || !(activity instanceof WebappActivity)) continue;

            WebappActivity webappActivity = (WebappActivity) activity;
            if (webappActivity.getActivityTab() != null
                    && webappActivity.getActivityTab().getId() == tabId) {
                Tab tab = webappActivity.getActivityTab();
                tab.getTabWebContentsDelegateAndroid().activateContents();
                return true;
            }
        }

        return false;
    }

    /** Tries to create WebappInfo/WebApkInfo for the intent. */
    private static WebappInfo tryCreateWebappInfo(Intent intent) {
        // Builds WebApkInfo for the intent if the WebAPK package specified in the intent is a valid
        // WebAPK and the URL specified in the intent can be fulfilled by the WebAPK.
        String webApkPackage =
                IntentUtils.safeGetStringExtra(intent, WebApkConstants.EXTRA_WEBAPK_PACKAGE_NAME);
        String url = IntentUtils.safeGetStringExtra(intent, ShortcutHelper.EXTRA_URL);
        if (!TextUtils.isEmpty(webApkPackage) && !TextUtils.isEmpty(url)
                && WebApkValidator.canWebApkHandleUrl(
                        ContextUtils.getApplicationContext(), webApkPackage, url)) {
            return WebApkInfo.create(intent);
        }

        Log.d(TAG, "%s is either not a WebAPK or %s is not within the WebAPK's scope",
                webApkPackage, url);
        return WebappInfo.create(intent);
    }

    /** Launches intent after a delay. */
    private static void launchAfterDelay(Context appContext, Intent intent, int launchDelayMs) {
        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                IntentUtils.safeStartActivity(appContext, intent);
            }
        }, launchDelayMs);
    }
}
