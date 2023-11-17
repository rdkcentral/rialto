/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "SchemaVersion.h"
#include <gtest/gtest.h>

using firebolt::rialto::common::SchemaVersion;

TEST(SchemaVersionTests, shouldInitialize)
{
    SchemaVersion schema{1, 2, 3};
    EXPECT_EQ(schema.major(), 1);
    EXPECT_EQ(schema.minor(), 2);
    EXPECT_EQ(schema.patch(), 3);
}

TEST(SchemaVersionTests, shouldReturnCurrentSchemaVersion)
{
    const auto kCurrentSchemaVersion{firebolt::rialto::common::getCurrentSchemaVersion()};
    const std::string kExpectedSchemaVersion{std::string(PROJECT_VER_MAJOR) + "." + std::string(PROJECT_VER_MINOR) +
                                             "." + std::string(PROJECT_VER_PATCH)};
    EXPECT_EQ(kCurrentSchemaVersion.str(), kExpectedSchemaVersion);
}

TEST(SchemaVersionTests, shouldEqual)
{
    SchemaVersion schema1{1, 2, 3};
    SchemaVersion schema2{1, 2, 3};
    EXPECT_EQ(schema1, schema2);
}

TEST(SchemaVersionTests, shouldBeNotEqualWhenPatchVersionIsDifferent)
{
    SchemaVersion schema1{1, 2, 3};
    SchemaVersion schema2{1, 2, 4};
    EXPECT_FALSE(schema1 == schema2);
}

TEST(SchemaVersionTests, shouldBeNotEqualWhenMinorVersionIsDifferent)
{
    SchemaVersion schema1{1, 2, 3};
    SchemaVersion schema2{1, 4, 3};
    EXPECT_FALSE(schema1 == schema2);
}

TEST(SchemaVersionTests, shouldBeNotEqualWhenMajorVersionIsDifferent)
{
    SchemaVersion schema1{1, 2, 3};
    SchemaVersion schema2{4, 2, 3};
    EXPECT_FALSE(schema1 == schema2);
}

TEST(SchemaVersionTests, shouldBeCompatibleWhenSchemasAreEqual)
{
    SchemaVersion schema1{1, 2, 3};
    SchemaVersion schema2{1, 2, 3};
    EXPECT_TRUE(schema1.isCompatible(schema2));
}

TEST(SchemaVersionTests, shouldBeCompatibleWhenPatchVersionIsDifferent)
{
    SchemaVersion schema1{1, 2, 3};
    SchemaVersion schema2{1, 2, 4};
    EXPECT_TRUE(schema1.isCompatible(schema2));
}

TEST(SchemaVersionTests, shouldBeCompatibleWhenMinorVersionIsDifferent)
{
    SchemaVersion schema1{1, 2, 3};
    SchemaVersion schema2{1, 4, 3};
    EXPECT_TRUE(schema1.isCompatible(schema2));
}

TEST(SchemaVersionTests, shouldBeNotCompatibleWhenMajorVersionIsDifferent)
{
    SchemaVersion schema1{1, 2, 3};
    SchemaVersion schema2{4, 2, 3};
    EXPECT_FALSE(schema1.isCompatible(schema2));
}

TEST(SchemaVersionTests, shouldConvertToString)
{
    SchemaVersion schema{1, 2, 3};
    EXPECT_EQ(schema.str(), "1.2.3");
}
