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

#include "MediaFrameWriterV2.h"
#include <cstdlib>
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::common;

namespace
{
const uint32_t kMaxMetadataBytes = 6;
const uint32_t kMaxMediaBytes = 4;
} // namespace

class RialtoPlayerCommonCreateMediaFrameWriterV2Test : public ::testing::Test
{
protected:
    std::shared_ptr<IMediaFrameWriterFactory> m_mediaFrameWriterFactory;

    uint8_t m_shmBuffer[kMaxMetadataBytes + kMaxMediaBytes] = {0};
    std::shared_ptr<MediaPlayerShmInfo> m_shmInfo;

    virtual void SetUp()
    {
        m_mediaFrameWriterFactory = IMediaFrameWriterFactory::getFactory();

        // init shm info
        m_shmInfo = std::make_shared<MediaPlayerShmInfo>();
        m_shmInfo->maxMetadataBytes = kMaxMetadataBytes;
        m_shmInfo->metadataOffset = 0;
        m_shmInfo->mediaDataOffset = kMaxMetadataBytes;
        m_shmInfo->maxMediaBytes = kMaxMediaBytes;
    }

    virtual void TearDown() { m_mediaFrameWriterFactory.reset(); }

    uint32_t readLEUint32(const uint8_t *buffer)
    {
        uint32_t value = buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0];
        return value;
    }
};

/**
 * Test that an MediaFrameWriterV2 object can be created successfully.
 */
TEST_F(RialtoPlayerCommonCreateMediaFrameWriterV2Test, CreateMediaFrameWriter)
{
    std::unique_ptr<IMediaFrameWriter> mediaFrameWriter =
        m_mediaFrameWriterFactory->createFrameWriter(m_shmBuffer, m_shmInfo);

    EXPECT_NE(mediaFrameWriter, nullptr);
    EXPECT_NO_THROW(dynamic_cast<MediaFrameWriterV2 &>(*mediaFrameWriter));
}

/**
 * Test that an MediaFrameWriterV2 writes the version to the shared buffer and zeroes the rest of the metadata.
 */
TEST_F(RialtoPlayerCommonCreateMediaFrameWriterV2Test, CheckSharedBufferData)
{
    std::unique_ptr<IMediaFrameWriter> mediaFrameWriter =
        m_mediaFrameWriterFactory->createFrameWriter(m_shmBuffer, m_shmInfo);
    EXPECT_NE(mediaFrameWriter, nullptr);

    // Version should be set to 2
    EXPECT_EQ(readLEUint32(m_shmBuffer), 2U);

    // Rest of the data should be zeroed
    constexpr size_t kZeroedMemSize{kMaxMetadataBytes + kMaxMediaBytes - VERSION_SIZE_BYTES};
    uint8_t zeroedMem[kZeroedMemSize] = {0};
    EXPECT_EQ(memcmp(zeroedMem, m_shmBuffer + VERSION_SIZE_BYTES, kZeroedMemSize), 0);
}

/**
 * Test that an MediaFrameWriterV2 writes data at the given offset.
 */
TEST_F(RialtoPlayerCommonCreateMediaFrameWriterV2Test, Offset)
{
    constexpr int kOffset{2};
    m_shmInfo->metadataOffset += kOffset;
    m_shmInfo->mediaDataOffset += kOffset;
    m_shmInfo->maxMediaBytes -= kOffset;

    std::unique_ptr<IMediaFrameWriter> mediaFrameWriter =
        m_mediaFrameWriterFactory->createFrameWriter(m_shmBuffer, m_shmInfo);
    EXPECT_NE(mediaFrameWriter, nullptr);

    // Version should be set to 2
    EXPECT_EQ(readLEUint32(m_shmBuffer + m_shmInfo->metadataOffset), 2U);

    // Rest of the metadata should be zeroed
    constexpr size_t kZeroedMemSize{kMaxMetadataBytes + kMaxMediaBytes - VERSION_SIZE_BYTES - kOffset};
    uint8_t zeroedMem[kZeroedMemSize] = {0};
    EXPECT_EQ(memcmp(zeroedMem, m_shmBuffer + VERSION_SIZE_BYTES + kOffset, kZeroedMemSize), 0);
}

/**
 * Test that an MediaFrameWriterV2 is created, when wrong version of metadata is set in env variable
 */
TEST_F(RialtoPlayerCommonCreateMediaFrameWriterV2Test, CreateMediaFrameWriterWhenEnvVariableVersionIsTooBig)
{
    m_mediaFrameWriterFactory.reset();
    setenv("RIALTO_METADATA_VERSION", "5", 1);
    m_mediaFrameWriterFactory = IMediaFrameWriterFactory::getFactory();
    std::unique_ptr<IMediaFrameWriter> mediaFrameWriter =
        m_mediaFrameWriterFactory->createFrameWriter(m_shmBuffer, m_shmInfo);

    EXPECT_NE(mediaFrameWriter, nullptr);
    EXPECT_NO_THROW(dynamic_cast<MediaFrameWriterV2 &>(*mediaFrameWriter));
    unsetenv("RIALTO_METADATA_VERSION");
}

/**
 * Test that an MediaFrameWriterV2 is created, when invalid version of metadata is set in env variable
 */
TEST_F(RialtoPlayerCommonCreateMediaFrameWriterV2Test, CreateMediaFrameWriterWhenEnvVariableVersionIsInvalid)
{
    m_mediaFrameWriterFactory.reset();
    setenv("RIALTO_METADATA_VERSION", "HELLO", 1);
    m_mediaFrameWriterFactory = IMediaFrameWriterFactory::getFactory();
    std::unique_ptr<IMediaFrameWriter> mediaFrameWriter =
        m_mediaFrameWriterFactory->createFrameWriter(m_shmBuffer, m_shmInfo);

    EXPECT_NE(mediaFrameWriter, nullptr);
    EXPECT_NO_THROW(dynamic_cast<MediaFrameWriterV2 &>(*mediaFrameWriter));
    unsetenv("RIALTO_METADATA_VERSION");
}
