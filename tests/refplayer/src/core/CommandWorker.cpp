#include "CommandWorker.h"

namespace rialto_refplayer
{

CommandWorker::CommandWorker() : m_thread(&CommandWorker::run, this)
{
}

CommandWorker::~CommandWorker()
{
    shutdown();
}

bool CommandWorker::post(Command command)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stopping)
        {
            return false;
        }
        m_queue.push_back(std::move(command));
    }
    m_cv.notify_one();
    return true;
}

bool CommandWorker::idle() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.empty() && !m_running;
}

void CommandWorker::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopping = true;
        m_queue.clear();
    }
    m_cv.notify_all();
    if (m_thread.joinable() && m_thread.get_id() != std::this_thread::get_id())
    {
        m_thread.join();
    }
}

void CommandWorker::run()
{
    while (true)
    {
        Command command;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return m_stopping || !m_queue.empty(); });
            if (m_stopping)
            {
                return;
            }
            command = std::move(m_queue.front());
            m_queue.pop_front();
            m_running = true;
        }
        command();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_running = false;
        }
    }
}

} // namespace rialto_refplayer
