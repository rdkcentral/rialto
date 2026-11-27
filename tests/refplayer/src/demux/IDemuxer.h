#pragma once

#include "TrackModel.h"

#include <istream>
#include <memory>
#include <string>

namespace rialto_refplayer::demux
{

// Container parser. It reads only what it is given, parses the track model, and
// never fetches anything itself. Sample bytes are read later by MediaFeeder through
// the stream its owner keeps open.
class IDemuxer
{
public:
    virtual ~IDemuxer() = default;

    virtual bool parse(std::istream &input, TrackModel &model, std::string &error) = 0;
};

// Returns the demuxer registered for this URL and MIME type, or nullptr if none accepts it.
// Phase 1 registers local MP4 only. Phase 3 (fMP4) and phase 5 (TS) register behind this function.
std::unique_ptr<IDemuxer> createDemuxer(const std::string &url, const std::string &mimeType);

} // namespace rialto_refplayer::demux
