#include "Mp4Demuxer.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace reference_player::mp4_demuxer
{
namespace
{
constexpr uint32_t fourcc(const char (&s)[5])
{
    return (static_cast<uint32_t>(s[0]) << 24) | (static_cast<uint32_t>(s[1]) << 16) |
           (static_cast<uint32_t>(s[2]) << 8) | static_cast<uint32_t>(s[3]);
}

uint16_t be16(const uint8_t *p)
{
    return static_cast<uint16_t>((p[0] << 8) | p[1]);
}

uint32_t be32(const uint8_t *p)
{
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}

uint64_t be64(const uint8_t *p)
{
    return (static_cast<uint64_t>(be32(p)) << 32) | be32(p + 4);
}

// Bounds-checked big-endian reader; 'ok' turns false on any overrun.
struct Reader
{
    const std::vector<uint8_t> &data;
    size_t pos;
    size_t end;
    bool ok{true};

    size_t remaining() const { return end - pos; }
    void skip(size_t n)
    {
        if (n > remaining())
        {
            ok = false;
            pos = end;
            return;
        }
        pos += n;
    }
    uint8_t u8()
    {
        if (remaining() < 1)
        {
            ok = false;
            return 0;
        }
        return data[pos++];
    }
    uint32_t u32()
    {
        if (remaining() < 4)
        {
            ok = false;
            pos = end;
            return 0;
        }
        const uint32_t v = be32(&data[pos]);
        pos += 4;
        return v;
    }
    uint64_t u64()
    {
        if (remaining() < 8)
        {
            ok = false;
            pos = end;
            return 0;
        }
        const uint64_t v = be64(&data[pos]);
        pos += 8;
        return v;
    }
};

// Calls fn(type, payloadBegin, boxEnd) for each box in [begin, end).
template <typename Fn> void forEachBox(const std::vector<uint8_t> &data, size_t begin, size_t end, Fn &&fn)
{
    size_t pos = begin;
    while (pos + 8 <= end)
    {
        uint64_t size = be32(&data[pos]);
        const uint32_t type = be32(&data[pos + 4]);
        size_t headerSize = 8;
        if (size == 1)
        {
            if (pos + 16 > end)
            {
                return;
            }
            size = be64(&data[pos + 8]);
            headerSize = 16;
        }
        else if (size == 0)
        {
            size = end - pos;
        }
        if (size < headerSize || size > end - pos)
        {
            return;
        }
        fn(type, pos + headerSize, pos + static_cast<size_t>(size));
        pos += static_cast<size_t>(size);
    }
}

bool findBox(const std::vector<uint8_t> &data, size_t begin, size_t end, uint32_t type, size_t &payload,
             size_t &boxEnd)
{
    bool found = false;
    forEachBox(data, begin, end,
               [&](uint32_t boxType, size_t boxPayload, size_t end_)
               {
                   if (!found && boxType == type)
                   {
                       found = true;
                       payload = boxPayload;
                       boxEnd = end_;
                   }
               });
    return found;
}

int64_t ticksToNs(int64_t ticks, uint32_t timescale)
{
    if (ticks < 0)
    {
        return -ticksToNs(-ticks, timescale);
    }
    const uint64_t t = static_cast<uint64_t>(ticks);
    return static_cast<int64_t>((t / timescale) * 1000000000ULL + (t % timescale) * 1000000000ULL / timescale);
}

// Reads an H.264 (avc1/avc3) track's configuration and sample table from a 'trak' box.
bool parseVideoTrack(const std::vector<uint8_t> &d, size_t trakBegin, size_t trakEnd, VideoTrack &out)
{
    size_t mdiaB = 0, mdiaE = 0, p = 0, e = 0;
    if (!findBox(d, trakBegin, trakEnd, fourcc("mdia"), mdiaB, mdiaE))
    {
        return false;
    }

    if (!findBox(d, mdiaB, mdiaE, fourcc("hdlr"), p, e))
    {
        return false;
    }
    Reader hdlr{d, p, e};
    hdlr.skip(8); // version/flags + pre_defined
    if (hdlr.u32() != fourcc("vide") || !hdlr.ok)
    {
        return false;
    }

    if (!findBox(d, mdiaB, mdiaE, fourcc("mdhd"), p, e))
    {
        return false;
    }
    Reader mdhd{d, p, e};
    const uint8_t mdhdVersion = mdhd.u8();
    mdhd.skip(3);
    mdhd.skip(mdhdVersion == 1 ? 16 : 8); // creation + modification times
    const uint32_t timescale = mdhd.u32();
    if (!mdhd.ok || timescale == 0)
    {
        return false;
    }

    size_t minfB = 0, minfE = 0, stblB = 0, stblE = 0;
    if (!findBox(d, mdiaB, mdiaE, fourcc("minf"), minfB, minfE) ||
        !findBox(d, minfB, minfE, fourcc("stbl"), stblB, stblE))
    {
        return false;
    }

    // First sample entry must be H.264 and carry the avcC configuration record.
    if (!findBox(d, stblB, stblE, fourcc("stsd"), p, e))
    {
        return false;
    }
    Reader stsd{d, p, e};
    stsd.skip(4);
    if (stsd.u32() < 1)
    {
        return false;
    }
    const uint32_t entrySize = stsd.u32();
    const uint32_t entryType = stsd.u32();
    if (!stsd.ok || (entryType != fourcc("avc1") && entryType != fourcc("avc3")))
    {
        return false;
    }
    const size_t entryPayload = stsd.pos;
    constexpr size_t kVisualSampleEntryBytes = 78;
    if (entrySize < 8 + kVisualSampleEntryBytes || entryPayload - 8 + entrySize > e)
    {
        return false;
    }
    const size_t entryEnd = entryPayload - 8 + entrySize;
    out.width = be16(&d[entryPayload + 24]);
    out.height = be16(&d[entryPayload + 26]);
    size_t avcB = 0, avcE = 0;
    if (!findBox(d, entryPayload + kVisualSampleEntryBytes, entryEnd, fourcc("avcC"), avcB, avcE))
    {
        return false;
    }
    out.codecData.assign(d.begin() + static_cast<std::ptrdiff_t>(avcB), d.begin() + static_cast<std::ptrdiff_t>(avcE));

    // Decoding time deltas.
    if (!findBox(d, stblB, stblE, fourcc("stts"), p, e))
    {
        return false;
    }
    Reader stts{d, p, e};
    stts.skip(4);
    const uint32_t sttsCount = stts.u32();
    if (!stts.ok || sttsCount > stts.remaining() / 8)
    {
        return false;
    }
    std::vector<std::pair<uint32_t, uint32_t>> timeToSample(sttsCount);
    for (auto &entry : timeToSample)
    {
        entry.first = stts.u32();
        entry.second = stts.u32();
    }

    // Sample sizes.
    if (!findBox(d, stblB, stblE, fourcc("stsz"), p, e))
    {
        return false;
    }
    Reader stsz{d, p, e};
    stsz.skip(4);
    const uint32_t constantSize = stsz.u32();
    const uint32_t sampleCount = stsz.u32();
    if (!stsz.ok)
    {
        return false;
    }
    std::vector<uint32_t> sizes;
    if (constantSize != 0)
    {
        sizes.assign(sampleCount, constantSize);
    }
    else
    {
        if (sampleCount > stsz.remaining() / 4)
        {
            return false;
        }
        sizes.resize(sampleCount);
        for (auto &size : sizes)
        {
            size = stsz.u32();
        }
    }

    // Samples per chunk.
    if (!findBox(d, stblB, stblE, fourcc("stsc"), p, e))
    {
        return false;
    }
    Reader stsc{d, p, e};
    stsc.skip(4);
    const uint32_t stscCount = stsc.u32();
    if (!stsc.ok || stscCount == 0 || stscCount > stsc.remaining() / 12)
    {
        return false;
    }
    struct ChunkRun
    {
        uint32_t firstChunk;
        uint32_t samplesPerChunk;
    };
    std::vector<ChunkRun> runs(stscCount);
    for (auto &run : runs)
    {
        run.firstChunk = stsc.u32();
        run.samplesPerChunk = stsc.u32();
        stsc.skip(4); // sample_description_index
    }

    // Chunk offsets (32-bit stco or 64-bit co64).
    std::vector<uint64_t> chunkOffsets;
    if (findBox(d, stblB, stblE, fourcc("stco"), p, e))
    {
        Reader stco{d, p, e};
        stco.skip(4);
        const uint32_t count = stco.u32();
        if (!stco.ok || count > stco.remaining() / 4)
        {
            return false;
        }
        for (uint32_t i = 0; i < count; ++i)
        {
            chunkOffsets.push_back(stco.u32());
        }
    }
    else if (findBox(d, stblB, stblE, fourcc("co64"), p, e))
    {
        Reader co64{d, p, e};
        co64.skip(4);
        const uint32_t count = co64.u32();
        if (!co64.ok || count > co64.remaining() / 8)
        {
            return false;
        }
        for (uint32_t i = 0; i < count; ++i)
        {
            chunkOffsets.push_back(co64.u64());
        }
    }
    else
    {
        return false;
    }

    // Composition offsets (optional; present when the stream has B-frames).
    std::vector<std::pair<uint32_t, int32_t>> compositionOffsets;
    if (findBox(d, stblB, stblE, fourcc("ctts"), p, e))
    {
        Reader ctts{d, p, e};
        ctts.skip(4);
        const uint32_t count = ctts.u32();
        if (!ctts.ok || count > ctts.remaining() / 8)
        {
            return false;
        }
        compositionOffsets.resize(count);
        for (auto &entry : compositionOffsets)
        {
            entry.first = ctts.u32();
            entry.second = static_cast<int32_t>(ctts.u32());
        }
    }

    // File offset and size of every sample.
    out.samples.assign(sampleCount, Sample{0, 0, 0, 0});
    size_t sample = 0;
    size_t run = 0;
    for (size_t chunk = 0; chunk < chunkOffsets.size() && sample < sampleCount; ++chunk)
    {
        while (run + 1 < runs.size() && runs[run + 1].firstChunk <= chunk + 1)
        {
            ++run;
        }
        uint64_t offset = chunkOffsets[chunk];
        for (uint32_t i = 0; i < runs[run].samplesPerChunk && sample < sampleCount; ++i, ++sample)
        {
            out.samples[sample].offset = offset;
            out.samples[sample].size = sizes[sample];
            offset += sizes[sample];
        }
    }
    if (sample != sampleCount)
    {
        return false;
    }

    // Presentation time and duration of every sample.
    std::vector<int64_t> dtsTicks(sampleCount, 0);
    std::vector<int64_t> deltaTicks(sampleCount, 0);
    std::vector<int64_t> ctsTicks(sampleCount, 0);
    size_t index = 0;
    int64_t dts = 0;
    for (const auto &entry : timeToSample)
    {
        for (uint32_t i = 0; i < entry.first && index < sampleCount; ++i, ++index)
        {
            dtsTicks[index] = dts;
            deltaTicks[index] = entry.second;
            dts += entry.second;
        }
    }
    index = 0;
    for (const auto &entry : compositionOffsets)
    {
        for (uint32_t i = 0; i < entry.first && index < sampleCount; ++i, ++index)
        {
            ctsTicks[index] = entry.second;
        }
    }
    for (size_t i = 0; i < sampleCount; ++i)
    {
        out.samples[i].ptsNs = std::max<int64_t>(0, ticksToNs(dtsTicks[i] + ctsTicks[i], timescale));
        out.samples[i].durationNs = ticksToNs(deltaTicks[i], timescale);
    }
    return true;
}

bool readDescriptorLength(const std::vector<uint8_t> &data, size_t &pos, size_t end, size_t &length)
{
    length = 0;
    for (size_t i = 0; i < 4; ++i)
    {
        if (pos >= end)
        {
            return false;
        }
        const uint8_t value = data[pos++];
        length = (length << 7) | (value & 0x7f);
        if ((value & 0x80) == 0)
        {
            return true;
        }
    }
    return false;
}

bool findAudioSpecificConfig(const std::vector<uint8_t> &data, size_t begin, size_t end, unsigned depth,
                             std::vector<uint8_t> &config)
{
    if (depth > 4)
    {
        return false;
    }
    size_t pos = begin;
    while (pos < end)
    {
        const uint8_t tag = data[pos++];
        size_t length = 0;
        if (!readDescriptorLength(data, pos, end, length) || length > end - pos)
        {
            return false;
        }
        const size_t payloadEnd = pos + length;
        if (tag == 0x05)
        {
            config.assign(data.begin() + static_cast<std::ptrdiff_t>(pos),
                          data.begin() + static_cast<std::ptrdiff_t>(payloadEnd));
            return !config.empty();
        }

        size_t children = pos;
        if (tag == 0x03)
        {
            if (payloadEnd - children < 3)
            {
                return false;
            }
            children += 3;
            const uint8_t flags = data[pos + 2];
            if (flags & 0x80) children += 2;
            if (flags & 0x40)
            {
                if (children >= payloadEnd) return false;
                children += 1 + data[children];
            }
            if (flags & 0x20) children += 2;
        }
        else if (tag == 0x04)
        {
            if (payloadEnd - children < 13)
            {
                return false;
            }
            children += 13;
        }

        if (children < payloadEnd && findAudioSpecificConfig(data, children, payloadEnd, depth + 1, config))
        {
            return true;
        }
        pos = payloadEnd;
    }
    return false;
}

bool parseSampleTable(const std::vector<uint8_t> &d, size_t stblB, size_t stblE, uint32_t timescale,
                      std::vector<Sample> &samples)
{
    size_t p = 0, e = 0;
    if (!findBox(d, stblB, stblE, fourcc("stts"), p, e))
    {
        return false;
    }
    Reader stts{d, p, e};
    stts.skip(4);
    const uint32_t sttsCount = stts.u32();
    if (!stts.ok || sttsCount > stts.remaining() / 8)
    {
        return false;
    }
    std::vector<std::pair<uint32_t, uint32_t>> timeToSample(sttsCount);
    for (auto &entry : timeToSample)
    {
        entry.first = stts.u32();
        entry.second = stts.u32();
    }

    if (!findBox(d, stblB, stblE, fourcc("stsz"), p, e))
    {
        return false;
    }
    Reader stsz{d, p, e};
    stsz.skip(4);
    const uint32_t constantSize = stsz.u32();
    const uint32_t sampleCount = stsz.u32();
    if (!stsz.ok)
    {
        return false;
    }
    std::vector<uint32_t> sizes;
    if (constantSize != 0)
    {
        sizes.assign(sampleCount, constantSize);
    }
    else
    {
        if (sampleCount > stsz.remaining() / 4) return false;
        sizes.resize(sampleCount);
        for (auto &size : sizes) size = stsz.u32();
    }

    if (!findBox(d, stblB, stblE, fourcc("stsc"), p, e))
    {
        return false;
    }
    Reader stsc{d, p, e};
    stsc.skip(4);
    const uint32_t stscCount = stsc.u32();
    if (!stsc.ok || stscCount == 0 || stscCount > stsc.remaining() / 12) return false;
    struct ChunkRun { uint32_t firstChunk; uint32_t samplesPerChunk; };
    std::vector<ChunkRun> runs(stscCount);
    for (auto &run : runs)
    {
        run.firstChunk = stsc.u32();
        run.samplesPerChunk = stsc.u32();
        stsc.skip(4);
    }

    std::vector<uint64_t> chunkOffsets;
    if (findBox(d, stblB, stblE, fourcc("stco"), p, e))
    {
        Reader offsets{d, p, e};
        offsets.skip(4);
        const uint32_t count = offsets.u32();
        if (!offsets.ok || count > offsets.remaining() / 4) return false;
        for (uint32_t i = 0; i < count; ++i) chunkOffsets.push_back(offsets.u32());
    }
    else if (findBox(d, stblB, stblE, fourcc("co64"), p, e))
    {
        Reader offsets{d, p, e};
        offsets.skip(4);
        const uint32_t count = offsets.u32();
        if (!offsets.ok || count > offsets.remaining() / 8) return false;
        for (uint32_t i = 0; i < count; ++i) chunkOffsets.push_back(offsets.u64());
    }
    else
    {
        return false;
    }

    std::vector<std::pair<uint32_t, int32_t>> compositionOffsets;
    if (findBox(d, stblB, stblE, fourcc("ctts"), p, e))
    {
        Reader ctts{d, p, e};
        const uint8_t version = ctts.u8();
        ctts.skip(3);
        const uint32_t count = ctts.u32();
        if (!ctts.ok || count > ctts.remaining() / 8) return false;
        compositionOffsets.resize(count);
        for (auto &entry : compositionOffsets)
        {
            entry.first = ctts.u32();
            const uint32_t offset = ctts.u32();
            entry.second = version == 1 ? static_cast<int32_t>(offset) : static_cast<int64_t>(offset);
        }
    }

    samples.assign(sampleCount, Sample{0, 0, 0, 0});
    size_t sample = 0, run = 0;
    for (size_t chunk = 0; chunk < chunkOffsets.size() && sample < sampleCount; ++chunk)
    {
        while (run + 1 < runs.size() && runs[run + 1].firstChunk <= chunk + 1) ++run;
        uint64_t offset = chunkOffsets[chunk];
        for (uint32_t i = 0; i < runs[run].samplesPerChunk && sample < sampleCount; ++i, ++sample)
        {
            samples[sample].offset = offset;
            samples[sample].size = sizes[sample];
            offset += sizes[sample];
        }
    }
    if (sample != sampleCount) return false;

    std::vector<int64_t> dts(sampleCount, 0), durations(sampleCount, 0), cts(sampleCount, 0);
    size_t index = 0;
    int64_t currentDts = 0;
    for (const auto &entry : timeToSample)
    {
        for (uint32_t i = 0; i < entry.first && index < sampleCount; ++i, ++index)
        {
            dts[index] = currentDts;
            durations[index] = entry.second;
            currentDts += entry.second;
        }
    }
    index = 0;
    for (const auto &entry : compositionOffsets)
    {
        for (uint32_t i = 0; i < entry.first && index < sampleCount; ++i, ++index) cts[index] = entry.second;
    }
    for (size_t i = 0; i < sampleCount; ++i)
    {
        samples[i].ptsNs = std::max<int64_t>(0, ticksToNs(dts[i] + cts[i], timescale));
        samples[i].durationNs = ticksToNs(durations[i], timescale);
    }
    return sampleCount != 0;
}

// Reads an AAC (mp4a) track and its AudioSpecificConfig from an MP4 'trak'.
bool parseAudioTrack(const std::vector<uint8_t> &d, size_t trakBegin, size_t trakEnd, AudioTrack &out)
{
    size_t p = 0, e = 0, mdiaB = 0, mdiaE = 0;
    if (!findBox(d, trakBegin, trakEnd, fourcc("mdia"), mdiaB, mdiaE) ||
        !findBox(d, mdiaB, mdiaE, fourcc("hdlr"), p, e)) return false;
    Reader hdlr{d, p, e};
    hdlr.skip(8);
    if (hdlr.u32() != fourcc("soun") || !hdlr.ok) return false;

    if (!findBox(d, mdiaB, mdiaE, fourcc("mdhd"), p, e)) return false;
    Reader mdhd{d, p, e};
    const uint8_t mdhdVersion = mdhd.u8();
    mdhd.skip(3);
    mdhd.skip(mdhdVersion == 1 ? 16 : 8);
    const uint32_t timescale = mdhd.u32();
    if (!mdhd.ok || timescale == 0) return false;

    size_t minfB = 0, minfE = 0, stblB = 0, stblE = 0;
    if (!findBox(d, mdiaB, mdiaE, fourcc("minf"), minfB, minfE) ||
        !findBox(d, minfB, minfE, fourcc("stbl"), stblB, stblE) ||
        !findBox(d, stblB, stblE, fourcc("stsd"), p, e)) return false;

    Reader stsd{d, p, e};
    stsd.skip(4);
    if (stsd.u32() < 1) return false;
    const uint32_t entrySize = stsd.u32();
    const uint32_t entryType = stsd.u32();
    if (!stsd.ok || entryType != fourcc("mp4a")) return false;
    const size_t entryPayload = stsd.pos;
    if (entrySize < 8 + 28 || entryPayload - 8 + entrySize > e || be16(&d[entryPayload + 8]) != 0) return false;
    const size_t entryEnd = entryPayload - 8 + entrySize;
    out.channels = be16(&d[entryPayload + 16]);
    out.sampleRate = static_cast<int32_t>(be32(&d[entryPayload + 24]) >> 16);

    size_t esdsB = 0, esdsE = 0;
    if (!findBox(d, entryPayload + 28, entryEnd, fourcc("esds"), esdsB, esdsE) || esdsE - esdsB < 4 ||
        !findAudioSpecificConfig(d, esdsB + 4, esdsE, 0, out.codecData) ||
        out.channels <= 0 || out.sampleRate <= 0) return false;

    return parseSampleTable(d, stblB, stblE, timescale, out.samples);
}

bool isLocalMp4Impl(const std::string &url)
{
    std::string lower = url;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower.rfind("file://", 0) == 0 && lower.size() > 4 && lower.compare(lower.size() - 4, 4, ".mp4") == 0;
}

} // namespace

bool isLocalMp4(const std::string &url)
{
    return isLocalMp4Impl(url);
}

bool parse(std::istream &file, Tracks &tracks, std::string &error)
{
    file.clear();
    file.seekg(0, std::ios::end);
    const std::streamoff fileSizeValue = file.tellg();
    if (!file || fileSizeValue < 0)
    {
        error = "Cannot determine MP4 file size";
        return false;
    }
    const uint64_t fileSize = static_cast<uint64_t>(fileSizeValue);

    std::vector<uint8_t> moov;
    uint64_t pos = 0;
    while (pos + 8 <= fileSize)
    {
        uint8_t header[16] = {};
        file.clear();
        file.seekg(static_cast<std::streamoff>(pos));
        file.read(reinterpret_cast<char *>(header), 8);
        uint64_t size = be32(header);
        const uint32_t type = be32(header + 4);
        uint64_t headerSize = 8;
        if (size == 1)
        {
            file.read(reinterpret_cast<char *>(header + 8), 8);
            size = be64(header + 8);
            headerSize = 16;
        }
        else if (size == 0)
        {
            size = fileSize - pos;
        }
        if (!file || size < headerSize || size > fileSize - pos)
        {
            break;
        }
        if (type == fourcc("moov"))
        {
            moov.resize(static_cast<size_t>(size - headerSize));
            file.seekg(static_cast<std::streamoff>(pos + headerSize));
            file.read(reinterpret_cast<char *>(moov.data()), static_cast<std::streamsize>(moov.size()));
            if (!file)
            {
                moov.clear();
            }
            break;
        }
        pos += size;
    }
    if (moov.empty())
    {
        error = "No moov box found (expected a non-fragmented MP4)";
        return false;
    }

    bool foundVideo = false;
    forEachBox(moov, 0, moov.size(),
               [&tracks, &foundVideo, &moov](uint32_t type, size_t payload, size_t end)
               {
                   if (type != fourcc("trak"))
                   {
                       return;
                   }
                   if (!foundVideo)
                   {
                       VideoTrack candidate;
                       if (parseVideoTrack(moov, payload, end, candidate))
                       {
                           tracks.video = std::move(candidate);
                           foundVideo = true;
                       }
                   }
                   if (!tracks.hasAudio)
                   {
                       AudioTrack candidate;
                       if (parseAudioTrack(moov, payload, end, candidate))
                       {
                           tracks.audio = std::move(candidate);
                           tracks.hasAudio = true;
                       }
                   }
               });
    if (!foundVideo || tracks.video.samples.empty())
    {
        error = "No H.264 (avc1/avc3) video track found";
        return false;
    }
    return true;
}
} // namespace reference_player::mp4_demuxer
