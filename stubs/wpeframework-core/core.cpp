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

#include "core/JSON.h"
#include "core/Portability.h"
#include "core/Serialization.h"
#include <string>

namespace Thunder::Core
{
namespace JSON
{
char IElement::NullTag[5] = {'n', 'u', 'l', 'l', '\0'};
char IElement::TrueTag[5] = {'t', 'r', 'u', 'e', '\0'};
char IElement::FalseTag[6] = {'f', 'a', 'l', 's', 'e', '\0'};
} // namespace JSON
std::string ToQuotedString(const TCHAR quote, const string &input)
{
    return "";
}

int8_t ToCodePoint(const TCHAR *data, const uint8_t length, uint32_t &codePoint)
{
    return 0;
}

int8_t FromCodePoint(uint32_t codePoint, TCHAR *data, const uint8_t length)
{
    return 0;
}

bool CodePointToUTF16(const uint32_t codePoint, uint16_t &lowPart, uint16_t &highPart)
{
    return false;
}

bool UTF16ToCodePoint(const uint16_t lowPart, const uint16_t highPart, uint32_t &codePoint)
{
    return false;
}
} // namespace Thunder::Core

void DumpCallStack(const ThreadId threadId, std::list<Thunder::Core::callstack_info> &stack) {}
