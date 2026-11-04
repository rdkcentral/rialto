#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace rialto_refplayer
{

// Serialised, levelled logger. Every line carries a timestamp and a thread id, and
// the sink is called under one mutex, so lines from the worker, the Rialto IPC
// threads and the feeder never interleave. There is no global instance: each player
// owns one. Client-library logs are forwarded to the most recent player's logger.
class Logger : public std::enable_shared_from_this<Logger>
{
public:
    enum class Level
    {
        Fatal,
        Error,
        Warning,
        Milestone,
        Info,
        Debug
    };

    using Sink = std::function<void(const std::string &line)>;

    Logger();                 // stdout sink, Info level
    explicit Logger(Sink sink);

    void setLevel(Level level);

    void log(Level level, const std::string &message);
    void error(const std::string &message) { log(Level::Error, message); }
    void warning(const std::string &message) { log(Level::Warning, message); }
    void info(const std::string &message) { log(Level::Info, message); }
    void debug(const std::string &message) { log(Level::Debug, message); }

    // Tune-time marker, emitted once per load at the first frame. The format is
    // the consumed interface for the test app's tune-time panel; keep it stable.
    //   [rialto-ref-player] TUNE_TIME_MS=<n>
    void tuneTime(int64_t milliseconds);

    // Forwards Rialto client-library logs to this logger. Requires a shared_ptr owner.
    // The most recent call wins.
    void attachClientLog();

private:
    std::mutex m_mutex;
    Level m_level{Level::Info};
    Sink m_sink;
};

const char *toString(Logger::Level level);

} // namespace rialto_refplayer
