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

#ifndef FIREBOLT_RIALTO_SERVER_GST_CAPABILITIES_H_
#define FIREBOLT_RIALTO_SERVER_GST_CAPABILITIES_H_

#include "IGstCapabilities.h"
#include "IGstWrapper.h"
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace firebolt::rialto::server
{
/**
 * @brief GstCapabilities factory class, returns a concrete implementation of GstCapabilities
 */
class GstCapabilitiesFactory : public IGstCapabilitiesFactory
{
public:
    /**
     * @brief Weak pointer to the singleton factory object.
     */
    static std::weak_ptr<IGstCapabilitiesFactory> m_factory;

    /**
     * @brief Creates a IGstCapabilities object.
     *
     * @retval the new gstreamer capabilities instance or null on error.
     */
    std::unique_ptr<IGstCapabilities> createGstCapabilities() override;
};

class GstCapabilities : public IGstCapabilities
{
public:
    explicit GstCapabilities(const std::shared_ptr<IGstWrapper> &gstWrapper);
    ~GstCapabilities() = default;

    GstCapabilities(const GstCapabilities &) = delete;
    GstCapabilities &operator=(const GstCapabilities &) = delete;
    GstCapabilities(GstCapabilities &&) = delete;
    GstCapabilities &operator=(GstCapabilities &&) = delete;

    /**
     * @brief Gets vector of mime types supported by gstreamer
     *
     * @retval vector of mime types supported by gstreamer
     */
    std::vector<std::string> getSupportedMimeTypes(MediaSourceType sourceType) override;

    /**
     * @brief Checks is \a mimeType is supported by gstreamer
     *
     * @retval True if mime type is supported by gstreamer
     */
    bool isMimeTypeSupported(const std::string &mimeType) override;

private:
    /**
     * @brief Sets list of supported mime types
     */
    void fillSupportedMimeTypes();

    /**
     * @brief Gets all unique caps from decoders' sink pads
     */
    std::vector<GstCaps *> getSupportedCapsFromDecoders();

    /**
     * @brief Appends all unique caps from parser->decoders chains' sink pads to \a supportedCaps
     */
    void appendSupportedCapsFromParserDecoderChains(std::vector<GstCaps *> &supportedCaps);

    /**
     * @brief Adds unique sink pads from \a padTemplates list to \a capsVector
     */
    void addAllUniqueSinkPadsCapsToVector(std::vector<GstCaps *> &capsVector, const GList *padTemplates);

    /**
     * @brief Checks if any src pad of parser can be connected to decoder
     *
     * @retval True if it's possible
     */
    bool canCreateParserDecoderChain(GstCaps *decoderCaps, const GList *parserPadTemplates);

    /**
     * @brief Checks if caps equal to \a caps is already in \a capsVector
     *
     * @retval True if there is equal caps
     */
    bool isCapsInVector(const std::vector<GstCaps *> &capsVector, GstCaps *caps) const;

    /**
     * @brief Maps vector of supported caps to set of mime types
     *
     * @retval Mime types
     */
    std::unordered_set<std::string> convertFromCapsVectorToMimeSet(std::vector<GstCaps *> &supportedCaps);

    /**
     * @brief Set of mime types which are supported by gstreamer
     */
    std::unordered_set<std::string> m_supportedMimeTypes;

    /**
     * @brief The gstreamer wrapper object.
     */
    std::shared_ptr<IGstWrapper> m_gstWrapper;
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_CAPABILITIES_H_
