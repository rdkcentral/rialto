#include "MediaPipelineIpcFactoryMock.h"
#include "MediaPipelineIpcMock.h"
#include "MediaPipelineIpc.h"

namespace firebolt::rialto::client
{
std::weak_ptr<IMediaPipelineIpcFactory> m_factory;

std::shared_ptr<IMediaPipelineIpcFactory> IMediaPipelineIpcFactory::getFactory()
{
    std::shared_ptr<IMediaPipelineIpcFactory> factory = m_factory.lock();
    std::cout << "lukewill" << std::endl;

    if (!factory)
    {
        try
        {
            factory = std::make_shared<MediaPipelineIpcFactoryMock>();
        }
        catch (const std::exception &e)
        {
        }

        m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IMediaPipelineIpc> MediaPipelineIpcFactory::createMediaPipelineIpc(
    IMediaPipelineIpcClient *client, const VideoRequirements &videoRequirements, std::weak_ptr<IIpcClient> ipcClientParam)
{
    std::unique_ptr<IMediaPipelineIpc> mediaPipelineIpc;
    try
    {
        std::shared_ptr<IIpcClient> ipcClient = ipcClientParam.lock();
        mediaPipelineIpc =
            std::make_unique<MediaPipelineIpc>(client, videoRequirements,
                                               ipcClient ? *ipcClient : IIpcClientAccessor::instance().getIpcClient(),
                                               firebolt::rialto::common::IEventThreadFactory::createFactory());
    }
    catch (const std::exception &e)
    {
    }

    return mediaPipelineIpc;
}
}
