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
#include "IpcLogging.h"
#include "rialtoipc.pb.h"


namespace
{
using google::protobuf::FieldDescriptor;
using google::protobuf::Message;
using google::protobuf::Reflection;

bool isFdField(const FieldDescriptor &field)
{
    const auto &options = field.options();
    return options.HasExtension(::firebolt::rialto::ipc::field_is_fd) &&
           options.GetExtension(::firebolt::rialto::ipc::field_is_fd);
}

// Sub-message access differs between the mutable and the const walk; everything else is shared.
Message &subMessage(Message &message, const Reflection &reflection, const FieldDescriptor &field)
{
    return *reflection.MutableMessage(&message, &field);
}

const Message &subMessage(const Message &message, const Reflection &reflection, const FieldDescriptor &field)
{
    return reflection.GetMessage(message, &field);
}

Message &repeatedSubMessage(Message &message, const Reflection &reflection, const FieldDescriptor &field, int index)
{
    return *reflection.MutableRepeatedMessage(&message, &field, index);
}

const Message &repeatedSubMessage(const Message &message, const Reflection &reflection, const FieldDescriptor &field,
                                  int index)
{
    return reflection.GetRepeatedMessage(message, &field, index);
}

template <typename MessageT, typename VisitorT> bool walk(MessageT &message, const VisitorT &visitor)
{
    const auto *descriptor = message.GetDescriptor();
    const Reflection *reflection = message.GetReflection();

    const int kFieldCount = descriptor->field_count();
    for (int i = 0; i < kFieldCount; ++i)
    {
        const FieldDescriptor &field = *descriptor->field(i);

        if (isFdField(field))
        {
            if (field.type() != FieldDescriptor::TYPE_INT32)
            {
                RIALTO_IPC_LOG_ERROR("field '%s' is marked as containing an fd but is not an int32 type",
                                     field.full_name().c_str());
                return false;
            }

            if (field.is_repeated())
            {
                const int kSize = reflection->FieldSize(message, &field);
                for (int index = 0; index < kSize; ++index)
                {
                    if (!visitor(message, field, index))
                        return false;
                }
            }
            else if (reflection->HasField(message, &field))
            {
                if (!visitor(message, field, -1))
                    return false;
            }
        }
        else if (field.cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE)
        {
            if (field.is_repeated())
            {
                const int kSize = reflection->FieldSize(message, &field);
                for (int index = 0; index < kSize; ++index)
                {
                    if (!walk(repeatedSubMessage(message, *reflection, field, index), visitor))
                        return false;
                }
            }
            else if (reflection->HasField(message, &field))
            {
                if (!walk(subMessage(message, *reflection, field), visitor))
                    return false;
            }
        }
    }

    return true;
}
} // namespace

namespace firebolt::rialto::ipc
{
bool walkFdFields(google::protobuf::Message &message, const FdFieldVisitor &visitor)
{
    return walk(message, visitor);
}

bool walkFdFields(const google::protobuf::Message &message, const ConstFdFieldVisitor &visitor)
{
    return walk(message, visitor);
}

int readFdField(const google::protobuf::Message &message, const google::protobuf::FieldDescriptor &field, int index)
{
    const Reflection *reflection = message.GetReflection();
    return (index < 0) ? reflection->GetInt32(message, &field) : reflection->GetRepeatedInt32(message, &field, index);
}

void writeFdField(google::protobuf::Message &message, const google::protobuf::FieldDescriptor &field, int index,
                  int value)
{
    const Reflection *reflection = message.GetReflection();
    if (index < 0)
        reflection->SetInt32(&message, &field, value);
    else
        reflection->SetRepeatedInt32(&message, &field, index, value);
}

} // namespace firebolt::rialto::ipc
