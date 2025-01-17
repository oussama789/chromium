// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

class TestDownloadsProxy {
  constructor() {
    /** @type {mdDownloads.mojom.PageCallbackRouter} */
    this.callbackRouter = new mdDownloads.mojom.PageCallbackRouter();

    /** @type {!mdDownloads.mojom.PageInterface} */
    this.pageRouterProxy = this.callbackRouter.createProxy();

    /** @type {mdDownloads.mojom.PageHandlerInterface} */
    this.handler = new TestDownloadsMojoHandler(this.pageRouterProxy);
  }
}

/** @implements {mdDownloads.mojom.PageHandlerInterface} */
class TestDownloadsMojoHandler {
  /** @param {mdDownloads.mojom.PageInterface} */
  constructor(pageRouterProxy) {
    /** @private {mdDownloads.mojom.PageInterface} */
    this.pageRouterProxy_ = pageRouterProxy;

    /** @private {TestBrowserProxy} */
    this.callTracker_ = new TestBrowserProxy(['remove']);
  }

  /**
   * @param {string} methodName
   * @return {!Promise}
   */
  whenCalled(methodName) {
    return this.callTracker_.whenCalled(methodName);
  }

  /** @override */
  async remove(id) {
    this.pageRouterProxy_.removeItem(id);
    await this.pageRouterProxy_.flushForTesting();
    this.callTracker_.methodCalled('remove', id);
  }

  /** @override */
  getDownloads(searchTerms) {}

  /** @override */
  openFileRequiringGesture(id) {}

  /** @override */
  drag(id) {}

  /** @override */
  saveDangerousRequiringGesture(id) {}

  /** @override */
  discardDangerous(id) {}

  /** @override */
  retryDownload(id) {}

  /** @override */
  show(id) {}

  /** @override */
  pause(id) {}

  /** @override */
  resume(id) {}

  /** @override */
  undo() {}

  /** @override */
  cancel(id) {}

  /** @override */
  clearAll() {}

  /** @override */
  openDownloadsFolderRequiringGesture() {}
}

/**
 * @param {Object=} config
 * @return {!downloads.Data}
 */
function createDownload(config) {
  return Object.assign(
      {
        byExtId: '',
        byExtName: '',
        dangerType: downloads.DangerType.NOT_DANGEROUS,
        dateString: '',
        fileExternallyRemoved: false,
        filePath: '/some/file/path',
        fileName: 'download 1',
        fileUrl: 'file:///some/file/path',
        id: '',
        lastReasonText: '',
        otr: false,
        percent: 100,
        progressStatusText: '',
        resume: false,
        retry: false,
        return: false,
        sinceString: 'Today',
        started: Date.now() - 10000,
        state: downloads.States.COMPLETE,
        total: -1,
        url: 'http://permission.site',
      },
      config || {});
}
