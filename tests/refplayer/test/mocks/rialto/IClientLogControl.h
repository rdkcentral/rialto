// Mock of the Rialto public client log control interface. Mock mode only.
#pragma once

#include "IClientLogHandler.h"

#include <memory>

namespace firebolt::rialto
{

class IClientLogControl
{
public:
    virtual ~IClientLogControl() = default;
    virtual bool registerLogHandler(const std::shared_ptr<IClientLogHandler> &handler, bool ignoreLogLevels) = 0;
};

class MockClientLogControl final : public IClientLogControl
{
public:
    bool registerLogHandler(const std::shared_ptr<IClientLogHandler> &handler, bool ignoreLogLevels) override
    {
        (void)handler;
        (void)ignoreLogLevels;
        return true;
    }
};

class IClientLogControlFactory
{
public:
    virtual ~IClientLogControlFactory() = default;
    static std::shared_ptr<IClientLogControlFactory> createFactory();
    virtual IClientLogControl &createClientLogControl() = 0;
};

class MockClientLogControlFactory final : public IClientLogControlFactory
{
public:
    IClientLogControl &createClientLogControl() override
    {
        static MockClientLogControl control;
        return control;
    }
};

inline std::shared_ptr<IClientLogControlFactory> IClientLogControlFactory::createFactory()
{
    return std::make_shared<MockClientLogControlFactory>();
}

} // namespace firebolt::rialto
