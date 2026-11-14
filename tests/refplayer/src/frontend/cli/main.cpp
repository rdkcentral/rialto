#include "CliLog.h"
#include "ContractValidator.h"
#include "CliTestSupport.h"
#include "NegativeChecks.h"

#include "rialto_refplayer/IRefPlayer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <future>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace rialto_refplayer_cli
{

namespace
{

using rialto_refplayer::PlayerState;
using rialto_refplayer::Result;

// The negative suite has no per-call timeout of its own, so the whole suite gets one ceiling.
constexpr std::chrono::minutes kNegativeSuiteCeiling{5};

struct RunSummary
{
    bool passed{false};
    std::string finalState{"NONE"};
    int64_t events{0};
};

// Argument parsing and the event recorder are implemented in CliTestSupport.

// Runs a call that may block and waits for it with a ceiling. If the call has not returned
// in time, the process stops with _Exit: the call still holds library state, so unwinding
// or running destructors would be unsafe.
template <typename Fn>
auto runWithCeiling(const std::string &what, std::chrono::milliseconds ceiling, Fn fn) -> decltype(fn())
{
    using Value = decltype(fn());
    auto task = std::make_shared<std::packaged_task<Value()>>(std::move(fn));
    std::future<Value> done = task->get_future();
    std::thread([task] { (*task)(); }).detach();
    if (done.wait_for(ceiling) != std::future_status::ready)
    {
        cliLog(what + " did not return within " + std::to_string(ceiling.count()) + " ms; stopping");
        std::cout.flush();
        std::_Exit(1);
    }
    return done.get();
}

void logResult(const std::string &call, Result result)
{
    cliLog(call + " returned " + rialto_refplayer::toString(result));
}

bool awaitState(RecordingClient &client, PlayerState target, std::chrono::milliseconds ceiling)
{
    const std::string name = rialto_refplayer::toString(target);
    switch (client.waitForState(target, ceiling))
    {
    case RecordingClient::Wait::Reached:
        cliLog("Reached " + name);
        return true;
    case RecordingClient::Wait::Failed:
        cliLog("Playback failed before reaching " + name);
        return false;
    case RecordingClient::Wait::TimedOut:
        break;
    }
    cliLog("Timed out after " + std::to_string(ceiling.count()) + " ms waiting for " + name);
    return false;
}

bool awaitMetric(RecordingClient &client, MetricType expected, std::chrono::milliseconds ceiling)
{
    switch (client.waitForExpectedMetricOrPlaybackEnd(ceiling))
    {
    case RecordingClient::MetricWait::Received:
        cliLog(std::string("Received expected metric ") + metricTypeName(expected));
        return true;
    case RecordingClient::MetricWait::PlaybackEnded:
        cliLog(std::string("Playback ended without expected metric ") + metricTypeName(expected));
        return false;
    case RecordingClient::MetricWait::Failed:
        cliLog(std::string("Playback failed before expected metric ") + metricTypeName(expected));
        return false;
    case RecordingClient::MetricWait::TimedOut:
        cliLog("Timed out after " + std::to_string(ceiling.count()) + " ms waiting for metric " +
               metricTypeName(expected));
        return false;
    }
    return false;
}

// The probe checks, each on its own Rialto probe, under one ceiling.
bool runContractChecks(const CliOptions &options)
{
    ContractOptions contract;
    contract.mimeType = options.mimeType;
    contract.validateCapabilities = options.validateCapabilities;
    contract.useWebAudio = options.useWebAudio;
    contract.priority = options.priority;
    contract.volume = options.desiredVolume;

    return runWithCeiling("contract checks", std::chrono::milliseconds(options.timeoutMs), [contract] {
        if (!validateAdvancedMediaPipelineFeatures(contract))
        {
            cliLog("validateAdvancedMediaPipelineFeatures() failed");
            return false;
        }
        if (contract.validateCapabilities && !validateCapabilities(contract))
        {
            cliLog("validateCapabilities() failed");
            return false;
        }
        if (contract.useWebAudio && !validateWebAudio(contract))
        {
            cliLog("validateWebAudio() failed");
            return false;
        }
        if (!validateLogging())
        {
            cliLog("validateLogging() failed");
            return false;
        }
        return true;
    });
}

// Tunes one URL through the public API. The probe checks run while the media is playing,
// as they did when they shared the player.
RunSummary runPlayback(const CliOptions &options)
{
    RunSummary summary;
    const auto timeout = std::chrono::milliseconds(options.timeoutMs);
    const std::shared_ptr<rialto_refplayer::IRefPlayer> player = rialto_refplayer::createRefPlayer();
    if (!player)
    {
        cliLog("createRefPlayer() returned null");
        return summary;
    }

    // The library holds the client weakly, so it is kept alive here for the whole run. It is
    // set before initialize() so that no event is lost.
    const auto client = std::make_shared<RecordingClient>();
    player->setClient(client);
    if (options.expectedMetric)
    {
        client->expectMetric(*options.expectedMetric);
    }

    rialto_refplayer::Options libraryOptions{};
    libraryOptions.feederFaultControl = options.feederFaultControl;
    const Result initResult = runWithCeiling(
        "initialize()", timeout, [player, libraryOptions] { return player->initialize(libraryOptions); });
    logResult("initialize()", initResult);
    bool ok = initResult == Result::Ok;

    bool loaded = false;
    if (ok)
    {
        // The library autoplays by default. Without playback the tune stops at PREPARED.
        const Result loadResult = player->load(options.url, options.enablePlayback);
        logResult("load()", loadResult);
        loaded = loadResult == Result::Ok;
        ok = loaded;
    }
    if (ok)
    {
        const PlayerState tuned = options.enablePlayback ? PlayerState::Playing : PlayerState::Prepared;
        ok = awaitState(*client, tuned, timeout);
    }
    if (ok && options.seekNs)
    {
        // The library seeks in milliseconds; --seek is given in nanoseconds.
        const int64_t positionMs = *options.seekNs / 1000000;
        const Result seekResult = player->seek(positionMs);
        logResult("seek(" + std::to_string(positionMs) + " ms)", seekResult);
        ok = seekResult == Result::Ok;
    }
    if (ok && options.desiredVolume)
    {
        const Result volumeResult = player->setVolume(*options.desiredVolume);
        if (volumeResult == Result::NotSupported)
        {
            cliLog("setVolume() is not supported by this library build; the value is validated on a probe pipeline");
        }
        else
        {
            logResult("setVolume()", volumeResult);
            ok = volumeResult == Result::Ok;
        }
    }
    if (ok)
    {
        ok = runContractChecks(options);
    }
    if (ok && options.expectedMetric)
    {
        ok = awaitMetric(*client, *options.expectedMetric, timeout);
    }
    if (ok && options.enablePlayback && !options.isLive)
    {
        // The media's own length, when the library knows it, is added to the ceiling.
        const int64_t mediaMs = player->durationMs();
        cliLog("Waiting for playback to complete");
        ok = awaitState(*client, PlayerState::Complete,
                        timeout + std::chrono::milliseconds(mediaMs > 0 ? mediaMs : 0));
    }

    summary.finalState = rialto_refplayer::toString(player->state());
    if (loaded)
    {
        const Result stopResult = player->stop();
        logResult("stop()", stopResult);
        if (ok)
        {
            ok = stopResult == Result::Ok && awaitState(*client, PlayerState::Stopped, timeout);
        }
    }
    runWithCeiling("destroy()", timeout, [player] { player->destroy(); });

    summary.events = client->eventCount();
    summary.passed = ok;
    cliLog(ok ? "Contract validation passed" : "Contract validation failed");
    return summary;
}

} // namespace

int runCli(int argc, char **argv)
{
    CliOptions options;
    const ParseStatus parsed = parseArguments(argc, argv, options);
    if (parsed == ParseStatus::Help)
    {
        return 0;
    }
    if (parsed == ParseStatus::Error)
    {
        return 1;
    }

    RunSummary summary;
    if (options.url.empty())
    {
        cliLog("No URL provided; using a synthetic contract-only validation flow.");
        summary.passed = runContractChecks(options);
        cliLog(summary.passed ? "Contract validation passed" : "Contract validation failed");
    }
    else
    {
        summary = runPlayback(options);
    }

    if (summary.passed && options.negativeRuns)
    {
        const int status =
            runWithCeiling("negative suite", kNegativeSuiteCeiling, [] { return runNegativeChecks(); });
        summary.passed = status == 0;
    }

    cliLog("summary: final state=" + summary.finalState + ", events=" + std::to_string(summary.events) +
           ", result=" + (summary.passed ? "PASS" : "FAIL"));
    return summary.passed ? 0 : 1;
}

} // namespace rialto_refplayer_cli

int main(int argc, char **argv)
{
    return rialto_refplayer_cli::runCli(argc, argv);
}
