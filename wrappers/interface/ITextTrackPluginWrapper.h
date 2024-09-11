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

#ifndef FIREBOLT_RIALTO_WRAPPERS_I_TEXT_TRACK_PLUGIN_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_I_TEXT_TRACK_PLUGIN_WRAPPER_H_

#include "ITextTrackWrapper.h"
#include <cstdint>
#include <memory>

namespace firebolt::rialto::wrappers
{
class ITextTrackPluginWrapper;

class ITextTrackPluginWrapperFactory
{
public:
    ITextTrackPluginWrapperFactory() = default;
    virtual ~ITextTrackPluginWrapperFactory() = default;

    /**
     * @brief Gets the ITextTrackPluginWrapperFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<ITextTrackPluginWrapperFactory> getFactory();

    /**
     * @brief Gets a ITextTrackPluginWrapper singleton object.
     *
     * @retval the wrapper instance or null on error.
     */
    virtual std::shared_ptr<ITextTrackPluginWrapper> getTextTrackPluginWrapper() = 0;
};

class ITextTrackPluginWrapper
{
public:
    ITextTrackPluginWrapper() = default;
    virtual ~ITextTrackPluginWrapper() = default;

    ITextTrackPluginWrapper(const ITextTrackPluginWrapper &) = delete;
    ITextTrackPluginWrapper &operator=(const ITextTrackPluginWrapper &) = delete;
    ITextTrackPluginWrapper(ITextTrackPluginWrapper &&) = delete;
    ITextTrackPluginWrapper &operator=(ITextTrackPluginWrapper &&) = delete;

    /**
     * @brief Opens connection with Text Track Plugin interface
     *
     * @retval the error code
     */
    virtual std::uint32_t open() = 0;

    /**
     * @brief Checks if Text Track plugin is operational
     *
     * @retval true, if plugin is operational
     */
    virtual bool isOperational() const = 0;

    /**
     * @brief Creates new ITextTrackWrapper instance
     *
     * @retval the ITextTrackWrapper instance or null on error.
     */
    virtual std::shared_ptr<ITextTrackWrapper> interface() = 0;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_I_TEXT_TRACK_PLUGIN_WRAPPER_H_
