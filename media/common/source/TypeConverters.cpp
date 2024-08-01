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

#include "TypeConverters.h"

namespace firebolt::rialto::common
{
const char *convertMediaSourceType(const MediaSourceType &mediaSourceType)
{
    switch (mediaSourceType)
    {
    case MediaSourceType::AUDIO:
        return "Audio";
    case MediaSourceType::VIDEO:
        return "Video";
    case MediaSourceType::SUBTITLE:
        return "Subtitle";
    case MediaSourceType::UNKNOWN:
    default:
        return "Unknown";
    }
}

const char *convertLayout(const Layout &layout)
{
    switch (layout)
    {
    case Layout::INTERLEAVED:
        return "interleaved";
    case Layout::NON_INTERLEAVED:
        return "non-interleaved";
    }
    return "";
}

const char *convertFormat(const Format &format)
{
    switch (format)
    {
    case Format::S8:
        return "S8";
    case Format::U8:
        return "U8";
    case Format::S16LE:
        return "S16LE";
    case Format::S16BE:
        return "S16BE";
    case Format::U16LE:
        return "U16LE";
    case Format::U16BE:
        return "U16BE";
    case Format::S24_32LE:
        return "S24_32LE";
    case Format::S24_32BE:
        return "S24_32BE";
    case Format::U24_32LE:
        return "U24_32LE";
    case Format::U24_32BE:
        return "U24_32BE";
    case Format::S32LE:
        return "S32LE";
    case Format::S32BE:
        return "S32BE";
    case Format::U32LE:
        return "U32LE";
    case Format::U32BE:
        return "U32BE";
    case Format::S24LE:
        return "S24LE";
    case Format::S24BE:
        return "S24BE";
    case Format::U24LE:
        return "U24LE";
    case Format::U24BE:
        return "U24BE";
    case Format::S20LE:
        return "S20LE";
    case Format::S20BE:
        return "S20BE";
    case Format::U20LE:
        return "U20LE";
    case Format::U20BE:
        return "U20BE";
    case Format::S18LE:
        return "S18LE";
    case Format::S18BE:
        return "S18BE";
    case Format::U18LE:
        return "U18LE";
    case Format::U18BE:
        return "U18BE";
    case Format::F32LE:
        return "F32LE";
    case Format::F32BE:
        return "F32BE";
    case Format::F64LE:
        return "F64LE";
    case Format::F64BE:
        return "F64BE";
    }
    return "";
}
} // namespace firebolt::rialto::common
