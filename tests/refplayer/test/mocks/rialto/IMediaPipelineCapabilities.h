// Mock of the Rialto public pipeline capabilities interface. Mock mode only.
#pragma once

#include "IMediaPipeline.h"

#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto
{

class IMediaPipelineCapabilities
{
public:
    virtual ~IMediaPipelineCapabilities() = default;
    virtual std::vector<std::string> getSupportedMimeTypes(MediaSourceType sourceType) = 0;
    virtual bool isMimeTypeSupported(const std::string &mimeType) = 0;
    virtual std::vector<std::string> getSupportedProperties(MediaSourceType mediaType,
                                                            const std::vector<std::string> &propertyNames) = 0;
    virtual bool isVideoMaster(bool &isVideoMaster) = 0;
};

class MockMediaPipelineCapabilities final : public IMediaPipelineCapabilities
{
public:
    std::vector<std::string> getSupportedMimeTypes(MediaSourceType sourceType) override
    {
        if (sourceType == MediaSourceType::VIDEO)
        {
            return {"video/mp4"};
        }
        if (sourceType == MediaSourceType::AUDIO)
        {
            return {"audio/mp4"};
        }
        return {};
    }
    bool isMimeTypeSupported(const std::string &mimeType) override
    {
        return mimeType == "video/mp4" || mimeType == "audio/mp4";
    }
    std::vector<std::string> getSupportedProperties(MediaSourceType mediaType,
                                                    const std::vector<std::string> &propertyNames) override
    {
        (void)mediaType;
        return propertyNames;
    }
    bool isVideoMaster(bool &isVideoMaster) override
    {
        isVideoMaster = true;
        return true;
    }
};

class IMediaPipelineCapabilitiesFactory
{
public:
    virtual ~IMediaPipelineCapabilitiesFactory() = default;
    static std::shared_ptr<IMediaPipelineCapabilitiesFactory> createFactory();
    virtual std::unique_ptr<IMediaPipelineCapabilities> createMediaPipelineCapabilities() const = 0;
};

class MockMediaPipelineCapabilitiesFactory final : public IMediaPipelineCapabilitiesFactory
{
public:
    std::unique_ptr<IMediaPipelineCapabilities> createMediaPipelineCapabilities() const override
    {
        return std::make_unique<MockMediaPipelineCapabilities>();
    }
};

inline std::shared_ptr<IMediaPipelineCapabilitiesFactory> IMediaPipelineCapabilitiesFactory::createFactory()
{
    return std::make_shared<MockMediaPipelineCapabilitiesFactory>();
}

} // namespace firebolt::rialto
