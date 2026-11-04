#pragma once

#include <iostream>
#include <string>

namespace rialto_refplayer_cli
{

// CLI output uses the library's prefix. The whole line is built first so that one
// insertion reaches stdout.
inline void cliLog(const std::string &message)
{
    const std::string line = "[rialto-ref-player] " + message;
    std::cout << line << std::endl;
}

} // namespace rialto_refplayer_cli
