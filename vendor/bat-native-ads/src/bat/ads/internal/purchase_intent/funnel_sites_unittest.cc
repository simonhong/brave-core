/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/purchase_intent/funnel_sites.h"

#include <string>
#include <vector>

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

struct TestInfo {
  std::string url;
  FunnelSiteInfo funnel_site_info;
};

}  // namespace

const std::vector<TestInfo> kTests = {
  { "http://www.carmax.com", _automotive_funnel_sites.at(1) },
  { "http://www.carmax.com/foobar", _automotive_funnel_sites.at(1) },
  { "http://carmax.com", _automotive_funnel_sites.at(1) },
  { "http://brave.com/foobar", FunnelSiteInfo() },
};

TEST(BatAdsPurchaseIntentFunnelSitesTest,
    MatchesFunnelSites) {
  for (const auto& test : kTests) {
    // Arrange
    const std::string url = test.url;

    // Act
    const FunnelSiteInfo funnel_site = FunnelSites::GetFunnelSite(url);

    // Assert
    const FunnelSiteInfo expected_funnel_site = test.funnel_site_info;

    EXPECT_EQ(expected_funnel_site.weight, funnel_site.weight);
    EXPECT_EQ(expected_funnel_site.url_netloc, funnel_site.url_netloc);
  }
}

}  // namespace ads
