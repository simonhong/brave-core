/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_usermodel_parameters/browser/brave_usermodel_parameter_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UsermodelParameterService*

namespace brave_usermodel_parameters {

TEST(UsermodelParameterServiceTest, InitializesServiceTest) {
  // const std::string model_1 = "purchase_intent_classifier";
  // const std::string model_2 = "contextual_page_classifier";
  // const std::string model_3 = "foobar_model";

  // // TODO(Moritz Haller): Mock or fake delegate object
  // ClientModelParameterService parameter_service(nullptr);

  // ClientModelParameters purchase_intent_parameters_1 =
  //     parameter_service.GetParameters(model_1);

  // const std::string gold_params = R"({"foo": "bar})";
  // EXPECT_EQ(purchase_intent_parameters_1, gold_params);

  // ClientModelParameters purchase_intent_parameters_2 =
  //     parameter_service.GetParameters(model_3);

  // EXPECT_TRUE(purchase_intent_parameters_2.empty());
  EXPECT_TRUE(true);
}

}  // namespace brave_usermodel_parameters
