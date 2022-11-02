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

#include "MediaFrameWriterV1.h"
#include <cstdlib>
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::common;

const uint32_t MAX_MEDIA_BYTES = 10;
const uint32_t MAX_METADATA_BYTES = VERSION_SIZE_BYTES + 24 * METADATA_V1_SIZE_PER_FRAME_BYTES;

class RialtoPlayerCommonCreateMediaFrameWriterV1Test : public ::testing::Test
{
protected:
    std::shared_ptr<IMediaFrameWriterFactory> m_mediaFrameWriterFactory;

    // Create a buffer that can hold 24 frames of metadata amd 10bytes of media data
    uint8_t m_shmBuffer[MAX_METADATA_BYTES + MAX_MEDIA_BYTES] = {0};
    std::shared_ptr<ShmInfo> m_shmInfo;

    virtual void SetUp()
    {
        setenv("RIALTO_METADATA_VERSION", "1", 1);
        m_mediaFrameWriterFactory = IMediaFrameWriterFactory::getFactory();

        // init shm info
        m_shmInfo = std::make_shared<ShmInfo>();
        m_shmInfo->maxMetadataBytes = MAX_METADATA_BYTES;
        m_shmInfo->metadataOffset = 0;
        m_shmInfo->mediaDataOffset = MAX_METADATA_BYTES;
        m_shmInfo->maxMediaBytes = MAX_MEDIA_BYTES;
    }

    virtual void TearDown()
    {
        m_mediaFrameWriterFactory.reset();
        unsetenv("RIALTO_METADATA_VERSION");
    }

    uint32_t readLEUint32(const uint8_t *buffer)
    {
        uint32_t value = buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
        return value;
    }
};

/**
 * Test that an MediaFrameWriterV1 object can be created successfully.
 */
TEST_F(RialtoPlayerCommonCreateMediaFrameWriterV1Test, CreateMediaFrameWriter)
{
    std::unique_ptr<IMediaFrameWriter> mediaFrameWriter =
        m_mediaFrameWriterFactory->createFrameWriter(m_shmBuffer, m_shmInfo);

    EXPECT_NE(mediaFrameWriter, nullptr);
    EXPECT_NO_THROW(dynamic_cast<MediaFrameWriterV1 &>(*mediaFrameWriter));
}

/**
 * Test that an MediaFrameWriterV1 writes the version to the shared buffer and zeros the rest of the metadata.
 */
TEST_F(RialtoPlayerCommonCreateMediaFrameWriterV1Test, CheckSharedBufferData)
{
    std::unique_ptr<IMediaFrameWriter> mediaFrameWriter =
        m_mediaFrameWriterFactory->createFrameWriter(m_shmBuffer, m_shmInfo);
    EXPECT_NE(mediaFrameWriter, nullptr);

    // Version should be set to 1
    EXPECT_EQ(readLEUint32(m_shmBuffer), 1U);

    // Rest of the metadata should be zeroed
    uint8_t zeroedMem[MAX_METADATA_BYTES] = {0};
    EXPECT_EQ(memcmp(zeroedMem, m_shmBuffer + VERSION_SIZE_BYTES, MAX_METADATA_BYTES), 0);
}

/**
 * Test that an MediaFrameWriterV1 writes data at the given offset.
 */
TEST_F(RialtoPlayerCommonCreateMediaFrameWriterV1Test, Offset)
{
    m_shmInfo->metadataOffset += 3;
    m_shmInfo->mediaDataOffset += 3;

    std::unique_ptr<IMediaFrameWriter> mediaFrameWriter =
        m_mediaFrameWriterFactory->createFrameWriter(m_shmBuffer, m_shmInfo);
    EXPECT_NE(mediaFrameWriter, nullptr);

    // Version should be set to 1
    EXPECT_EQ(readLEUint32(m_shmBuffer + m_shmInfo->metadataOffset), 1U);

    // Rest of the metadata should be zeroed
    uint8_t zeroedMem[MAX_METADATA_BYTES] = {0};
    EXPECT_EQ(memcmp(zeroedMem, m_shmBuffer + VERSION_SIZE_BYTES + m_shmInfo->metadataOffset, MAX_METADATA_BYTES), 0);
}
