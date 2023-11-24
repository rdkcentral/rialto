/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "ConfigReaderFactory.h"
#include "ConfigReader.h"
#include "FileReaderFactory.h"
#include "IJsonCppWrapperFactory.h"

namespace rialto::servermanager::service
{
std::shared_ptr<IConfigReader> ConfigReaderFactory::createConfigReader(const std::string &filePath) const
{
    std::unique_ptr<firebolt::rialto::wrappers::IJsonCppWrapperFactory> jsonCppWrapperFactory =
        firebolt::rialto::wrappers::IJsonCppWrapperFactory::createFactory();
    std::shared_ptr<firebolt::rialto::wrappers::IJsonCppWrapper> jsonWrapper =
        jsonCppWrapperFactory->createJsonCppWrapper();

    std::unique_ptr<IFileReaderFactory> fileReaderFactory = std::make_unique<FileReaderFactory>();
    std::shared_ptr<IFileReader> fileReader = fileReaderFactory->createFileReader(filePath);

    return std::make_shared<ConfigReader>(jsonWrapper, fileReader);
}
} // namespace rialto::servermanager::service
