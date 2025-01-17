// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/background_fetch/background_fetch_struct_traits.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "content/common/background_fetch/background_fetch_types.h"
#include "mojo/public/cpp/base/string16_mojom_traits.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/manifest/manifest.h"
#include "third_party/blink/public/common/manifest/manifest_mojom_traits.h"
#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"
#include "ui/gfx/geometry/size.h"
#include "url/mojom/url_gurl_mojom_traits.h"

namespace content {

namespace {

// Creates a new ImageResource object for the given arguments.
blink::Manifest::ImageResource CreateIcon(const std::string& src,
                                          std::vector<gfx::Size> sizes,
                                          const std::string& type) {
  blink::Manifest::ImageResource icon;
  icon.src = GURL(src);
  icon.sizes = std::move(sizes);
  icon.type = base::ASCIIToUTF16(type);

  return icon;
}

}  // namespace

TEST(BackgroundFetchStructTraitsTest, BackgroundFetchRegistrationRoundTrip) {
  BackgroundFetchRegistration registration;
  registration.developer_id = "my_id";
  registration.unique_id = "7e57ab1e-c0de-a150-ca75-1e75f005ba11";
  registration.download_total = 9001;
  registration.result = blink::mojom::BackgroundFetchResult::FAILURE;
  registration.failure_reason =
      blink::mojom::BackgroundFetchFailureReason::CANCELLED_FROM_UI;

  BackgroundFetchRegistration roundtrip_registration;
  ASSERT_TRUE(blink::mojom::BackgroundFetchRegistration::Deserialize(
      blink::mojom::BackgroundFetchRegistration::Serialize(&registration),
      &roundtrip_registration));

  EXPECT_EQ(roundtrip_registration.developer_id, registration.developer_id);
  EXPECT_EQ(roundtrip_registration.unique_id, registration.unique_id);

  EXPECT_EQ(roundtrip_registration.download_total, registration.download_total);
  EXPECT_EQ(roundtrip_registration.result, registration.result);
  EXPECT_EQ(roundtrip_registration.failure_reason, registration.failure_reason);
}

TEST(BackgroundFetchStructTraitsTest, ImageResourceRoundtrip) {
  blink::Manifest::ImageResource icon =
      CreateIcon("my_icon.png", {{256, 256}}, "image/png");

  blink::Manifest::ImageResource roundtrip_icon;
  ASSERT_TRUE(blink::mojom::ManifestImageResource::Deserialize(
      blink::mojom::ManifestImageResource::Serialize(&icon), &roundtrip_icon));

  EXPECT_EQ(icon, roundtrip_icon);
}

}  // namespace content
