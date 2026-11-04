#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace rialto_refplayer_cli
{

struct ContractOptions
{
    std::string mimeType{"video/mp4"};
    bool validateCapabilities{true};
    bool useWebAudio{false};
    uint32_t priority{1};
    // --set-volume. It is set and read back on the probe; without it, 0.7 is used.
    std::optional<double> volume;
};

// Each check runs on its own probe made through the Rialto factories, not on the
// player. Each returns false when a call or a read-back fails.
bool validateAdvancedMediaPipelineFeatures(const ContractOptions &options);
bool validateCapabilities(const ContractOptions &options);
bool validateWebAudio(const ContractOptions &options);
bool validateLogging();

} // namespace rialto_refplayer_cli
