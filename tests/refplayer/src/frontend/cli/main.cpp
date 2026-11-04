#include "CliLog.h"
#include "ContractValidator.h"
#include "NegativeChecks.h"

#include "rialto_refplayer/IRefPlayer.h"

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

namespace rialto_refplayer_cli
{

namespace
{

using rialto_refplayer::EventType;
using rialto_refplayer::PlayerState;
using rialto_refplayer::Result;

constexpr int64_t kDefaultTimeoutMs = 30000;
// The negative suite has no per-call timeout of its own, so the whole suite gets one ceiling.
constexpr std::chrono::minutes kNegativeSuiteCeiling{5};

struct CliOptions
{
    std::string url;
    std::string mimeType{"video/mp4"};
    bool isLive{false};
    bool validateCapabilities{true};
    bool useWebAudio{false};
    bool enablePlayback{true};
    bool negativeRuns{false};
    std::optional<double> desiredVolume;
    std::optional<int64_t> seekNs;
    uint32_t priority{1};
    int64_t timeoutMs{kDefaultTimeoutMs};
};

enum class ParseStatus
{
    Run,
    Help,
    Error
};

struct RunSummary
{
    bool passed{false};
    std::string finalState{"NONE"};
    int64_t events{0};
};

void printUsage(std::ostream &out)
{
    out << "Usage: rialto_reference_player [options]\n"
           "  --url <url>                 Media URL; a local file:// .mp4 (H.264) is demuxed and fed over MSE\n"
           "  --mime <mime>               MIME type for the capability probes (default: video/mp4); phase 1 plays\n"
           "                              only video/mp4 MP4 files, so playback ignores this value\n"
           "  --live                      CLI only: do not wait for end of stream after PLAYING\n"
           "  --web-audio                 Also create an IWebAudioPlayer and validate the PCM API\n"
           "  --negative-runs             Run negative API checks after contract validation\n"
           "  --set-volume <v>            Volume in [0.0, 1.0]; set and read back on a probe pipeline\n"
           "  --seek <ns>                 Seek to a position in nanoseconds; needs --url, not --no-playback\n"
           "  --priority <n>              Audio player priority (default: 1)\n"
           "  --no-playback               Validate contract without calling play()\n"
           "  --no-validate-capabilities  Skip capability tests\n"
           "  --timeout-ms <ms>           Ceiling for each wait on a player state (default: 30000)\n"
           "  --help                      Print this help text\n";
}

ParseStatus argumentError(const std::string &message)
{
    std::cerr << "[rialto-ref-player] error: " << message << std::endl;
    return ParseStatus::Error;
}

bool parseDouble(const std::string &text, double &value)
{
    try
    {
        std::size_t used = 0;
        value = std::stod(text, &used);
        return used == text.size();
    }
    catch (const std::logic_error &)
    {
        return false;
    }
}

bool parseInt64(const std::string &text, int64_t &value)
{
    try
    {
        std::size_t used = 0;
        value = std::stoll(text, &used);
        return used == text.size();
    }
    catch (const std::logic_error &)
    {
        return false;
    }
}

ParseStatus parseArguments(int argc, char **argv, CliOptions &options)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--help")
        {
            printUsage(std::cout);
            return ParseStatus::Help;
        }
        if (arg == "--live")
        {
            options.isLive = true;
        }
        else if (arg == "--no-validate-capabilities")
        {
            options.validateCapabilities = false;
        }
        else if (arg == "--web-audio")
        {
            options.useWebAudio = true;
        }
        else if (arg == "--no-playback")
        {
            options.enablePlayback = false;
        }
        else if (arg == "--negative-runs")
        {
            options.negativeRuns = true;
        }
        else if (arg == "--url" || arg == "--mime" || arg == "--set-volume" || arg == "--seek" ||
                 arg == "--priority" || arg == "--timeout-ms")
        {
            if (i + 1 >= argc)
            {
                return argumentError("missing value for " + arg);
            }
            const std::string value = argv[++i];
            if (arg == "--url")
            {
                options.url = value;
            }
            else if (arg == "--mime")
            {
                options.mimeType = value;
            }
            else if (arg == "--set-volume")
            {
                double volume = 0.0;
                if (!parseDouble(value, volume) || !(volume >= 0.0 && volume <= 1.0))
                {
                    return argumentError("--set-volume must be a number in [0.0, 1.0]");
                }
                options.desiredVolume = volume;
            }
            else if (arg == "--seek")
            {
                int64_t ns = 0;
                if (!parseInt64(value, ns) || ns < 0)
                {
                    return argumentError("--seek must be a non-negative integer in nanoseconds");
                }
                options.seekNs = ns;
            }
            else if (arg == "--priority")
            {
                int64_t priority = 0;
                if (!parseInt64(value, priority) || priority < 0 ||
                    priority > static_cast<int64_t>(std::numeric_limits<uint32_t>::max()))
                {
                    return argumentError("--priority must be a non-negative integer");
                }
                options.priority = static_cast<uint32_t>(priority);
            }
            else
            {
                int64_t timeout = 0;
                if (!parseInt64(value, timeout) || timeout <= 0)
                {
                    return argumentError("--timeout-ms must be a positive integer");
                }
                options.timeoutMs = timeout;
            }
        }
        else
        {
            return argumentError("unknown option " + arg + " (see --help)");
        }
    }

    if (options.seekNs && options.url.empty())
    {
        return argumentError("--seek needs --url");
    }
    if (options.seekNs && !options.enablePlayback)
    {
        return argumentError("--seek cannot be combined with --no-playback");
    }
    return ParseStatus::Run;
}

// Receives events on any library thread and lets the driver wait for a state.
class RecordingClient final : public rialto_refplayer::IRefPlayerClient
{
public:
    enum class Wait
    {
        Reached,
        Failed,
        TimedOut
    };

    void onEvent(const rialto_refplayer::Event &event) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_events;
        if (event.type == EventType::StateChanged)
        {
            const auto index = static_cast<std::size_t>(event.state);
            if (index < m_seen.size())
            {
                m_seen[index] = true;
            }
            cliLog(std::string("state: ") + rialto_refplayer::toString(event.state));
        }
        else if (event.type == EventType::Failed)
        {
            m_failed = true;
            cliLog("playbackFailed: " + event.description);
        }
        m_cv.notify_all();
    }

    // A state counts once it has been seen, so a short clip that passes PLAYING before the
    // wait starts is still reached. ERROR and a Failed event end the wait early.
    Wait waitForState(PlayerState target, std::chrono::milliseconds ceiling)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait_for(lock, ceiling,
                      [this, target] { return seen(target) || seen(PlayerState::Error) || m_failed; });
        if (seen(target))
        {
            return Wait::Reached;
        }
        return (m_failed || seen(PlayerState::Error)) ? Wait::Failed : Wait::TimedOut;
    }

    int64_t eventCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_events;
    }

private:
    // Requires m_mutex to be held.
    bool seen(PlayerState state) const
    {
        const auto index = static_cast<std::size_t>(state);
        return index < m_seen.size() && m_seen[index];
    }

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::array<bool, 16> m_seen{}; // indexed by PlayerState; the library has 15 states
    bool m_failed{false};
    int64_t m_events{0};
};

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

    const rialto_refplayer::Options libraryOptions{};
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
