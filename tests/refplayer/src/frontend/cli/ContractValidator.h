#pragma once

#include "IMediaPipeline.h"
#include "rialto_refplayer/IRefPlayer.h"

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
    // --set-volume. It is set and read back on the player's pipeline; without it, 0.7 is used.
    std::optional<double> volume;
};

// The two pipeline groups run on the pipeline that the player is using, through
// IRefPlayer::runOnPipeline(), so the calls act on the media that is being played.
// Before play: the calls that are only valid before PLAYING (PREPARED).
bool validatePipelineBeforePlay(firebolt::rialto::IMediaPipeline &pipeline,
                                const rialto_refplayer::PipelineSources &sources);
// While PLAYING: property set and read-back, duration, and a position that advances.
bool validatePipelineWhilePlaying(firebolt::rialto::IMediaPipeline &pipeline,
                                  const rialto_refplayer::PipelineSources &sources, const ContractOptions &options);

// The remaining groups do not involve the player's pipeline. Each makes its own Rialto
// objects and returns false when a call fails.
bool validateCapabilities(const ContractOptions &options);
bool validateWebAudio(const ContractOptions &options);
bool validateLogging();

} // namespace rialto_refplayer_cli
