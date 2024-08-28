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

#include "GenericTasksTestsBase.h"

class AttachSourceTest : public GenericTasksTestsBase
{
};

TEST_F(AttachSourceTest, shouldAttachAudioSource)
{
    shouldAttachAudioSource();
    triggerAttachAudioSource();
    checkAudioSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachAudioSourceWithChannelsAndRateAndDrm)
{
    shouldAttachAudioSourceWithChannelsAndRate();
    triggerAttachAudioSourceWithChannelsAndRateAndDrm();
    checkAudioSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldAttachOpusWithAudioSpecificConf)
{
    shouldAttachAudioSourceWithAudioSpecificConf();
    triggerAttachOpusAudioSourceWithAudioSpecificConf();
    checkAudioSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachBwavAudioSource)
{
    shouldAttachBwavAudioSource();
    triggerAttachBwavAudioSource();
    checkAudioSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceAuAvc)
{
    std::string mimeType = "video/h264";
    firebolt::rialto::SegmentAlignment alignment = firebolt::rialto::SegmentAlignment::AU;
    firebolt::rialto::StreamFormat streamFormat = firebolt::rialto::StreamFormat::AVC;
    std::string expectedMimeType = "video/x-h264";
    std::string expectedAlignment = "au";
    std::string expectedStreamFormat = "avc";

    shouldAttachVideoSource(expectedMimeType, expectedAlignment, expectedStreamFormat);
    triggerAttachVideoSource(mimeType, alignment, streamFormat);
    checkVideoSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceNalAvc)
{
    std::string mimeType = "video/h264";
    firebolt::rialto::SegmentAlignment alignment = firebolt::rialto::SegmentAlignment::NAL;
    firebolt::rialto::StreamFormat streamFormat = firebolt::rialto::StreamFormat::AVC;
    std::string expectedMimeType = "video/x-h264";
    std::string expectedAlignment = "nal";
    std::string expectedStreamFormat = "avc";

    shouldAttachVideoSource(expectedMimeType, expectedAlignment, expectedStreamFormat);
    triggerAttachVideoSource(mimeType, alignment, streamFormat);
    checkVideoSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceAuHvc)
{
    std::string mimeType = "video/h265";
    firebolt::rialto::SegmentAlignment alignment = firebolt::rialto::SegmentAlignment::AU;
    firebolt::rialto::StreamFormat streamFormat = firebolt::rialto::StreamFormat::HVC1;
    std::string expectedMimeType = "video/x-h265";
    std::string expectedAlignment = "au";
    std::string expectedStreamFormat = "hvc1";

    shouldAttachVideoSource(expectedMimeType, expectedAlignment, expectedStreamFormat);
    triggerAttachVideoSource(mimeType, alignment, streamFormat);
    checkVideoSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceAuHev)
{
    std::string mimeType = "video/h265";
    firebolt::rialto::SegmentAlignment alignment = firebolt::rialto::SegmentAlignment::AU;
    firebolt::rialto::StreamFormat streamFormat = firebolt::rialto::StreamFormat::HEV1;
    std::string expectedMimeType = "video/x-h265";
    std::string expectedAlignment = "au";
    std::string expectedStreamFormat = "hev1";

    shouldAttachVideoSource(expectedMimeType, expectedAlignment, expectedStreamFormat);
    triggerAttachVideoSource(mimeType, alignment, streamFormat);
    checkVideoSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachSubtitleSource)
{
    shouldAttachSubtitleSource();
    triggerAttachSubtitleSource();
    checkSubtitleSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceWithStringCodecData)
{
    shouldAttachVideoSourceWithStringCodecData();
    triggerAttachVideoSourceWithStringCodecData();
    checkVideoSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceEmptyCodecData)
{
    shouldAttachVideoSourceWithEmptyCodecData();
    triggerAttachVideoSourceWithEmptyCodecData();
    checkVideoSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldAttachVideoDolbyVisionSource)
{
    shouldAttachVideoSourceWithDolbyVisionSource();
    triggerAttachVideoSourceWithDolbyVisionSource();
    checkVideoSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldSwitchAudioSource)
{
    shouldSwitchAudioSource();
    triggerReattachAudioSource();
    checkNewAudioSourceAttached();
}

TEST_F(AttachSourceTest, shouldReattachAudioSource)
{
    shouldReattachAudioSource();
    triggerReattachAudioSource();
    checkNewAudioSourceAttached();
}

TEST_F(AttachSourceTest, shouldNotReattachAudioSourceWhenMimeTypeIsEmpty)
{
    shouldNotSwitchAudioSourceWhenMimeTypeIsEmpty();
    triggerReattachAudioSourceWithEmptyMimeType();
}

TEST_F(AttachSourceTest, shouldFailToCastAudioSource)
{
    triggerFailToCastAudioSource();
}

TEST_F(AttachSourceTest, shouldFailToCastVideoSource)
{
    triggerFailToCastVideoSource();
}

TEST_F(AttachSourceTest, shouldFailToCastDolbyVisionSource)
{
    triggerFailToCastDolbyVisionSource();
}
