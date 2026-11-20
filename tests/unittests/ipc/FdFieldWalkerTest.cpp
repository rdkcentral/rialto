/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "FdFieldWalker.h"
#include "testmodule.pb.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;

namespace
{
struct Visit
{
    std::string field;
    int index;
    int value;

    bool operator==(const Visit &other) const
    {
        return field == other.field && index == other.index && value == other.value;
    }
};

std::ostream &operator<<(std::ostream &os, const Visit &visit)
{
    return os << "{" << visit.field << ", " << visit.index << ", " << visit.value << "}";
}

// Records every visited FD field, using the read-only walk.
std::vector<Visit> collect(const google::protobuf::Message &message, bool *result = nullptr)
{
    std::vector<Visit> visits;
    const bool kOk = walkFdFields(message,
                                  [&](const google::protobuf::Message &holder,
                                      const google::protobuf::FieldDescriptor &field, int index)
                                  {
                                      visits.push_back({field.name(), index, readFdField(holder, field, index)});
                                      return true;
                                  });
    if (result)
        *result = kOk;
    return visits;
}

void addHolder(TestFdNestedRepeated &message, int fd)
{
    auto *holder = message.add_holders();
    holder->set_id(fd * 100);
    holder->set_fd(fd);
}
} // namespace

TEST(FdFieldWalkerTest, ShouldVisitTopLevelFieldsInDeclarationOrder)
{
    TestFdTopLevel message;
    message.set_fd1(10);
    message.set_value(5);
    message.set_fd2(20);

    bool result{false};
    EXPECT_EQ(collect(message, &result), (std::vector<Visit>{{"fd1", -1, 10}, {"fd2", -1, 20}}));
    EXPECT_TRUE(result);
}

TEST(FdFieldWalkerTest, ShouldSkipUnsetTopLevelField)
{
    TestFdTopLevel message;
    message.set_fd2(20);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fd2", -1, 20}}));
}

TEST(FdFieldWalkerTest, ShouldVisitNothingForEmptyMessages)
{
    TestNoVar noFdFields;
    TestFdTopLevel noneSet;
    TestFdNestedRepeated noElements;

    bool result{false};
    EXPECT_TRUE(collect(noFdFields, &result).empty());
    EXPECT_TRUE(result);
    EXPECT_TRUE(collect(noneSet).empty());
    EXPECT_TRUE(collect(noElements).empty());
}

TEST(FdFieldWalkerTest, ShouldVisitOneNestedRepeatedFd)
{
    TestFdNestedRepeated message;
    addHolder(message, 7);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fd", -1, 7}}));
}

TEST(FdFieldWalkerTest, ShouldVisitThreeNestedRepeatedFdsInElementOrder)
{
    TestFdNestedRepeated message;
    addHolder(message, 7);
    addHolder(message, 8);
    addHolder(message, 9);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fd", -1, 7}, {"fd", -1, 8}, {"fd", -1, 9}}));
}

TEST(FdFieldWalkerTest, ShouldSkipNestedElementWithoutFd)
{
    TestFdNestedRepeated message;
    addHolder(message, 7);
    message.add_holders()->set_id(1);
    addHolder(message, 9);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fd", -1, 7}, {"fd", -1, 9}}));
}

TEST(FdFieldWalkerTest, ShouldVisitRepeatedFdFieldInIndexOrder)
{
    TestFdRepeatedField message;
    message.add_fds(4);
    message.add_fds(5);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fds", 0, 4}, {"fds", 1, 5}}));
}

TEST(FdFieldWalkerTest, ShouldWalkMixedFieldsInDeclarationOrderNotFieldNumberOrder)
{
    TestFdMixed message;
    message.set_last_fd(4);
    auto *first = message.add_holders();
    first->set_fd(2);
    auto *second = message.add_holders();
    second->set_fd(3);
    message.set_first_fd(1);

    EXPECT_EQ(collect(message),
              (std::vector<Visit>{{"first_fd", -1, 1}, {"fd", -1, 2}, {"fd", -1, 3}, {"last_fd", -1, 4}}));
}

TEST(FdFieldWalkerTest, ShouldSkipUnsetOptionalSubMessage)
{
    TestFdOptionalNested message;
    message.set_fd(9);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fd", -1, 9}}));
    EXPECT_FALSE(message.has_holder());
}

TEST(FdFieldWalkerTest, ShouldSkipSetSubMessageWithoutFd)
{
    TestFdOptionalNested message;
    message.mutable_holder()->set_id(1);
    message.set_fd(9);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fd", -1, 9}}));
}

TEST(FdFieldWalkerTest, ShouldVisitSetSubMessageAtItsOwnPosition)
{
    TestFdOptionalNested message;
    message.mutable_holder()->set_fd(5);
    message.set_fd(9);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fd", -1, 5}, {"fd", -1, 9}}));
}

TEST(FdFieldWalkerTest, ShouldVisitFieldsTwoLevelsDeep)
{
    TestFdDeep message;
    addHolder(*message.mutable_inner(), 11);
    addHolder(*message.mutable_inner(), 12);

    EXPECT_EQ(collect(message), (std::vector<Visit>{{"fd", -1, 11}, {"fd", -1, 12}}));
}

TEST(FdFieldWalkerTest, ShouldFailForNonInt32FdField)
{
    TestFdNotInt32 message;
    message.set_fd("not a descriptor");

    bool result{true};
    EXPECT_TRUE(collect(message, &result).empty());
    EXPECT_FALSE(result);
}

TEST(FdFieldWalkerTest, ShouldFailForNonInt32FdFieldEvenWhenUnset)
{
    TestFdNotInt32 message;

    bool result{true};
    collect(message, &result);
    EXPECT_FALSE(result);
}

TEST(FdFieldWalkerTest, ShouldStopWhenVisitorReturnsFalse)
{
    TestFdNestedRepeated message;
    addHolder(message, 7);
    addHolder(message, 8);

    int visits{0};
    EXPECT_FALSE(walkFdFields(static_cast<const google::protobuf::Message &>(message),
                              [&](const google::protobuf::Message &, const google::protobuf::FieldDescriptor &, int)
                              {
                                  ++visits;
                                  return false;
                              }));
    EXPECT_EQ(visits, 1);
}

TEST(FdFieldWalkerTest, ShouldRewriteValuesThroughMutableWalk)
{
    TestFdMixed message;
    message.set_first_fd(1);
    message.add_holders()->set_fd(2);
    message.set_last_fd(3);
    TestFdRepeatedField repeated;
    repeated.add_fds(4);
    repeated.add_fds(5);

    auto replaceWithMinusOne =
        [](google::protobuf::Message &holder, const google::protobuf::FieldDescriptor &field, int index)
    {
        writeFdField(holder, field, index, -1);
        return true;
    };
    EXPECT_TRUE(walkFdFields(static_cast<google::protobuf::Message &>(message), replaceWithMinusOne));
    EXPECT_TRUE(walkFdFields(static_cast<google::protobuf::Message &>(repeated), replaceWithMinusOne));

    EXPECT_EQ(message.first_fd(), -1);
    EXPECT_EQ(message.holders(0).fd(), -1);
    EXPECT_EQ(message.last_fd(), -1);
    EXPECT_EQ(repeated.fds(0), -1);
    EXPECT_EQ(repeated.fds(1), -1);
}
