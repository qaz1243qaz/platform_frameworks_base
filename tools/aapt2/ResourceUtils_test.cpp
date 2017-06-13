/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ResourceUtils.h"

#include "Resource.h"
#include "test/Test.h"

using ::aapt::test::ValueEq;
using ::android::Res_value;
using ::android::ResTable_map;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Pointee;

namespace aapt {

TEST(ResourceUtilsTest, ParseBool) {
  EXPECT_THAT(ResourceUtils::ParseBool("true"), Eq(Maybe<bool>(true)));
  EXPECT_THAT(ResourceUtils::ParseBool("TRUE"), Eq(Maybe<bool>(true)));
  EXPECT_THAT(ResourceUtils::ParseBool("True"), Eq(Maybe<bool>(true)));

  EXPECT_THAT(ResourceUtils::ParseBool("false"), Eq(Maybe<bool>(false)));
  EXPECT_THAT(ResourceUtils::ParseBool("FALSE"), Eq(Maybe<bool>(false)));
  EXPECT_THAT(ResourceUtils::ParseBool("False"), Eq(Maybe<bool>(false)));
}

TEST(ResourceUtilsTest, ParseResourceName) {
  ResourceNameRef actual;
  bool actual_priv = false;
  EXPECT_TRUE(ResourceUtils::ParseResourceName("android:color/foo", &actual, &actual_priv));
  EXPECT_THAT(actual, Eq(ResourceNameRef("android", ResourceType::kColor, "foo")));
  EXPECT_FALSE(actual_priv);

  EXPECT_TRUE(ResourceUtils::ParseResourceName("color/foo", &actual, &actual_priv));
  EXPECT_THAT(actual, Eq(ResourceNameRef({}, ResourceType::kColor, "foo")));
  EXPECT_FALSE(actual_priv);

  EXPECT_TRUE(ResourceUtils::ParseResourceName("*android:color/foo", &actual, &actual_priv));
  EXPECT_THAT(actual, Eq(ResourceNameRef("android", ResourceType::kColor, "foo")));
  EXPECT_TRUE(actual_priv);

  EXPECT_FALSE(ResourceUtils::ParseResourceName(android::StringPiece(), &actual, &actual_priv));
}

TEST(ResourceUtilsTest, ParseReferenceWithNoPackage) {
  ResourceNameRef actual;
  bool create = false;
  bool private_ref = false;
  EXPECT_TRUE(ResourceUtils::ParseReference("@color/foo", &actual, &create, &private_ref));
  EXPECT_THAT(actual, Eq(ResourceNameRef({}, ResourceType::kColor, "foo")));
  EXPECT_FALSE(create);
  EXPECT_FALSE(private_ref);
}

TEST(ResourceUtilsTest, ParseReferenceWithPackage) {
  ResourceNameRef actual;
  bool create = false;
  bool private_ref = false;
  EXPECT_TRUE(ResourceUtils::ParseReference("@android:color/foo", &actual, &create, &private_ref));
  EXPECT_THAT(actual, Eq(ResourceNameRef("android", ResourceType::kColor, "foo")));
  EXPECT_FALSE(create);
  EXPECT_FALSE(private_ref);
}

TEST(ResourceUtilsTest, ParseReferenceWithSurroundingWhitespace) {
  ResourceNameRef actual;
  bool create = false;
  bool private_ref = false;
  EXPECT_TRUE(ResourceUtils::ParseReference("\t @android:color/foo\n \n\t", &actual, &create, &private_ref));
  EXPECT_THAT(actual, Eq(ResourceNameRef("android", ResourceType::kColor, "foo")));
  EXPECT_FALSE(create);
  EXPECT_FALSE(private_ref);
}

TEST(ResourceUtilsTest, ParseAutoCreateIdReference) {
  ResourceNameRef actual;
  bool create = false;
  bool private_ref = false;
  EXPECT_TRUE(ResourceUtils::ParseReference("@+android:id/foo", &actual, &create, &private_ref));
  EXPECT_THAT(actual, Eq(ResourceNameRef("android", ResourceType::kId, "foo")));
  EXPECT_TRUE(create);
  EXPECT_FALSE(private_ref);
}

TEST(ResourceUtilsTest, ParsePrivateReference) {
  ResourceNameRef actual;
  bool create = false;
  bool private_ref = false;
  EXPECT_TRUE(ResourceUtils::ParseReference("@*android:id/foo", &actual, &create, &private_ref));
  EXPECT_THAT(actual, Eq(ResourceNameRef("android", ResourceType::kId, "foo")));
  EXPECT_FALSE(create);
  EXPECT_TRUE(private_ref);
}

TEST(ResourceUtilsTest, FailToParseAutoCreateNonIdReference) {
  bool create = false;
  bool private_ref = false;
  ResourceNameRef actual;
  EXPECT_FALSE(ResourceUtils::ParseReference("@+android:color/foo", &actual, &create, &private_ref));
}

TEST(ResourceUtilsTest, ParseAttributeReferences) {
  EXPECT_TRUE(ResourceUtils::IsAttributeReference("?android"));
  EXPECT_TRUE(ResourceUtils::IsAttributeReference("?android:foo"));
  EXPECT_TRUE(ResourceUtils::IsAttributeReference("?attr/foo"));
  EXPECT_TRUE(ResourceUtils::IsAttributeReference("?android:attr/foo"));
}

TEST(ResourceUtilsTest, FailParseIncompleteReference) {
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?style/foo"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?android:style/foo"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?android:"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?android:attr/"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?:attr/"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?:attr/foo"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?:/"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?:/foo"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?attr/"));
  EXPECT_FALSE(ResourceUtils::IsAttributeReference("?/foo"));
}

TEST(ResourceUtilsTest, ParseStyleParentReference) {
  const ResourceName kAndroidStyleFooName("android", ResourceType::kStyle, "foo");
  const ResourceName kStyleFooName({}, ResourceType::kStyle, "foo");

  std::string err_str;
  Maybe<Reference> ref = ResourceUtils::ParseStyleParentReference("@android:style/foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kAndroidStyleFooName)));

  ref = ResourceUtils::ParseStyleParentReference("@style/foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kStyleFooName)));

  ref = ResourceUtils::ParseStyleParentReference("?android:style/foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kAndroidStyleFooName)));

  ref = ResourceUtils::ParseStyleParentReference("?style/foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kStyleFooName)));

  ref = ResourceUtils::ParseStyleParentReference("android:style/foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kAndroidStyleFooName)));

  ref = ResourceUtils::ParseStyleParentReference("android:foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kAndroidStyleFooName)));

  ref = ResourceUtils::ParseStyleParentReference("@android:foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kAndroidStyleFooName)));

  ref = ResourceUtils::ParseStyleParentReference("foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kStyleFooName)));

  ref = ResourceUtils::ParseStyleParentReference("*android:style/foo", &err_str);
  ASSERT_TRUE(ref);
  EXPECT_THAT(ref.value().name, Eq(make_value(kAndroidStyleFooName)));
  EXPECT_TRUE(ref.value().private_reference);
}

TEST(ResourceUtilsTest, ParseEmptyFlag) {
  std::unique_ptr<Attribute> attr =
      test::AttributeBuilder(false)
          .SetTypeMask(ResTable_map::TYPE_FLAGS)
          .AddItem("one", 0x01)
          .AddItem("two", 0x02)
          .Build();

  std::unique_ptr<BinaryPrimitive> result = ResourceUtils::TryParseFlagSymbol(attr.get(), "");
  ASSERT_THAT(result, NotNull());
  EXPECT_THAT(result->value.data, Eq(0u));
}

TEST(ResourceUtilsTest, NullIsEmptyReference) {
  ASSERT_THAT(ResourceUtils::MakeNull(), Pointee(ValueEq(Reference())));
  ASSERT_THAT(ResourceUtils::TryParseNullOrEmpty("@null"), Pointee(ValueEq(Reference())));
}

TEST(ResourceUtilsTest, EmptyIsBinaryPrimitive) {
  ASSERT_THAT(ResourceUtils::MakeEmpty(), Pointee(ValueEq(BinaryPrimitive(Res_value::TYPE_NULL, Res_value::DATA_NULL_EMPTY))));
  ASSERT_THAT(ResourceUtils::TryParseNullOrEmpty("@empty"), Pointee(ValueEq(BinaryPrimitive(Res_value::TYPE_NULL, Res_value::DATA_NULL_EMPTY))));
}

}  // namespace aapt
