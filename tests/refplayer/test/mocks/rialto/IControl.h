// Mock of the Rialto public control interface. Mirrors the subset of
// /usr/include/rialto/IControl.h that the reference player calls. Mock mode only.
#pragma once

#include <cstdint>
#include <memory>

namespace firebolt::rialto
{

enum class ApplicationState
{
    UNKNOWN,
    RUNNING,
    INACTIVE
};

class IControlClient
{
public:
    virtual ~IControlClient() = default;
    virtual void notifyApplicationState(ApplicationState state) = 0;
};

class IControl
{
public:
    virtual ~IControl() = default;
    virtual bool registerClient(std::weak_ptr<IControlClient> client, ApplicationState &appState) = 0;
};

// Registers as RUNNING and reports that state synchronously, as a healthy server does.
class MockControl final : public IControl
{
public:
    bool registerClient(std::weak_ptr<IControlClient> client, ApplicationState &appState) override
    {
        m_client = client.lock();
        appState = ApplicationState::RUNNING;
        if (m_client)
        {
            m_client->notifyApplicationState(appState);
        }
        return true;
    }

private:
    std::shared_ptr<IControlClient> m_client;
};

class IControlFactory
{
public:
    virtual ~IControlFactory() = default;
    static std::shared_ptr<IControlFactory> createFactory();
    virtual std::shared_ptr<IControl> createControl() const = 0;
};

class MockControlFactory final : public IControlFactory
{
public:
    std::shared_ptr<IControl> createControl() const override { return std::make_shared<MockControl>(); }
};

inline std::shared_ptr<IControlFactory> IControlFactory::createFactory()
{
    return std::make_shared<MockControlFactory>();
}

} // namespace firebolt::rialto
