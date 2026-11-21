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

#ifndef FIREBOLT_RIALTO_CLIENT_FRAME_IMPORTER_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_FRAME_IMPORTER_MOCK_H_

#include "IFrameImporter.h"
#include <gmock/gmock.h>
#include <memory>
#include <vector>

namespace firebolt::rialto::client
{
class FrameImporterMock : public IFrameImporter
{
public:
    MOCK_METHOD(std::shared_ptr<IImportedImage>, importFrame,
                (const CaptureFrameDescription &description, const std::vector<CaptureObjectFd> &fds, bool &fdsConsumed),
                (override));
    MOCK_METHOD(void, retire, (const std::shared_ptr<IImportedImage> &image), (override));
    MOCK_METHOD(void, drain, (), (override));
};

class ImportedImageMock : public IImportedImage
{
public:
    MOCK_METHOD(std::uint32_t, textureTarget, (), (const, override));
    MOCK_METHOD(const std::vector<std::uint32_t> &, textureIds, (), (const, override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_FRAME_IMPORTER_MOCK_H_
