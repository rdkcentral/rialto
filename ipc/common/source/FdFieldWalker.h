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

#ifndef FIREBOLT_RIALTO_IPC_FD_FIELD_WALKER_H_
#define FIREBOLT_RIALTO_IPC_FD_FIELD_WALKER_H_

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <functional>

// -----------------------------------------------------------------------------
/*!
    \file FdFieldWalker.h
    \brief Finds the file descriptor fields of a protobuf message.

    A field carries a file descriptor when it is marked with the
    \c field_is_fd option.  Sender and receiver must agree on the order in
    which those fields are matched to the descriptors sent alongside the
    message, so every place that collects or restores descriptors uses this
    walker.

    The walk order is:
      1. fields in declaration order (\c Descriptor::field(i));
      2. for a repeated \c int32 FD field, each element in index order;
      3. for a present singular sub-message, the whole sub-message is walked
         at its own field position; for a repeated sub-message, each element
         is walked in index order.

    Absent fields and absent sub-messages are skipped.  For messages whose FD
    fields are all top-level this is the same order as the original
    top-level-only loops.
 */

namespace firebolt::rialto::ipc
{
/**
 * @brief Called for each present FD field.
 *
 * @param[in] message : The (sub-)message that holds the field.
 * @param[in] field   : The FD field.
 * @param[in] index   : The element index for a repeated field, or -1 for a singular field.
 *
 * @retval false to stop the walk.
 */
using FdFieldVisitor =
    std::function<bool(google::protobuf::Message &message, const google::protobuf::FieldDescriptor &field, int index)>;

/**
 * @brief Read-only variant of FdFieldVisitor.
 */
using ConstFdFieldVisitor =
    std::function<bool(const google::protobuf::Message &message, const google::protobuf::FieldDescriptor &field, int index)>;

/**
 * @brief Visits every present FD field of \a message, at any depth, in walk order.
 *
 * @retval false if a field marked \c field_is_fd is not an \c int32 field (logged), or if the visitor stopped
 *               the walk; true otherwise.
 */
bool walkFdFields(google::protobuf::Message &message, const FdFieldVisitor &visitor);

/**
 * @brief Read-only overload of walkFdFields().
 */
bool walkFdFields(const google::protobuf::Message &message, const ConstFdFieldVisitor &visitor);

/**
 * @brief Returns the value of an FD field reported by the walker.
 */
int readFdField(const google::protobuf::Message &message, const google::protobuf::FieldDescriptor &field, int index);

/**
 * @brief Sets the value of an FD field reported by the walker.
 */
void writeFdField(google::protobuf::Message &message, const google::protobuf::FieldDescriptor &field, int index,
                  int value);

} // namespace firebolt::rialto::ipc

#endif // FIREBOLT_RIALTO_IPC_FD_FIELD_WALKER_H_
