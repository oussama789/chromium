// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_log_util.h"

#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

TEST(SpdyLogUtilTest, ElideGoAwayDebugDataForNetLog) {
  // Only elide for appropriate log level.
  EXPECT_EQ(
      "[6 bytes were stripped]",
      ElideGoAwayDebugDataForNetLog(NetLogCaptureMode::Default(), "foobar"));
  EXPECT_EQ("foobar",
            ElideGoAwayDebugDataForNetLog(
                NetLogCaptureMode::IncludeCookiesAndCredentials(), "foobar"));
}

TEST(SpdyLogUtilTest, ElideSpdyHeaderBlockForNetLog) {
  spdy::SpdyHeaderBlock headers;
  headers["foo"] = "bar";
  headers["cookie"] = "name=value";

  std::unique_ptr<base::ListValue> list =
      ElideSpdyHeaderBlockForNetLog(headers, NetLogCaptureMode::Default());

  ASSERT_TRUE(list);
  ASSERT_EQ(2u, list->GetList().size());

  ASSERT_TRUE(list->GetList()[0].is_string());
  EXPECT_EQ("foo: bar", list->GetList()[0].GetString());

  ASSERT_TRUE(list->GetList()[1].is_string());
  EXPECT_EQ("cookie: [10 bytes were stripped]", list->GetList()[1].GetString());

  list = ElideSpdyHeaderBlockForNetLog(
      headers, NetLogCaptureMode::IncludeCookiesAndCredentials());

  ASSERT_TRUE(list);
  ASSERT_EQ(2u, list->GetList().size());

  ASSERT_TRUE(list->GetList()[0].is_string());
  EXPECT_EQ("foo: bar", list->GetList()[0].GetString());

  ASSERT_TRUE(list->GetList()[1].is_string());
  EXPECT_EQ("cookie: name=value", list->GetList()[1].GetString());
}

TEST(SpdyLogUtilTest, SpdyHeaderBlockNetLogCallback) {
  spdy::SpdyHeaderBlock headers;
  headers["foo"] = "bar";
  headers["cookie"] = "name=value";

  std::unique_ptr<base::Value> dict =
      SpdyHeaderBlockNetLogCallback(&headers, NetLogCaptureMode::Default());

  ASSERT_TRUE(dict);
  ASSERT_TRUE(dict->is_dict());
  ASSERT_EQ(1u, dict->DictSize());

  auto* header_list = dict->FindKey("headers");
  ASSERT_TRUE(header_list);
  ASSERT_TRUE(header_list->is_list());
  ASSERT_EQ(2u, header_list->GetList().size());

  ASSERT_TRUE(header_list->GetList()[0].is_string());
  EXPECT_EQ("foo: bar", header_list->GetList()[0].GetString());

  ASSERT_TRUE(header_list->GetList()[1].is_string());
  EXPECT_EQ("cookie: [10 bytes were stripped]",
            header_list->GetList()[1].GetString());

  dict = SpdyHeaderBlockNetLogCallback(
      &headers, NetLogCaptureMode::IncludeCookiesAndCredentials());

  ASSERT_TRUE(dict);
  ASSERT_TRUE(dict->is_dict());
  ASSERT_EQ(1u, dict->DictSize());

  header_list = dict->FindKey("headers");
  ASSERT_TRUE(header_list);
  ASSERT_TRUE(header_list->is_list());
  ASSERT_EQ(2u, header_list->GetList().size());

  ASSERT_TRUE(header_list->GetList()[0].is_string());
  EXPECT_EQ("foo: bar", header_list->GetList()[0].GetString());

  ASSERT_TRUE(header_list->GetList()[1].is_string());
  EXPECT_EQ("cookie: name=value", header_list->GetList()[1].GetString());
}

}  // namespace net
