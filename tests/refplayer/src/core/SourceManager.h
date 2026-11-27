#pragma once

#include "Logger.h"

#include "IMediaPipeline.h"
#include "rialto_refplayer/RefPlayerTypes.h"
#include "IDemuxer.h"
#include "TrackModel.h"

#include <cstdint>
#include <functional>
#include <istream>
#include <memory>
#include <string>

namespace rialto_refplayer
{

// Opens a source, parses its tracks, and attaches the resulting sources to the pipeline.
// It owns the input stream and the track model. MediaFeeder reads them only while it runs,
// and teardown joins the feeder before removeAll() clears them.
class SourceManager
{
public:
    using DemuxerFactory = std::function<std::unique_ptr<demux::IDemuxer>(const std::string &url,
                                                                           const std::string &mimeType)>;
    using StreamOpener = std::function<std::unique_ptr<std::istream>(const std::string &url)>;

    struct Dependencies
    {
        DemuxerFactory createDemuxer;
        StreamOpener openStream;
    };

    SourceManager(std::shared_ptr<Logger> logger, Dependencies dependencies);
    ~SourceManager();

    // Opens and parses. On failure, nothing is retained.
    bool open(const std::string &url, const std::string &mimeType, std::string &error);

    // Attaches the video source (parsed size and avcC) and the audio source if the track exists.
    bool attach(firebolt::rialto::IMediaPipeline &pipeline);

    // Removes every attached source, then clears the track model and closes the stream.
    // The pipeline may be null (already released).
    void removeAll(firebolt::rialto::IMediaPipeline *pipeline);

    const demux::TrackModel &tracks() const { return m_tracks; }
    std::istream *input() const { return m_input.get(); }
    int32_t videoSourceId() const { return m_videoSourceId; }
    int32_t audioSourceId() const { return m_audioSourceId; }
    bool hasAudioSource() const { return m_audioSourceId != 0; }
    bool isOpen() const { return m_input != nullptr; }

private:
    std::shared_ptr<Logger> m_logger;
    Dependencies m_dependencies;
    std::unique_ptr<std::istream> m_input;
    demux::TrackModel m_tracks;
    int32_t m_videoSourceId{0};
    int32_t m_audioSourceId{0};
};

// Opens a local file:// URL for reading in binary mode.
std::unique_ptr<std::istream> openLocalFile(const std::string &url);

} // namespace rialto_refplayer
