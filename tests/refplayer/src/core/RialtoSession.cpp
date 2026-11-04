#include "RialtoSession.h"

#include "RialtoNames.h"

namespace rialto_refplayer
{
namespace
{

// Forwards the application state to the owner. Owned by the session, and only weakly
// held by Rialto, so the handler never outlives the session that installed it.
class ControlCallbacks final : public firebolt::rialto::IControlClient
{
public:
    explicit ControlCallbacks(RialtoSession::AppStateHandler handler) : m_handler(std::move(handler)) {}

    void notifyApplicationState(firebolt::rialto::ApplicationState state) override
    {
        if (m_handler)
        {
            m_handler(state);
        }
    }

private:
    RialtoSession::AppStateHandler m_handler;
};

} // namespace

RialtoSession::RialtoSession(std::shared_ptr<Logger> logger, AppStateHandler onAppState)
    : m_logger(std::move(logger)), m_onAppState(std::move(onAppState))
{
}

RialtoSession::~RialtoSession()
{
    releasePipeline();
}

Result RialtoSession::connect()
{
    if (!m_control)
    {
        const auto factory = firebolt::rialto::IControlFactory::createFactory();
        if (!factory)
        {
            m_logger->error("IControlFactory::createFactory() returned null");
            return Result::RialtoError;
        }
        m_control = factory->createControl();
        if (!m_control)
        {
            m_logger->error("IControlFactory::createControl() failed");
            return Result::RialtoError;
        }
    }

    if (!m_controlClient)
    {
        m_controlClient = std::make_shared<ControlCallbacks>(m_onAppState);
    }

    firebolt::rialto::ApplicationState appState = firebolt::rialto::ApplicationState::UNKNOWN;
    if (!m_control->registerClient(m_controlClient, appState))
    {
        m_logger->error("IControl::registerClient() failed");
        return Result::RialtoError;
    }

    m_logger->info("Registered control client; application state = " + toString(appState));
    if (appState != firebolt::rialto::ApplicationState::RUNNING)
    {
        m_logger->error("Application is not RUNNING; playback cannot start");
        return Result::ApplicationNotRunning;
    }
    return Result::Ok;
}

Result RialtoSession::createPipeline(std::weak_ptr<firebolt::rialto::IMediaPipelineClient> client,
                                     const firebolt::rialto::VideoRequirements &requirements)
{
    if (!m_pipelineFactory)
    {
        m_pipelineFactory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    }
    if (!m_pipelineFactory)
    {
        m_logger->error("IMediaPipelineFactory::createFactory() returned null");
        return Result::RialtoError;
    }

    m_pipeline = m_pipelineFactory->createMediaPipeline(std::move(client), requirements);
    if (!m_pipeline)
    {
        m_logger->error("IMediaPipelineFactory::createMediaPipeline() failed");
        return Result::RialtoError;
    }
    m_logger->info("Media pipeline created");
    return Result::Ok;
}

void RialtoSession::releasePipeline()
{
    if (m_pipeline)
    {
        m_pipeline.reset();
        m_logger->info("Media pipeline released");
    }
}

void RialtoSession::disconnect()
{
    releasePipeline();
    m_controlClient.reset();
    m_control.reset();
}

} // namespace rialto_refplayer
