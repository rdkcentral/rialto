/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include "GstExpect.h"

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

namespace
{
GParamSpec gParamSpec{};
}
namespace firebolt::rialto::server::testcommon
{

template <typename T>
void expectSetProperty(std::shared_ptr<firebolt::rialto::wrappers::GlibWrapperMock> glibWrapperMock,
                       std::shared_ptr<firebolt::rialto::wrappers::GstWrapperMock> gstWrapperMock, GstElement *element,
                       const std::string &propertyName, const T &value)
{
    EXPECT_CALL(*glibWrapperMock, gObjectClassFindProperty(G_OBJECT_GET_CLASS(element), StrEq(propertyName.c_str())))
        .WillOnce(Return(&gParamSpec));

    if constexpr (std::is_same_v<T, bool>)
    {
        EXPECT_CALL(*glibWrapperMock, gObjectSetBoolStub(_, StrEq(propertyName.c_str()), value)).Times(1);
    }
    else if constexpr (std::is_same_v<T, int32_t>)
    {
        EXPECT_CALL(*glibWrapperMock, gObjectSetIntStub(_, StrEq(propertyName.c_str()), value)).Times(1);
    }
    else
    {
        EXPECT_CALL(*glibWrapperMock, gObjectSetStub(_, StrEq(propertyName.c_str()))).Times(1);
    }
}

template void expectSetProperty<bool>(std::shared_ptr<firebolt::rialto::wrappers::GlibWrapperMock> glibWrapperMock,
                                      std::shared_ptr<firebolt::rialto::wrappers::GstWrapperMock> gstWrapperMock,
                                      GstElement *element, const std::string &propertyName, const bool &value);
template void expectSetProperty<int>(std::shared_ptr<firebolt::rialto::wrappers::GlibWrapperMock> glibWrapperMock,
                                     std::shared_ptr<firebolt::rialto::wrappers::GstWrapperMock> gstWrapperMock,
                                     GstElement *element, const std::string &propertyName, const int &value);

void expectPropertyDoesntExist(std::shared_ptr<firebolt::rialto::wrappers::GlibWrapperMock> glibWrapperMock,
                               std::shared_ptr<firebolt::rialto::wrappers::GstWrapperMock> gstWrapperMock,
                               GstElement *element, const std::string &propertyName)
{
    EXPECT_CALL(*glibWrapperMock, gObjectClassFindProperty(G_OBJECT_GET_CLASS(element), StrEq(propertyName.c_str())))
        .WillOnce(Return(nullptr));
}

} // namespace firebolt::rialto::server::testcommon
