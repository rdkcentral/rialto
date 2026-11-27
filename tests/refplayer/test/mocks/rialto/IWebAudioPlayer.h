// Mock of the Rialto public web audio interface. Mock mode only.
#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace firebolt::rialto
{

enum class WebAudioPlayerState
{
    UNKNOWN,
    IDLE,
    PLAYING,
    PAUSED,
    END_OF_STREAM,
    FAILURE
};

struct WebAudioPcmConfig
{
    uint32_t rate{48000};
    uint32_t channels{2};
    uint32_t sampleSize{16};
    bool isBigEndian{false};
    bool isSigned{true};
    bool isFloat{false};
};

union WebAudioConfig
{
    // User-provided, because WebAudioPcmConfig's member initialisers make the implicit one deleted.
    WebAudioConfig() : pcm{} {}
    WebAudioPcmConfig pcm;
};

struct WebAudioShmInfo
{
    uint32_t offsetMain{0};
    uint32_t lengthMain{0};
    uint32_t offsetWrap{0};
    uint32_t lengthWrap{0};
};

class IWebAudioPlayerClient
{
public:
    virtual ~IWebAudioPlayerClient() = default;
    virtual void notifyState(WebAudioPlayerState state) = 0;
};

class IWebAudioPlayer
{
public:
    virtual ~IWebAudioPlayer() = default;
    virtual bool play() = 0;
    virtual bool pause() = 0;
    virtual bool setEos() = 0;
    virtual bool getBufferAvailable(uint32_t &availableFrames, std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo) = 0;
    virtual bool getBufferDelay(uint32_t &delayFrames) = 0;
    virtual bool writeBuffer(const uint32_t numberOfFrames, void *data) = 0;
    virtual bool getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay) = 0;
    virtual bool setVolume(double volume) = 0;
    virtual bool getVolume(double &volume) = 0;
    virtual std::weak_ptr<IWebAudioPlayerClient> getClient() = 0;
};

class IWebAudioPlayerFactory
{
public:
    virtual ~IWebAudioPlayerFactory() = default;
    static std::shared_ptr<IWebAudioPlayerFactory> createFactory();
    virtual std::unique_ptr<IWebAudioPlayer> createWebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client,
                                                                  const std::string &audioMimeType,
                                                                  const uint32_t priority,
                                                                  std::weak_ptr<const WebAudioConfig> config) const = 0;
};

class MockWebAudioPlayer final : public IWebAudioPlayer
{
public:
    explicit MockWebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client = {}) : m_client(client) {}

    bool play() override { return true; }
    bool pause() override { return true; }
    bool setEos() override { return true; }
    bool getBufferAvailable(uint32_t &availableFrames, std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo) override
    {
        webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
        webAudioShmInfo->lengthMain = 4096;
        availableFrames = 512;
        return true;
    }
    bool getBufferDelay(uint32_t &delayFrames) override
    {
        delayFrames = 0U;
        return true;
    }
    bool writeBuffer(const uint32_t numberOfFrames, void *data) override
    {
        (void)numberOfFrames;
        (void)data;
        return true;
    }
    bool getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay) override
    {
        preferredFrames = 256;
        maximumFrames = 4096;
        supportDeferredPlay = true;
        return true;
    }
    bool setVolume(double volume) override
    {
        (void)volume;
        return true;
    }
    bool getVolume(double &volume) override
    {
        volume = 0.5;
        return true;
    }
    std::weak_ptr<IWebAudioPlayerClient> getClient() override { return m_client; }

private:
    std::weak_ptr<IWebAudioPlayerClient> m_client;
};

class MockWebAudioPlayerFactory final : public IWebAudioPlayerFactory
{
public:
    std::unique_ptr<IWebAudioPlayer> createWebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client,
                                                          const std::string &audioMimeType, const uint32_t priority,
                                                          std::weak_ptr<const WebAudioConfig> config) const override
    {
        (void)audioMimeType;
        (void)priority;
        (void)config;
        return std::make_unique<MockWebAudioPlayer>(std::move(client));
    }
};

inline std::shared_ptr<IWebAudioPlayerFactory> IWebAudioPlayerFactory::createFactory()
{
    return std::make_shared<MockWebAudioPlayerFactory>();
}

} // namespace firebolt::rialto
