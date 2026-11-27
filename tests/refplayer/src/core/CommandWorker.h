#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace rialto_refplayer
{

// Runs control commands one at a time, in the order they were posted, on its own
// thread. Posting never blocks on a command. A command may block on Rialto, but no
// Rialto callback ever waits on the worker.
class CommandWorker
{
public:
    using Command = std::function<void()>;

    CommandWorker();
    ~CommandWorker();

    CommandWorker(const CommandWorker &) = delete;
    CommandWorker &operator=(const CommandWorker &) = delete;

    // Returns false once shut down; the command is dropped.
    bool post(Command command);

    // True when nothing is queued and nothing is running. Only the poster may rely on it,
    // because a poster that checks idle() and then posts cannot be overtaken by its own work.
    bool idle() const;

    // Drops queued commands, lets the running one finish, and joins. Idempotent.
    // Safe to call from a command: the join is then skipped.
    void shutdown();

private:
    void run();

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<Command> m_queue;
    bool m_running{false};
    bool m_stopping{false};
    std::thread m_thread;
};

} // namespace rialto_refplayer
