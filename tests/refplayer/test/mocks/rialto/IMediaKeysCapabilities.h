// Mock of the Rialto public media keys capabilities interface. Mock mode only.
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto
{

class IMediaKeysCapabilities
{
public:
    virtual ~IMediaKeysCapabilities() = default;
    virtual std::vector<std::string> getSupportedKeySystems() = 0;
    virtual bool supportsKeySystem(const std::string &keySystem) = 0;
    virtual bool getSupportedKeySystemVersion(const std::string &keySystem, std::string &version) = 0;
    virtual bool isServerCertificateSupported(const std::string &keySystem) = 0;
};

class MockMediaKeysCapabilities final : public IMediaKeysCapabilities
{
public:
    std::vector<std::string> getSupportedKeySystems() override
    {
        return {"com.widevine.alpha", "com.microsoft.playready"};
    }
    bool supportsKeySystem(const std::string &keySystem) override { return keySystem == "com.widevine.alpha"; }
    bool getSupportedKeySystemVersion(const std::string &keySystem, std::string &version) override
    {
        (void)keySystem;
        version = "1.0";
        return true;
    }
    bool isServerCertificateSupported(const std::string &keySystem) override
    {
        return keySystem == "com.widevine.alpha";
    }
};

class IMediaKeysCapabilitiesFactory
{
public:
    virtual ~IMediaKeysCapabilitiesFactory() = default;
    static std::shared_ptr<IMediaKeysCapabilitiesFactory> createFactory();
    virtual std::shared_ptr<IMediaKeysCapabilities> getMediaKeysCapabilities() const = 0;
};

class MockMediaKeysCapabilitiesFactory final : public IMediaKeysCapabilitiesFactory
{
public:
    std::shared_ptr<IMediaKeysCapabilities> getMediaKeysCapabilities() const override
    {
        return std::make_shared<MockMediaKeysCapabilities>();
    }
};

inline std::shared_ptr<IMediaKeysCapabilitiesFactory> IMediaKeysCapabilitiesFactory::createFactory()
{
    return std::make_shared<MockMediaKeysCapabilitiesFactory>();
}

} // namespace firebolt::rialto
