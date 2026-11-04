#include "Logger.h"

#include "IClientLogControl.h"
#include "IClientLogHandler.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>
#include <utility>

namespace rialto_refplayer
{
namespace
{

constexpr const char *kPrefix = "[rialto-ref-player]";

// Forwards Rialto client-library logs to whichever player logger attached last.
// Registered with the Rialto client once per process; it holds only a weak
// reference, so it never keeps a torn-down player alive.
class ClientLogForwarder final : public firebolt::rialto::IClientLogHandler
{
public:
    void setTarget(std::weak_ptr<Logger> target)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_target = std::move(target);
    }

    void log(Level level, const std::string &file, int line, const std::string &function,
             const std::string &message) override
    {
        std::shared_ptr<Logger> target;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            target = m_target.lock();
        }
        if (!target)
        {
            return;
        }
        (void)level;
        target->debug("rialto " + file + ":" + std::to_string(line) + " " + function + ": " + message);
    }

private:
    std::mutex m_mutex;
    std::weak_ptr<Logger> m_target;
};

std::shared_ptr<ClientLogForwarder> clientLogForwarder()
{
    static std::shared_ptr<ClientLogForwarder> forwarder = std::make_shared<ClientLogForwarder>();
    return forwarder;
}

std::string timestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    const std::time_t seconds = static_cast<std::time_t>(millis / 1000);
    std::tm local{};
    localtime_r(&seconds, &local);
    std::ostringstream out;
    out << std::put_time(&local, "%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << (millis % 1000);
    return out.str();
}

std::string threadTag()
{
    std::ostringstream out;
    out << "tid=" << std::hex << (std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xffffU);
    return out.str();
}

} // namespace

const char *toString(Logger::Level level)
{
    switch (level)
    {
    case Logger::Level::Fatal: return "FATAL";
    case Logger::Level::Error: return "ERROR";
    case Logger::Level::Warning: return "WARN";
    case Logger::Level::Milestone: return "MILESTONE";
    case Logger::Level::Info: return "INFO";
    case Logger::Level::Debug: return "DEBUG";
    }
    return "UNKNOWN";
}

Logger::Logger()
    : m_sink([](const std::string &line) { std::cout << line << std::endl; })
{
}

Logger::Logger(Sink sink) : m_sink(std::move(sink))
{
}

void Logger::setLevel(Level level)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

void Logger::log(Level level, const std::string &message)
{
    std::string line = std::string(kPrefix) + " " + timestamp() + " " + toString(level) + " " + threadTag() + " " +
                       message;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (static_cast<int>(level) > static_cast<int>(m_level))
    {
        return;
    }
    m_sink(line);
}

void Logger::tuneTime(int64_t milliseconds)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sink(std::string(kPrefix) + " TUNE_TIME_MS=" + std::to_string(milliseconds));
}

void Logger::attachClientLog()
{
    const auto forwarder = clientLogForwarder();
    forwarder->setTarget(weak_from_this());
    static std::once_flag registered;
    std::call_once(registered, [forwarder] {
        auto factory = firebolt::rialto::IClientLogControlFactory::createFactory();
        if (factory)
        {
            factory->createClientLogControl().registerLogHandler(forwarder, false);
        }
    });
}

} // namespace rialto_refplayer
