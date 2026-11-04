#pragma once

#include "Logger.h"

#include "rialto_refplayer/RefPlayerTypes.h"

#include "IControl.h"
#include "IMediaPipeline.h"

#include <functional>
#include <memory>
#include <string>

namespace rialto_refplayer
{

// Everything that talks to the Rialto control and pipeline factories. The control
// registration is kept for the life of the session. The pipeline is created and
// released as playback needs it, so stop() releases it and load() recreates it.
class RialtoSession
{
public:
    using AppStateHandler = std::function<void(firebolt::rialto::ApplicationState)>;

    RialtoSession(std::shared_ptr<Logger> logger, AppStateHandler onAppState);
    ~RialtoSession();

    RialtoSession(const RialtoSession &) = delete;
    RialtoSession &operator=(const RialtoSession &) = delete;

    // Creates the control and registers. Fails unless the application is RUNNING.
    // Tolerates an existing control, so a re-connect does not register twice.
    Result connect();

    // Creates the pipeline for the given client. Returns RialtoError on failure.
    Result createPipeline(std::weak_ptr<firebolt::rialto::IMediaPipelineClient> client,
                          const firebolt::rialto::VideoRequirements &requirements);

    // Destroys the pipeline. Keeps the control registration.
    void releasePipeline();

    // Drops the control registration as well.
    void disconnect();

    bool isConnected() const { return m_control != nullptr; }
    bool hasPipeline() const { return m_pipeline != nullptr; }
    firebolt::rialto::IMediaPipeline *pipeline() const { return m_pipeline.get(); }

private:
    std::shared_ptr<Logger> m_logger;
    AppStateHandler m_onAppState;
    std::shared_ptr<firebolt::rialto::IControl> m_control;
    std::shared_ptr<firebolt::rialto::IControlClient> m_controlClient;
    std::shared_ptr<firebolt::rialto::IMediaPipelineFactory> m_pipelineFactory;
    std::unique_ptr<firebolt::rialto::IMediaPipeline> m_pipeline;
};

} // namespace rialto_refplayer
