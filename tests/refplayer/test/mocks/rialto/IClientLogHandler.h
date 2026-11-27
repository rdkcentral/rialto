// Mock of the Rialto public client log handler interface. Mock mode only.
#pragma once

#include <string>

namespace firebolt::rialto
{

class IClientLogHandler
{
public:
    enum class Level
    {
        Fatal,
        Error,
        Warning,
        Milestone,
        Info,
        Debug,
        External
    };
    virtual ~IClientLogHandler() = default;
    virtual void log(Level level, const std::string &file, int line, const std::string &function,
                     const std::string &message) = 0;
};

} // namespace firebolt::rialto
