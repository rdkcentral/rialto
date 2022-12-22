/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "MediaSourceUtil.h"

namespace firebolt::rialto
{
bool operator==(const AudioConfig &lac, const AudioConfig &rac)
{
    return lac.numberOfChannels == rac.numberOfChannels && lac.sampleRate == rac.sampleRate &&
           std::equal(std::begin(lac.codecSpecificConfig), std::end(lac.codecSpecificConfig),
                      std::begin(rac.codecSpecificConfig));
}

bool operator==(const IMediaPipeline::MediaSource &lms, const IMediaPipeline::MediaSource &rms)
{
    return lms.getId() == rms.getId() && lms.getType() == rms.getType() && lms.getMimeType() == rms.getMimeType();
}
} // namespace firebolt::rialto
