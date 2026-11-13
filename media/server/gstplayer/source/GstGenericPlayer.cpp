/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <atomic>  // RDKAI
#include <climits>  // RDKAI
#include <cmath>    // RDKAI
#include <cstdint>  // RDKAI
#include <cstdio>   // RDKAI
#include <cstdlib>  // RDKAI
#include <memory>   // RDKAI
#include <mutex>    // RDKAI
#include <string>   // RDKAI
#include <thread>   // RDKAI
#include <vector>   // RDKAI
#include <gst/audio/audio.h> // RDKAI
#include <sys/stat.h>        // RDKAI
#include <unistd.h>          // RDKAI
#include <algorithm>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <limits>
#include <malloc.h>
#include <optional>
#include <stdexcept>

#include "FlushWatcher.h"
#include "GstDispatcherThread.h"
#include "GstGenericPlayer.h"
#include "GstProfiler.h"
#include "GstProtectionMetadata.h"
#include "IGstTextTrackSinkFactory.h"
#include "IMediaPipeline.h"
#include "ITimer.h"
#include "RialtoServerLogging.h"
#include "TypeConverters.h"
#include "Utils.h"
#include "WorkerThread.h"
#include "tasks/generic/FirstFrameReceived.h"
#include "tasks/generic/GenericPlayerTaskFactory.h"

namespace
{
/**
 * @brief Report position interval in ms.
 *        The position reporting timer should be started whenever the PLAYING state is entered and stopped
 *        whenever the session moves to another playback state.
 */
constexpr std::chrono::milliseconds kPositionReportTimerMs{250};
constexpr std::chrono::seconds kSubtitleClockResyncInterval{10};

std::optional<int> getIntEnv(const char *envName)
{
    const char *value = std::getenv(envName);
    if (!value || value[0] == '\0')
    {
        return std::nullopt;
    }

    char *endPtr{nullptr};
    const int64_t parsedValue{std::strtol(value, &endPtr, 10)};
    if (endPtr == value || *endPtr != '\0' || parsedValue < std::numeric_limits<int>::min() ||
        parsedValue > std::numeric_limits<int>::max())
    {
        RIALTO_SERVER_LOG_WARN("Ignoring invalid integer value '%s' from %s", value, envName);
        return std::nullopt;
    }

    return static_cast<int>(parsedValue);
}

std::optional<firebolt::rialto::server::Rectangle> getDefaultVideoGeometryFromEnvironment()
{
    const char *rectangleValue = std::getenv("RIALTO_VIDEO_WINDOW_RECTANGLE");
    if (rectangleValue && rectangleValue[0] != '\0')
    {
        int x{}, y{}, width{}, height{};
        if (std::sscanf(rectangleValue, "%d,%d,%d,%d", &x, &y, &width, &height) == 4)
        {
            if (width > 0 && height > 0)
            {
                return firebolt::rialto::server::Rectangle{x, y, width, height};
            }

            RIALTO_SERVER_LOG_WARN("Ignoring invalid %s value '%s' because width/height must be positive",
                                   "RIALTO_VIDEO_WINDOW_RECTANGLE", rectangleValue);
            return std::nullopt;
        }

        RIALTO_SERVER_LOG_WARN("Ignoring invalid %s value '%s', expected x,y,width,height",
                               "RIALTO_VIDEO_WINDOW_RECTANGLE", rectangleValue);
        return std::nullopt;
    }

    const std::optional<int> x = getIntEnv("RIALTO_VIDEO_WINDOW_X");
    const std::optional<int> y = getIntEnv("RIALTO_VIDEO_WINDOW_Y");
    const std::optional<int> width = getIntEnv("RIALTO_VIDEO_WINDOW_WIDTH");
    const std::optional<int> height = getIntEnv("RIALTO_VIDEO_WINDOW_HEIGHT");

    if (!x && !y && !width && !height)
    {
        return std::nullopt;
    }

    if (!x || !y || !width || !height)
    {
        RIALTO_SERVER_LOG_WARN("Ignoring incomplete video geometry environment. Set all of %s, %s, %s and %s",
                               "RIALTO_VIDEO_WINDOW_X", "RIALTO_VIDEO_WINDOW_Y", "RIALTO_VIDEO_WINDOW_WIDTH",
                               "RIALTO_VIDEO_WINDOW_HEIGHT");
        return std::nullopt;
    }

    if (*width <= 0 || *height <= 0)
    {
        RIALTO_SERVER_LOG_WARN(
            "Ignoring invalid video geometry from environment because width/height must be positive");
        return std::nullopt;
    }

    return firebolt::rialto::server::Rectangle{*x, *y, *width, *height};
}

bool setRenderRectangleProperty(const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                                const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                                GstElement *videoSink, const firebolt::rialto::server::Rectangle &rectangle)
{
    GValue renderRectangle = G_VALUE_INIT;
    glibWrapper->gValueInit(&renderRectangle, GST_TYPE_ARRAY);

    auto appendCoordinate = [&](int coordinate)
    {
        GValue value = G_VALUE_INIT;
        glibWrapper->gValueInit(&value, G_TYPE_INT);
        g_value_set_int(&value, coordinate);
        gstWrapper->gstValueArrayAppendValue(&renderRectangle, &value);
        glibWrapper->gValueUnset(&value);
    };

    appendCoordinate(rectangle.x);
    appendCoordinate(rectangle.y);
    appendCoordinate(rectangle.width);
    appendCoordinate(rectangle.height);

    g_object_set_property(G_OBJECT(videoSink), "render-rectangle", &renderRectangle);
    glibWrapper->gValueUnset(&renderRectangle);
    return true;
}

void applyPlaybinSinkOverride(const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                              const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                              GstElement *pipeline, const char *envName, const char *propertyName)
{
    const char *sinkFactoryName = std::getenv(envName);
    if (!sinkFactoryName || sinkFactoryName[0] == '\0')
    {
        return;
    }

    GstElement *sink = gstWrapper->gstElementFactoryMake(sinkFactoryName, sinkFactoryName);
    if (!sink)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create '%s' from %s", sinkFactoryName, envName);
        return;
    }

    glibWrapper->gObjectSet(pipeline, propertyName, sink, nullptr);
    RIALTO_SERVER_LOG_INFO("Overrode playbin %s with %s from %s", propertyName, sinkFactoryName, envName);
}

bool operator==(const firebolt::rialto::server::SegmentData &lhs, const firebolt::rialto::server::SegmentData &rhs)
{
    return (lhs.position == rhs.position) && (lhs.resetTime == rhs.resetTime) && (lhs.appliedRate == rhs.appliedRate) &&
           (lhs.stopPosition == rhs.stopPosition);
}
} // namespace

namespace firebolt::rialto::server
{
std::weak_ptr<IGstGenericPlayerFactory> GstGenericPlayerFactory::m_factory;

std::shared_ptr<IGstGenericPlayerFactory> IGstGenericPlayerFactory::getFactory()
{
    std::shared_ptr<IGstGenericPlayerFactory> factory = GstGenericPlayerFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<GstGenericPlayerFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer player factory, reason: %s", e.what());
        }

        GstGenericPlayerFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IGstGenericPlayer> GstGenericPlayerFactory::createGstGenericPlayer(
    IGstGenericPlayerClient *client, IDecryptionService &decryptionService, MediaType type,
    const VideoRequirements &videoRequirements, bool isLive,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapperFactory> &rdkGstreamerUtilsWrapperFactory,
    const std::shared_ptr<IGstProfilerFactory> &gstProfilerFactory)
{
    std::unique_ptr<IGstGenericPlayer> gstPlayer;

    try
    {
        auto gstWrapperFactory = firebolt::rialto::wrappers::IGstWrapperFactory::getFactory();
        auto glibWrapperFactory = firebolt::rialto::wrappers::IGlibWrapperFactory::getFactory();
        std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper;
        std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper;
        std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> rdkGstreamerUtilsWrapper;
        if ((!gstWrapperFactory) || (!(gstWrapper = gstWrapperFactory->getGstWrapper())))
        {
            throw std::runtime_error("Cannot create GstWrapper");
        }
        if ((!glibWrapperFactory) || (!(glibWrapper = glibWrapperFactory->getGlibWrapper())))
        {
            throw std::runtime_error("Cannot create GlibWrapper");
        }
        if ((!rdkGstreamerUtilsWrapperFactory) ||
            (!(rdkGstreamerUtilsWrapper = rdkGstreamerUtilsWrapperFactory->createRdkGstreamerUtilsWrapper())))
        {
            throw std::runtime_error("Cannot create RdkGstreamerUtilsWrapper");
        }

        // Fall back to the default profiler factory if the caller explicitly
        // passed nullptr. IGstProfilerFactory::getFactory() can itself return
        // nullptr on allocation failure, so re-check afterwards and fail with
        // a clear error rather than passing null on and reporting the generic
        // "No gst profiler factory provided" message from the constructor.
        auto resolvedGstProfilerFactory = gstProfilerFactory ? gstProfilerFactory : IGstProfilerFactory::getFactory();
        if (!resolvedGstProfilerFactory)
        {
            throw std::runtime_error("Cannot obtain default IGstProfilerFactory");
        }

        gstPlayer = std::make_unique<
            GstGenericPlayer>(client, decryptionService, type, videoRequirements, isLive, gstWrapper, glibWrapper,
                              rdkGstreamerUtilsWrapper, IGstInitialiser::instance(), std::make_unique<FlushWatcher>(),
                              IGstSrcFactory::getFactory(), resolvedGstProfilerFactory,
                              common::ITimerFactory::getFactory(),
                              std::make_unique<GenericPlayerTaskFactory>(client, gstWrapper, glibWrapper,
                                                                         rdkGstreamerUtilsWrapper,
                                                                         IGstTextTrackSinkFactory::createFactory()),
                              std::make_unique<WorkerThreadFactory>(), std::make_unique<GstDispatcherThreadFactory>(),
                              IGstProtectionMetadataHelperFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the gstreamer player, reason: %s", e.what());
    }

    return gstPlayer;
}

GstGenericPlayer::GstGenericPlayer(
    IGstGenericPlayerClient *client, IDecryptionService &decryptionService, MediaType type,
    const VideoRequirements &videoRequirements, bool isLive,
    const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
    const std::shared_ptr<firebolt::rialto::wrappers::IRdkGstreamerUtilsWrapper> &rdkGstreamerUtilsWrapper,
    const IGstInitialiser &gstInitialiser, std::unique_ptr<IFlushWatcher> &&flushWatcher,
    const std::shared_ptr<IGstSrcFactory> &gstSrcFactory,
    const std::shared_ptr<IGstProfilerFactory> &gstProfilerFactory, std::shared_ptr<common::ITimerFactory> timerFactory,
    std::unique_ptr<IGenericPlayerTaskFactory> taskFactory, std::unique_ptr<IWorkerThreadFactory> workerThreadFactory,
    std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory,
    std::shared_ptr<IGstProtectionMetadataHelperFactory> gstProtectionMetadataFactory)
    : m_gstPlayerClient(client), m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper},
      m_rdkGstreamerUtilsWrapper{rdkGstreamerUtilsWrapper}, m_gstProfilerFactory{gstProfilerFactory},
      m_timerFactory{timerFactory}, m_taskFactory{std::move(taskFactory)}, m_flushWatcher{std::move(flushWatcher)}
{
    RIALTO_SERVER_LOG_DEBUG("GstGenericPlayer is constructed.");

    gstInitialiser.waitForInitialisation();

    m_context.isLive = isLive;
    m_context.decryptionService = &decryptionService;

    if ((!gstSrcFactory) || (!(m_context.gstSrc = gstSrcFactory->getGstSrc())))
    {
        throw std::runtime_error("Cannot create GstSrc");
    }
    if (!m_gstProfilerFactory)
    {
        throw std::runtime_error("No gst profiler factory provided");
    }

    if (!timerFactory)
    {
        throw std::runtime_error("TimeFactory is invalid");
    }

    if ((!gstProtectionMetadataFactory) ||
        (!(m_protectionMetadataWrapper = gstProtectionMetadataFactory->createProtectionMetadataWrapper(m_gstWrapper))))
    {
        throw std::runtime_error("Cannot create protection metadata wrapper");
    }

    // Ensure that rialtosrc has been initalised
    m_context.gstSrc->initSrc();

    // Start task thread
    if ((!workerThreadFactory) || (!(m_workerThread = workerThreadFactory->createWorkerThread())))
    {
        throw std::runtime_error("Failed to create the worker thread");
    }

    // Initialise pipeline
    switch (type)
    {
    case MediaType::MSE:
    {
        initMsePipeline();
        break;
    }
    default:
    {
        resetWorkerThread();
        throw std::runtime_error("Media type not supported");
    }
    }

    // Check the video requirements for a limited video.
    // If the video requirements are set to anything lower than the minimum, this playback is assumed to be a secondary
    // video in a dual video scenario.
    if ((kMinPrimaryVideoWidth > videoRequirements.maxWidth) || (kMinPrimaryVideoHeight > videoRequirements.maxHeight))
    {
        RIALTO_SERVER_LOG_MIL("Secondary video playback selected");
        bool westerossinkSecondaryVideoResult = setWesterossinkSecondaryVideo();
        bool ermContextResult = setErmContext();
        if (!westerossinkSecondaryVideoResult && !ermContextResult)
        {
            resetWorkerThread();
            termPipeline();
            throw std::runtime_error("Could not set secondary video");
        }
    }
    else
    {
        RIALTO_SERVER_LOG_MIL("Primary video playback selected");
    }

    m_gstDispatcherThread = gstDispatcherThreadFactory->createGstDispatcherThread(*this, m_context.pipeline,
                                                                                  m_context.flushOnPrerollController,
                                                                                  m_gstWrapper, m_glibWrapper);
}

GstGenericPlayer::~GstGenericPlayer()
{
    RIALTO_SERVER_LOG_DEBUG("GstGenericPlayer is destructed.");
    m_gstDispatcherThread.reset();

    try
    {
        resetWorkerThread();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Exception during resetWorkerThread in destructor: %s", e.what());
    }
    catch (...)
    {
        RIALTO_SERVER_LOG_ERROR("Unknown exception during resetWorkerThread in destructor");
    }

    try
    {
        termPipeline();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Exception during termPipeline in destructor: %s", e.what());
    }
    catch (...)
    {
        RIALTO_SERVER_LOG_ERROR("Unknown exception during termPipeline in destructor");
    }
}

// ---------------- RDKAI prototype (RDKAI-66/67): test bins as playbin "audio-sink" -- DO NOT UPSTREAM ----------------
namespace
{
// Platform sink: RDKAI_SINK_NAME if set, else the first vendor sink found in the registry
const char *rdkaiSinkName()
{
    static const std::string kName = []() -> std::string
    {
        std::string result;
        const char *env = getenv("RDKAI_SINK_NAME");
        if (env && env[0])
        {
            result = env;
        }
        else
        {
            static const char *const kCandidates[] = {"brcmaudiosink", "amlhalasink"};
            for (const char *candidate : kCandidates)
            {
                GstElementFactory *factory = gst_element_factory_find(candidate);
                if (factory)
                {
                    gst_object_unref(factory);
                    result = candidate;
                    break;
                }
            }
        }
        RIALTO_SERVER_LOG_MIL("RDKAI: audio sink element = %s", result.empty() ? "(none found)" : result.c_str());
        return result;
    }();
    return kName.c_str();
}

// Logs the caps that enter the bin (shows whether S16LE reaches it)
GstPadProbeReturn rdkaiCapsProbe(GstPad *, GstPadProbeInfo *info, gpointer)
{
    GstEvent *ev = GST_PAD_PROBE_INFO_EVENT(info);
    if (ev && GST_EVENT_TYPE(ev) == GST_EVENT_CAPS)
    {
        GstCaps *caps = nullptr;
        gst_event_parse_caps(ev, &caps);
        gchar *s = caps ? gst_caps_to_string(caps) : nullptr;
        RIALTO_SERVER_LOG_MIL("RDKAI: caps entering bin = %s", s ? s : "none");
        g_free(s);
    }
    return GST_PAD_PROBE_OK;
}

// RDKAI-66: identity probe, counts buffers and logs every 100th PTS
GstPadProbeReturn rdkaiCountProbe(GstPad *, GstPadProbeInfo *info, gpointer)
{
    static guint64 count{0};
    GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER(info);
    if (buf)
    {
        ++count;
        if (count == 1 || count % 100 == 0)
        {
            RIALTO_SERVER_LOG_MIL("RDKAI-66 passthrough: buffers=%llu pts=%" GST_TIME_FORMAT " size=%u",
                                  static_cast<unsigned long long>(count), GST_TIME_ARGS(GST_BUFFER_PTS(buf)),
                                  static_cast<unsigned>(gst_buffer_get_size(buf)));
        }
    }
    return GST_PAD_PROBE_OK;
}

// RDKAI-66 bin: ghost sink -> identity -> platform sink
GstElement *rdkaiCreatePassthroughAudioBin()
{
    GstElement *bin = gst_bin_new("rdkai_passthrough");
    GstElement *identity = gst_element_factory_make("identity", "rdkai_identity");
    GstElement *sink = gst_element_factory_make(rdkaiSinkName(), nullptr); // default name keeps Rialto's prefix match
    if (!bin || !identity || !sink)
    {
        RIALTO_SERVER_LOG_ERROR("RDKAI-66: failed to create passthrough elements");
        if (bin)
            gst_object_unref(bin);
        if (identity)
            gst_object_unref(identity);
        if (sink)
            gst_object_unref(sink);
        return nullptr;
    }
    gst_bin_add_many(GST_BIN(bin), identity, sink, nullptr);
    if (!gst_element_link(identity, sink))
    {
        RIALTO_SERVER_LOG_ERROR("RDKAI-66: failed to link identity -> sink");
        gst_object_unref(bin);
        return nullptr;
    }
    GstPad *idSink = gst_element_get_static_pad(identity, "sink");
    gst_element_add_pad(bin, gst_ghost_pad_new("sink", idSink));
    gst_pad_add_probe(idSink, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, rdkaiCapsProbe, nullptr, nullptr);
    gst_object_unref(idSink);
    GstPad *idSrc = gst_element_get_static_pad(identity, "src");
    gst_pad_add_probe(idSrc, GST_PAD_PROBE_TYPE_BUFFER, rdkaiCountProbe, nullptr, nullptr);
    gst_object_unref(idSrc);
    return bin;
}

struct RdkaiTapCtx
{
    GstElement *vol{nullptr};
    std::atomic<guint64> buffers{0};
    std::atomic<guint64> bytes{0};
    FILE *dump{nullptr};
    guint64 dumpLimit{0};
};

void rdkaiTapCtxFree(gpointer data, GClosure *)
{
    RdkaiTapCtx *ctx = static_cast<RdkaiTapCtx *>(data);
    RIALTO_SERVER_LOG_MIL("RDKAI-67 bed-tap: closed, total buffers=%llu bytes=%llu",
                          static_cast<unsigned long long>(ctx->buffers.load()),
                          static_cast<unsigned long long>(ctx->bytes.load()));
    if (ctx->dump)
    {
        fclose(ctx->dump);
    }
    delete ctx;
}

// appsink "new-sample": counts buffers, logs caps once, optionally dumps ~30 s of PCM, optional slow-consumer test
GstFlowReturn rdkaiTapNewSample(GstElement *appsink, gpointer userData)
{
    RdkaiTapCtx *ctx = static_cast<RdkaiTapCtx *>(userData);
    GstSample *sample = nullptr;
    g_signal_emit_by_name(appsink, "pull-sample", &sample);
    if (!sample)
    {
        return GST_FLOW_OK;
    }
    GstBuffer *buf = gst_sample_get_buffer(sample);
    const guint64 n = ++ctx->buffers;
    if (buf)
    {
        const guint64 total = (ctx->bytes += gst_buffer_get_size(buf));
        if (ctx->dump)
        {
            if (total <= ctx->dumpLimit)
            {
                GstMapInfo map;
                if (gst_buffer_map(buf, &map, GST_MAP_READ))
                {
                    fwrite(map.data, 1, map.size, ctx->dump);
                    gst_buffer_unmap(buf, &map);
                }
            }
            else
            {
                fclose(ctx->dump);
                ctx->dump = nullptr;
            }
        }
    }
    if (n == 1)
    {
        GstCaps *caps = gst_sample_get_caps(sample);
        gchar *s = caps ? gst_caps_to_string(caps) : nullptr;
        RIALTO_SERVER_LOG_MIL("RDKAI-67 bed-tap: first buffer caps=%s", s ? s : "none");
        g_free(s);
    }
    if (n % 100 == 0)
    {
        gdouble v{-1.0};
        g_object_get(ctx->vol, "volume", &v, nullptr);
        RIALTO_SERVER_LOG_MIL("RDKAI-67 bed-tap: buffers=%llu bytes=%llu orig-vol=%.2f",
                              static_cast<unsigned long long>(n), static_cast<unsigned long long>(ctx->bytes.load()), v);
    }
    gst_sample_unref(sample);
    static const int slowMs = getenv("RDKAI_TAP_SLOW_MS") ? atoi(getenv("RDKAI_TAP_SLOW_MS")) : 0;
    if (slowMs > 0)
    {
        g_usleep(static_cast<gulong>(slowMs) * 1000);
    }
    return GST_FLOW_OK;
}

// Logs every outside change to orig-vol
void rdkaiVolNotify(GObject *obj, GParamSpec *, gpointer)
{
    gdouble v{-1.0};
    gboolean m{FALSE};
    g_object_get(obj, "volume", &v, "mute", &m, nullptr);
    RIALTO_SERVER_LOG_MIL("RDKAI-67: orig-vol changed externally -> volume=%.2f mute=%d", v, static_cast<int>(m));
}

// ---- RDKAI-68: TTS injection (tts-src -> audiomixer), ducking, PTS-anchored to the main audio ----
// The TTS feed is driven by the main path: every main buffer gets a TTS buffer covering the same
// span (silence when no line is active), and main caps/segment/flush/EOS are mirrored, so seeks,
// pauses and per-source flushes stay aligned without any seek handling of our own.
constexpr const char *kTtsDir = "/tmp/edgeai";
constexpr const char *kTtsCtl = "/tmp/edgeai/tts.ctl";
constexpr gint64 kTtsMaxWavBytes = 128 * 1024 * 1024;
constexpr gint64 kTtsCtlPollUs = 200 * 1000;
constexpr int kTtsBlockMs = 10;

struct RdkaiTtsTrack
{
    std::string path;
    int rate{0};
    std::vector<int16_t> pcm; // mono
    std::vector<float> gain;  // orig-vol gain per kTtsBlockMs block
};

double rdkaiEnvDouble(const char *name, double def, double lo, double hi)
{
    const char *v = getenv(name);
    if (!v || !v[0])
        return def;
    const double d = atof(v);
    return (d >= lo && d <= hi) ? d : def;
}

// Dir must be root/self-owned and not group/world-writable; file must resolve inside it
bool rdkaiTtsPathAllowed(const std::string &path, std::string &resolved, std::string &why)
{
    struct stat dst{};
    if (stat(kTtsDir, &dst) != 0 || !S_ISDIR(dst.st_mode) || (dst.st_uid != 0 && dst.st_uid != geteuid()) ||
        (dst.st_mode & (S_IWGRP | S_IWOTH)))
    {
        why = std::string(kTtsDir) + " missing, not owned by root/self, or group/world-writable";
        return false;
    }
    char buf[PATH_MAX];
    if (!realpath(path.c_str(), buf))
    {
        why = "cannot resolve '" + path + "'";
        return false;
    }
    resolved = buf;
    const std::string prefix = std::string(kTtsDir) + "/";
    if (resolved.compare(0, prefix.size(), prefix) != 0)
    {
        why = "'" + resolved + "' is outside " + kTtsDir;
        return false;
    }
    return true;
}

uint32_t rdkaiLe32(const unsigned char *p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

uint16_t rdkaiLe16(const unsigned char *p)
{
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

// Loads a 16-bit PCM WAV (mono/stereo, any rate) as mono and precomputes the ducking envelope
std::shared_ptr<RdkaiTtsTrack> rdkaiLoadTtsWav(const std::string &path, double duck, int fadeMs, std::string &why)
{
    std::string resolved;
    if (!rdkaiTtsPathAllowed(path, resolved, why))
        return nullptr;
    struct stat st{};
    if (stat(resolved.c_str(), &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 44 || st.st_size > kTtsMaxWavBytes)
    {
        why = "'" + resolved + "' is not a regular file of 44 B..128 MB";
        return nullptr;
    }
    std::vector<unsigned char> data(static_cast<size_t>(st.st_size));
    FILE *f = fopen(resolved.c_str(), "rb");
    if (!f)
    {
        why = "cannot open '" + resolved + "'";
        return nullptr;
    }
    const size_t got = fread(data.data(), 1, data.size(), f);
    fclose(f);
    if (got != data.size() || memcmp(data.data(), "RIFF", 4) != 0 || memcmp(data.data() + 8, "WAVE", 4) != 0)
    {
        why = "'" + resolved + "' is not a RIFF/WAVE file";
        return nullptr;
    }
    int channels = 0, rate = 0, bits = 0, fmtTag = 0;
    const unsigned char *pcm = nullptr;
    size_t pcmBytes = 0;
    for (size_t off = 12; off + 8 <= data.size();)
    {
        const uint32_t len = rdkaiLe32(&data[off + 4]);
        const size_t body = off + 8;
        if (len > data.size() - body)
            break;
        if (memcmp(&data[off], "fmt ", 4) == 0 && len >= 16)
        {
            fmtTag = rdkaiLe16(&data[body]);
            channels = rdkaiLe16(&data[body + 2]);
            rate = static_cast<int>(rdkaiLe32(&data[body + 4]));
            bits = rdkaiLe16(&data[body + 14]);
        }
        else if (memcmp(&data[off], "data", 4) == 0)
        {
            pcm = &data[body];
            pcmBytes = len;
        }
        off = body + len + (len & 1u);
    }
    if ((fmtTag != 1 && fmtTag != 0xFFFE) || bits != 16 || channels < 1 || channels > 2 || rate < 8000 ||
        rate > 96000 || !pcm)
    {
        why = "'" + resolved + "': need 16-bit PCM, 1-2 channels, 8-96 kHz (got tag=" + std::to_string(fmtTag) +
              " bits=" + std::to_string(bits) + " ch=" + std::to_string(channels) + " rate=" + std::to_string(rate) +
              ")";
        return nullptr;
    }

    auto track = std::make_shared<RdkaiTtsTrack>();
    track->path = resolved;
    track->rate = rate;
    const size_t frames = pcmBytes / (2u * static_cast<size_t>(channels));
    track->pcm.resize(frames);
    for (size_t i = 0; i < frames; ++i)
    {
        const unsigned char *s = pcm + i * 2u * static_cast<size_t>(channels);
        int v = static_cast<int16_t>(rdkaiLe16(s));
        if (channels == 2)
            v = (v + static_cast<int16_t>(rdkaiLe16(s + 2))) / 2;
        track->pcm[i] = static_cast<int16_t>(v);
    }

    // Envelope: duck where a block has speech (|s| above ~-36 dBFS), linear ramp of fadeMs either side
    const size_t blockFrames = static_cast<size_t>(rate) * kTtsBlockMs / 1000;
    const size_t blocks = (frames + blockFrames - 1) / blockFrames;
    std::vector<int> dist(blocks, INT_MAX / 2);
    for (size_t b = 0; b < blocks; ++b)
    {
        const size_t end = std::min(frames, (b + 1) * blockFrames);
        for (size_t i = b * blockFrames; i < end; ++i)
        {
            if (std::abs(static_cast<int>(track->pcm[i])) > 500)
            {
                dist[b] = 0;
                break;
            }
        }
    }
    for (size_t b = 1; b < blocks; ++b)
        dist[b] = std::min(dist[b], dist[b - 1] + kTtsBlockMs);
    for (size_t b = blocks; b-- > 1;)
        dist[b - 1] = std::min(dist[b - 1], dist[b] + kTtsBlockMs);
    track->gain.resize(blocks);
    for (size_t b = 0; b < blocks; ++b)
    {
        const double t = fadeMs > 0 ? std::min(1.0, static_cast<double>(dist[b]) / fadeMs) : (dist[b] ? 1.0 : 0.0);
        track->gain[b] = static_cast<float>(duck + (1.0 - duck) * t);
    }
    return track;
}

struct RdkaiTtsCtx
{
    GstElement *vol{nullptr};
    GstElement *mixCaps{nullptr};
    GstPad *ttsPad{nullptr};
    double duck{0.25};
    int fadeMs{100};
    gint64 offsetNs{0};

    std::mutex trackLock; // track is swapped by the loader thread
    std::shared_ptr<const RdkaiTtsTrack> track;
    std::thread loader;
    std::atomic<bool> loading{false};

    std::mutex segLock; // main segment is read by the mixer's thread
    GstSegment mainSeg{};
    bool mainSegValid{false};

    // main streaming thread only
    GstAudioInfo mainInfo{};
    bool mainInfoValid{false};
    gdouble lastGain{1.0};
    gint64 lastPollUs{0};
    bool ctlSeen{false};
    struct timespec ctlMtime{};
    ino_t ctlIno{0};
    bool loggedNoTrack{false};
    bool loggedPushError{false};

    // mixer src thread only
    GstSegment mixSeg{};
    bool mixSegValid{false};

    ~RdkaiTtsCtx()
    {
        if (loader.joinable())
            loader.join();
        if (ttsPad)
        {
            gst_pad_set_active(ttsPad, FALSE);
            gst_object_unref(ttsPad);
        }
    }
};

void rdkaiTtsCtxFree(gpointer data)
{
    delete static_cast<std::shared_ptr<RdkaiTtsCtx> *>(data);
}

void rdkaiTtsSetTrack(RdkaiTtsCtx &ctx, std::shared_ptr<const RdkaiTtsTrack> track)
{
    std::lock_guard<std::mutex> lock(ctx.trackLock);
    ctx.track = std::move(track);
}

// Loads off the streaming thread; a load already in progress makes this a no-op until it finishes
void rdkaiTtsStartLoad(RdkaiTtsCtx &ctx, const std::string &path)
{
    if (ctx.loading.exchange(true))
        return;
    if (ctx.loader.joinable())
        ctx.loader.join();
    ctx.loader = std::thread(
        [&ctx, path]()
        {
            std::string why;
            auto track = rdkaiLoadTtsWav(path, ctx.duck, ctx.fadeMs, why);
            if (track)
            {
                RIALTO_SERVER_LOG_MIL("RDKAI-68 tts: loaded '%s' rate=%d frames=%zu (%.1f s)", track->path.c_str(),
                                      track->rate, track->pcm.size(),
                                      static_cast<double>(track->pcm.size()) / track->rate);
                rdkaiTtsSetTrack(ctx, track);
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("RDKAI-68 tts: load failed, keeping current track: %s", why.c_str());
            }
            ctx.loading = false;
        });
}

// Control file: one line, a WAV path under /tmp/edgeai or "off". Only root/self-owned, not group/world-writable.
void rdkaiTtsPollCtl(RdkaiTtsCtx &ctx)
{
    const gint64 now = g_get_monotonic_time();
    if (now - ctx.lastPollUs < kTtsCtlPollUs)
        return;
    ctx.lastPollUs = now;
    struct stat st{};
    if (lstat(kTtsCtl, &st) != 0)
        return;
    if (ctx.ctlSeen && st.st_ino == ctx.ctlIno && st.st_mtim.tv_sec == ctx.ctlMtime.tv_sec &&
        st.st_mtim.tv_nsec == ctx.ctlMtime.tv_nsec)
        return;
    ctx.ctlSeen = true;
    ctx.ctlIno = st.st_ino;
    ctx.ctlMtime = st.st_mtim;
    if (!S_ISREG(st.st_mode) || (st.st_uid != 0 && st.st_uid != geteuid()) || (st.st_mode & (S_IWGRP | S_IWOTH)) ||
        st.st_size > 1024)
    {
        RIALTO_SERVER_LOG_ERROR("RDKAI-68 tts: ignoring %s (not a small regular file owned by root/self, or "
                                "group/world-writable)",
                                kTtsCtl);
        return;
    }
    char line[1100] = {0};
    FILE *f = fopen(kTtsCtl, "r");
    if (!f)
        return;
    const size_t n = fread(line, 1, sizeof(line) - 1, f);
    fclose(f);
    std::string cmd(line, n);
    const auto first = cmd.find_first_not_of(" \t\r\n");
    const auto last = cmd.find_last_not_of(" \t\r\n");
    cmd = (first == std::string::npos) ? std::string() : cmd.substr(first, last - first + 1);
    if (cmd.empty() || cmd == "off")
    {
        RIALTO_SERVER_LOG_MIL("RDKAI-68 tts: control file says off -> original audio");
        rdkaiTtsSetTrack(ctx, nullptr);
        return;
    }
    RIALTO_SERVER_LOG_MIL("RDKAI-68 tts: control file -> '%s'", cmd.c_str());
    rdkaiTtsStartLoad(ctx, cmd);
}

void rdkaiTtsPush(RdkaiTtsCtx &ctx, GstMiniObject *obj)
{
    GstFlowReturn ret = GST_FLOW_OK;
    if (GST_IS_EVENT(obj))
    {
        gst_pad_push_event(ctx.ttsPad, GST_EVENT_CAST(obj));
        return;
    }
    ret = gst_pad_push(ctx.ttsPad, GST_BUFFER_CAST(obj));
    if (ret != GST_FLOW_OK && ret != GST_FLOW_FLUSHING && !ctx.loggedPushError)
    {
        ctx.loggedPushError = true;
        RIALTO_SERVER_LOG_ERROR("RDKAI-68 tts: push into mixer returned %s", gst_flow_get_name(ret));
    }
}

// Builds the TTS buffer for one main buffer and returns the orig-vol gain for it
GstBuffer *rdkaiTtsMakeBuffer(RdkaiTtsCtx &ctx, GstBuffer *main, gdouble &gainOut)
{
    gainOut = 1.0;
    const int rate = GST_AUDIO_INFO_RATE(&ctx.mainInfo);
    const int channels = GST_AUDIO_INFO_CHANNELS(&ctx.mainInfo);
    const gsize frames = gst_buffer_get_size(main) / GST_AUDIO_INFO_BPF(&ctx.mainInfo);
    GstBuffer *out = gst_buffer_new_allocate(nullptr, frames * 2u * static_cast<gsize>(channels), nullptr);
    GST_BUFFER_PTS(out) = GST_BUFFER_PTS(main);
    GST_BUFFER_DURATION(out) = GST_BUFFER_DURATION_IS_VALID(main)
                                   ? GST_BUFFER_DURATION(main)
                                   : gst_util_uint64_scale_int(frames, GST_SECOND, rate);
    if (GST_BUFFER_IS_DISCONT(main))
        GST_BUFFER_FLAG_SET(out, GST_BUFFER_FLAG_DISCONT);

    GstMapInfo map;
    if (!gst_buffer_map(out, &map, GST_MAP_WRITE))
        return out;
    memset(map.data, 0, map.size);

    std::shared_ptr<const RdkaiTtsTrack> track;
    {
        std::lock_guard<std::mutex> lock(ctx.trackLock);
        track = ctx.track;
    }
    guint64 stream = GST_CLOCK_TIME_NONE;
    double segRate = 1.0;
    {
        std::lock_guard<std::mutex> lock(ctx.segLock);
        if (ctx.mainSegValid && GST_BUFFER_PTS_IS_VALID(main))
        {
            stream = gst_segment_to_stream_time(&ctx.mainSeg, GST_FORMAT_TIME, GST_BUFFER_PTS(main));
            segRate = ctx.mainSeg.rate;
        }
    }
    if (!track)
    {
        if (!ctx.loggedNoTrack)
        {
            ctx.loggedNoTrack = true;
            RIALTO_SERVER_LOG_MIL("RDKAI-68 tts: no TTS track loaded, feeding silence");
        }
    }
    else if (stream != GST_CLOCK_TIME_NONE && segRate == 1.0)
    {
        ctx.loggedNoTrack = false;
        const double startSec = (static_cast<double>(stream) - static_cast<double>(ctx.offsetNs)) / GST_SECOND;
        const double step = static_cast<double>(track->rate) / rate;
        const double last = static_cast<double>(track->pcm.size()) - 1.0;
        int16_t *dst = reinterpret_cast<int16_t *>(map.data);
        double pos = startSec * track->rate;
        for (gsize i = 0; i < frames; ++i, pos += step)
        {
            int16_t s = 0;
            if (pos >= 0.0 && pos < last)
            {
                const size_t k = static_cast<size_t>(pos);
                const double frac = pos - static_cast<double>(k);
                s = static_cast<int16_t>(track->pcm[k] + frac * (track->pcm[k + 1] - track->pcm[k]));
            }
            for (int c = 0; c < channels; ++c)
                *dst++ = GINT16_TO_LE(s);
        }
        const double midMs = (startSec * 1000.0) + 500.0 * static_cast<double>(frames) / rate;
        if (midMs >= 0.0)
        {
            const size_t block = static_cast<size_t>(midMs) / kTtsBlockMs;
            if (block < track->gain.size())
                gainOut = track->gain[block];
        }
    }
    gst_buffer_unmap(out, &map);
    return out;
}

// Probe on the tee sink pad: mirrors main events into tts-src and pushes a matching TTS buffer before each main buffer
GstPadProbeReturn rdkaiTtsMainProbe(GstPad *, GstPadProbeInfo *info, gpointer userData)
{
    RdkaiTtsCtx &ctx = **static_cast<std::shared_ptr<RdkaiTtsCtx> *>(userData);
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER)
    {
        GstBuffer *main = GST_PAD_PROBE_INFO_BUFFER(info);
        if (!ctx.mainInfoValid)
            return GST_PAD_PROBE_OK;
        rdkaiTtsPollCtl(ctx);
        gdouble gain = 1.0;
        GstBuffer *tts = rdkaiTtsMakeBuffer(ctx, main, gain);
        if (std::fabs(gain - ctx.lastGain) > 0.001)
        {
            ctx.lastGain = gain;
            g_object_set(ctx.vol, "volume", gain, nullptr); // orig-vol runs right after, in this thread
        }
        rdkaiTtsPush(ctx, GST_MINI_OBJECT_CAST(tts));
        return GST_PAD_PROBE_OK;
    }

    GstEvent *ev = GST_PAD_PROBE_INFO_EVENT(info);
    if (!ev)
        return GST_PAD_PROBE_OK;
    switch (GST_EVENT_TYPE(ev))
    {
    case GST_EVENT_STREAM_START:
    {
        GstEvent *ss = gst_event_new_stream_start("rdkai-tts");
        guint group = 0;
        if (gst_event_parse_group_id(ev, &group))
            gst_event_set_group_id(ss, group);
        rdkaiTtsPush(ctx, GST_MINI_OBJECT_CAST(ss));
        break;
    }
    case GST_EVENT_CAPS:
    {
        GstCaps *caps = nullptr;
        gst_event_parse_caps(ev, &caps);
        GstAudioInfo in;
        if (!caps || !gst_audio_info_from_caps(&in, caps) || GST_AUDIO_INFO_LAYOUT(&in) != GST_AUDIO_LAYOUT_INTERLEAVED)
        {
            gchar *s = caps ? gst_caps_to_string(caps) : nullptr;
            RIALTO_SERVER_LOG_ERROR("RDKAI-68 tts: unsupported main caps %s, TTS disabled", s ? s : "none");
            g_free(s);
            ctx.mainInfoValid = false;
            rdkaiTtsPush(ctx, GST_MINI_OBJECT_CAST(gst_event_new_eos())); // mixer must not wait on tts-src
            break;
        }
        ctx.mainInfo = in;
        ctx.mainInfoValid = true;
        GstAudioInfo ttsInfo;
        gst_audio_info_set_format(&ttsInfo, GST_AUDIO_FORMAT_S16LE, GST_AUDIO_INFO_RATE(&in),
                                  GST_AUDIO_INFO_CHANNELS(&in), in.position);
        GstCaps *ttsCaps = gst_audio_info_to_caps(&ttsInfo);
        rdkaiTtsPush(ctx, GST_MINI_OBJECT_CAST(gst_event_new_caps(ttsCaps)));
        gst_caps_unref(ttsCaps);
        g_object_set(ctx.mixCaps, "caps", caps, nullptr); // mixer outputs what the sink received before
        break;
    }
    case GST_EVENT_SEGMENT:
    {
        const GstSegment *seg = nullptr;
        gst_event_parse_segment(ev, &seg);
        {
            std::lock_guard<std::mutex> lock(ctx.segLock);
            gst_segment_copy_into(seg, &ctx.mainSeg);
            ctx.mainSegValid = true;
        }
        GstEvent *copy = gst_event_new_segment(seg);
        gst_event_set_seqnum(copy, gst_event_get_seqnum(ev));
        rdkaiTtsPush(ctx, GST_MINI_OBJECT_CAST(copy));
        break;
    }
    case GST_EVENT_GAP:
    case GST_EVENT_EOS:
    case GST_EVENT_FLUSH_START:
    case GST_EVENT_FLUSH_STOP:
        if (GST_EVENT_TYPE(ev) == GST_EVENT_FLUSH_STOP)
        {
            std::lock_guard<std::mutex> lock(ctx.segLock);
            ctx.mainSegValid = false;
        }
        rdkaiTtsPush(ctx, GST_MINI_OBJECT_CAST(gst_event_copy(ev)));
        break;
    default:
        break;
    }
    return GST_PAD_PROBE_OK;
}

// Probe on the mixer src pad: re-stamps output onto the main segment, so the platform sink (sync=FALSE,
// hardware A/V sync on Broadcom) sees exactly the PTS and segment it would have seen without the mixer
GstPadProbeReturn rdkaiTtsMixerProbe(GstPad *, GstPadProbeInfo *info, gpointer userData)
{
    RdkaiTtsCtx &ctx = **static_cast<std::shared_ptr<RdkaiTtsCtx> *>(userData);
    GstEvent *ev = (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER) ? nullptr : GST_PAD_PROBE_INFO_EVENT(info);
    if (ev && GST_EVENT_TYPE(ev) == GST_EVENT_SEGMENT)
    {
        const GstSegment *seg = nullptr;
        gst_event_parse_segment(ev, &seg);
        gst_segment_copy_into(seg, &ctx.mixSeg);
        ctx.mixSegValid = true;
    }
    else if (ev && GST_EVENT_TYPE(ev) == GST_EVENT_FLUSH_STOP)
    {
        ctx.mixSegValid = false;
    }
    GstSegment main;
    {
        std::lock_guard<std::mutex> lock(ctx.segLock);
        if (!ctx.mainSegValid)
            return GST_PAD_PROBE_OK;
        gst_segment_copy_into(&ctx.mainSeg, &main);
    }
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_BUFFER)
    {
        GstBuffer *buf = GST_PAD_PROBE_INFO_BUFFER(info);
        if (!ctx.mixSegValid || !GST_BUFFER_PTS_IS_VALID(buf))
            return GST_PAD_PROBE_OK;
        const guint64 running = gst_segment_to_running_time(&ctx.mixSeg, GST_FORMAT_TIME, GST_BUFFER_PTS(buf));
        const guint64 pts = gst_segment_position_from_running_time(&main, GST_FORMAT_TIME, running);
        if (running == GST_CLOCK_TIME_NONE || pts == GST_CLOCK_TIME_NONE || pts == GST_BUFFER_PTS(buf))
            return GST_PAD_PROBE_OK;
        buf = gst_buffer_make_writable(buf);
        GST_BUFFER_PTS(buf) = pts;
        GST_PAD_PROBE_INFO_DATA(info) = buf;
        return GST_PAD_PROBE_OK;
    }
    if (ev && GST_EVENT_TYPE(ev) == GST_EVENT_SEGMENT)
    {
        GstEvent *replacement = gst_event_new_segment(&main);
        gst_event_set_seqnum(replacement, gst_event_get_seqnum(ev));
        gst_event_unref(ev);
        GST_PAD_PROBE_INFO_DATA(info) = replacement;
    }
    return GST_PAD_PROBE_OK;
}

// tts-src answers the mixer's upstream queries/events itself (it has no upstream)
gboolean rdkaiTtsPadQuery(GstPad *pad, GstObject *, GstQuery *query)
{
    switch (GST_QUERY_TYPE(query))
    {
    case GST_QUERY_LATENCY:
        gst_query_set_latency(query, FALSE, 0, GST_CLOCK_TIME_NONE);
        return TRUE;
    case GST_QUERY_CAPS:
    {
        GstCaps *current = gst_pad_get_current_caps(pad);
        GstCaps *caps = current ? current : gst_caps_from_string("audio/x-raw,format=S16LE,layout=interleaved");
        gst_query_set_caps_result(query, caps);
        gst_caps_unref(caps);
        return TRUE;
    }
    case GST_QUERY_ACCEPT_CAPS:
        gst_query_set_accept_caps_result(query, TRUE);
        return TRUE;
    default:
        return FALSE;
    }
}

gboolean rdkaiTtsPadEvent(GstPad *, GstObject *, GstEvent *event)
{
    gst_event_unref(event); // seeks/QoS/reconfigure: the feed follows the main path, nothing to do
    return TRUE;
}

// Adds RDKAI_TTS_LATENCY_MS to the latency the platform sink gets back from upstream (post-query probe)
GstPadProbeReturn rdkaiTtsLatencyProbe(GstPad *, GstPadProbeInfo *info, gpointer userData)
{
    GstQuery *query = GST_PAD_PROBE_INFO_QUERY(info);
    if (!query || GST_QUERY_TYPE(query) != GST_QUERY_LATENCY)
        return GST_PAD_PROBE_OK;
    const GstClockTime extra = static_cast<GstClockTime>(GPOINTER_TO_UINT(userData)) * GST_MSECOND;
    gboolean live{FALSE};
    GstClockTime minLat{0};
    GstClockTime maxLat{GST_CLOCK_TIME_NONE};
    gst_query_parse_latency(query, &live, &minLat, &maxLat);
    gst_query_set_latency(query, live, minLat + extra, GST_CLOCK_TIME_IS_VALID(maxLat) ? maxLat + extra : maxLat);
    static std::atomic<bool> logged{false};
    if (!logged.exchange(true))
    {
        RIALTO_SERVER_LOG_MIL("RDKAI-68 tts: latency query live=%d min=%" GST_TIME_FORMAT " (+%" GST_TIME_FORMAT
                              " processing latency)",
                              static_cast<int>(live), GST_TIME_ARGS(minLat + extra), GST_TIME_ARGS(extra));
    }
    return GST_PAD_PROBE_OK;
}

// Adds audiomixer + tts-src between orig-vol and the sink. Returns false on failure (caller falls back).
bool rdkaiTtsAttach(GstElement *bin, GstElement *tee, GstElement *vol, GstElement *sink)
{
    GstElement *mixer = gst_element_factory_make("audiomixer", "rdkai_mixer");
    GstElement *mixCaps = gst_element_factory_make("capsfilter", "rdkai_mix_caps");
    if (!mixer || !mixCaps)
    {
        RIALTO_SERVER_LOG_ERROR("RDKAI-68 tts: audiomixer/capsfilter not available on this image");
        if (mixer)
            gst_object_unref(mixer);
        if (mixCaps)
            gst_object_unref(mixCaps);
        return false;
    }
    gst_bin_add_many(GST_BIN(bin), mixer, mixCaps, nullptr);
    if (!gst_element_link_many(vol, mixer, mixCaps, sink, nullptr))
    {
        RIALTO_SERVER_LOG_ERROR("RDKAI-68 tts: failed to link orig-vol -> mixer -> sink");
        return false;
    }

    auto ctx = std::make_shared<RdkaiTtsCtx>();
    ctx->vol = vol;
    ctx->mixCaps = mixCaps;
    ctx->duck = rdkaiEnvDouble("RDKAI_TTS_DUCK", 0.25, 0.0, 1.0);
    ctx->fadeMs = static_cast<int>(rdkaiEnvDouble("RDKAI_TTS_FADE_MS", 100, 0, 1000));
    ctx->offsetNs = static_cast<gint64>(rdkaiEnvDouble("RDKAI_TTS_OFFSET_MS", 0, -3600000, 3600000) * GST_MSECOND);
    ctx->ttsPad = gst_pad_new("tts-src", GST_PAD_SRC);
    gst_pad_set_query_function(ctx->ttsPad, rdkaiTtsPadQuery);
    gst_pad_set_event_function(ctx->ttsPad, rdkaiTtsPadEvent);
    gst_pad_set_active(ctx->ttsPad, TRUE);
    GstPad *mixSink = gst_element_get_request_pad(mixer, "sink_%u");
    const GstPadLinkReturn link = mixSink ? gst_pad_link(ctx->ttsPad, mixSink) : GST_PAD_LINK_REFUSED;
    if (mixSink)
        gst_object_unref(mixSink);
    if (link != GST_PAD_LINK_OK)
    {
        RIALTO_SERVER_LOG_ERROR("RDKAI-68 tts: failed to link tts-src to the mixer (%d)", static_cast<int>(link));
        return false;
    }

    GstPad *teeSink = gst_element_get_static_pad(tee, "sink");
    gst_pad_add_probe(teeSink,
                      static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM |
                                                   GST_PAD_PROBE_TYPE_EVENT_FLUSH),
                      rdkaiTtsMainProbe, new std::shared_ptr<RdkaiTtsCtx>(ctx), rdkaiTtsCtxFree);
    gst_object_unref(teeSink);
    GstPad *mixSrc = gst_element_get_static_pad(mixer, "src");
    gst_pad_add_probe(mixSrc,
                      static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM |
                                                   GST_PAD_PROBE_TYPE_EVENT_FLUSH),
                      rdkaiTtsMixerProbe, new std::shared_ptr<RdkaiTtsCtx>(ctx), rdkaiTtsCtxFree);
    gst_object_unref(mixSrc);

    const guint latencyMs = static_cast<guint>(rdkaiEnvDouble("RDKAI_TTS_LATENCY_MS", 0, 0, 10000));
    GstPad *platformSink = gst_element_get_static_pad(sink, "sink");
    if (platformSink)
    {
        gst_pad_add_probe(platformSink,
                          static_cast<GstPadProbeType>(GST_PAD_PROBE_TYPE_QUERY_UPSTREAM | GST_PAD_PROBE_TYPE_PULL),
                          rdkaiTtsLatencyProbe, GUINT_TO_POINTER(latencyMs), nullptr);
        gst_object_unref(platformSink);
    }

    const char *wav = getenv("RDKAI_TTS_WAV");
    if (wav && wav[0])
        rdkaiTtsStartLoad(*ctx, wav);
    RIALTO_SERVER_LOG_MIL("RDKAI-68 tts: mixer attached (duck=%.2f fade=%d ms offset=%lld ms latency=%u ms, "
                          "initial wav=%s, control file=%s)",
                          ctx->duck, ctx->fadeMs, static_cast<long long>(ctx->offsetNs / GST_MSECOND), latencyMs,
                          (wav && wav[0]) ? wav : "(none)", kTtsCtl);
    return true;
}

// RDKAI-67 bin: ghost sink -> [optional audioconvert/resample/S16LE caps] -> tee
//                 tee -> orig-vol -> platform sink                        (main path)
//                 tee -> leaky queue -> convert -> resample -> caps -> appsink "bed-tap"
// RDKAI-68 (withTts): orig-vol -> audiomixer (+ tts-src) -> caps -> platform sink
GstElement *rdkaiCreateTeeAudioBin(bool withTts)
{
    const char *convEnv = getenv("RDKAI_BIN_CONVERT");
    const bool convert = (convEnv && convEnv[0] == '1');

    GstElement *bin = gst_bin_new("rdkai_tee_bin");
    GstElement *tee = gst_element_factory_make("tee", "rdkai_tee");
    GstElement *vol = gst_element_factory_make("volume", "orig-vol");
    GstElement *sink = gst_element_factory_make(rdkaiSinkName(), nullptr); // default name keeps Rialto's prefix match
    GstElement *tq = gst_element_factory_make("queue", "rdkai_tap_queue");
    GstElement *tconv = gst_element_factory_make("audioconvert", "rdkai_tap_conv");
    GstElement *tres = gst_element_factory_make("audioresample", "rdkai_tap_resample");
    GstElement *tcf = gst_element_factory_make("capsfilter", "rdkai_tap_caps");
    GstElement *app = gst_element_factory_make("appsink", "bed-tap");
    GstElement *conv = convert ? gst_element_factory_make("audioconvert", "rdkai_front_conv") : nullptr;
    GstElement *res = convert ? gst_element_factory_make("audioresample", "rdkai_front_resample") : nullptr;
    GstElement *cf = convert ? gst_element_factory_make("capsfilter", "rdkai_front_caps") : nullptr;

    GstElement *all[] = {bin, tee, vol, sink, tq, tconv, tres, tcf, app, conv, res, cf};
    bool ok = bin && tee && vol && sink && tq && tconv && tres && tcf && app && (!convert || (conv && res && cf));
    if (!ok)
    {
        RIALTO_SERVER_LOG_ERROR("RDKAI-67: failed to create tee bin elements (sink=%s)", rdkaiSinkName());
        for (GstElement *e : all)
        {
            if (e)
            {
                gst_object_unref(e);
            }
        }
        return nullptr;
    }

    gst_bin_add_many(GST_BIN(bin), tee, vol, sink, tq, tconv, tres, tcf, app, nullptr);
    if (convert)
    {
        gst_bin_add_many(GST_BIN(bin), conv, res, cf, nullptr);
    }
    bool linked = (withTts ? gst_element_link(tee, vol) : gst_element_link_many(tee, vol, sink, nullptr)) &&
                  gst_element_link_many(tee, tq, tconv, tres, tcf, app, nullptr);
    if (convert)
    {
        GstCaps *frontCaps = gst_caps_from_string("audio/x-raw,format=S16LE,rate=48000,channels=2");
        g_object_set(cf, "caps", frontCaps, nullptr);
        gst_caps_unref(frontCaps);
        linked = linked && gst_element_link_many(conv, res, cf, tee, nullptr);
    }
    if (!linked)
    {
        RIALTO_SERVER_LOG_ERROR("RDKAI-67: failed to link tee bin");
        gst_object_unref(bin);
        return nullptr;
    }
    if (withTts && !rdkaiTtsAttach(bin, tee, vol, sink))
    {
        gst_object_unref(bin);
        return nullptr;
    }

    // Tap format: S16LE mono, rate from RDKAI_TAP_RATE (default 16000, common speech-recognition input)
    int rate = 16000;
    const char *rateEnv = getenv("RDKAI_TAP_RATE");
    if (rateEnv && atoi(rateEnv) >= 8000 && atoi(rateEnv) <= 48000)
    {
        rate = atoi(rateEnv);
    }
    gchar *capsStr = g_strdup_printf("audio/x-raw,format=S16LE,channels=1,rate=%d", rate);
    GstCaps *caps = gst_caps_from_string(capsStr);
    g_object_set(tcf, "caps", caps, nullptr);
    gst_caps_unref(caps);
    g_free(capsStr);

    // The tap must never stall playback: bounded leaky queue + non-blocking appsink that drops when full
    g_object_set(tq, "leaky", 2, "max-size-buffers", 10u, "max-size-bytes", 0u, "max-size-time", (guint64)0, nullptr);
    g_object_set(app, "emit-signals", TRUE, "drop", TRUE, "max-buffers", 8u, "sync", FALSE, "async", FALSE,
                 "enable-last-sample", FALSE, nullptr);

    RdkaiTapCtx *ctx = new RdkaiTapCtx();
    ctx->vol = vol;
    const char *dumpEnv = getenv("RDKAI_TAP_DUMP");
    if (dumpEnv && dumpEnv[0] == '1')
    {
        ctx->dump = fopen("/tmp/rdkai_bedtap.raw", "wb");
        ctx->dumpLimit = static_cast<guint64>(rate) * 2 * 30; // 30 s of S16LE mono
    }
    g_signal_connect_data(app, "new-sample", G_CALLBACK(rdkaiTapNewSample), ctx, rdkaiTapCtxFree,
                          static_cast<GConnectFlags>(0));

    // orig-vol must be settable (manual test) and is left at unity gain
    gdouble v{-1.0};
    gst_util_set_object_arg(G_OBJECT(vol), "volume", "0.8");
    g_object_get(vol, "volume", &v, nullptr);
    RIALTO_SERVER_LOG_MIL("RDKAI-67: orig-vol set test -> %.2f (expect 0.80)", v);
    gst_util_set_object_arg(G_OBJECT(vol), "volume", "1.0");
    g_object_get(vol, "volume", &v, nullptr);
    RIALTO_SERVER_LOG_MIL("RDKAI-67: orig-vol restored -> %.2f (expect 1.00), tap rate=%d, front convert=%d", v, rate,
                          static_cast<int>(convert));
    if (!withTts) // in mode 3 ducking changes orig-vol on every speech line
    {
        g_signal_connect(vol, "notify::volume", G_CALLBACK(rdkaiVolNotify), nullptr);
        g_signal_connect(vol, "notify::mute", G_CALLBACK(rdkaiVolNotify), nullptr);
    }

    GstElement *head = convert ? conv : tee;
    GstPad *headSink = gst_element_get_static_pad(head, "sink");
    gst_element_add_pad(bin, gst_ghost_pad_new("sink", headSink));
    gst_pad_add_probe(headSink, GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM, rdkaiCapsProbe, nullptr, nullptr);
    gst_object_unref(headSink);
    return bin;
}

// RDKAI_PASSTHROUGH=1 -> identity bin, =2 -> tee bin, =3 -> tee bin + TTS mixer (falls back to 2)
GstElement *rdkaiCreateSelectedAudioBin()
{
    const char *mode = getenv("RDKAI_PASSTHROUGH");
    if (mode && mode[0] == '3')
    {
        GstElement *bin = rdkaiCreateTeeAudioBin(true);
        if (bin)
            return bin;
        RIALTO_SERVER_LOG_ERROR("RDKAI-68: TTS bin failed, falling back to mode 2");
        return rdkaiCreateTeeAudioBin(false);
    }
    return (mode && mode[0] == '2') ? rdkaiCreateTeeAudioBin(false) : rdkaiCreatePassthroughAudioBin();
}
} // namespace
// ---------------- end RDKAI ----------------

void GstGenericPlayer::initMsePipeline()
{
    // Make playbin
    m_context.pipeline = m_gstWrapper->gstElementFactoryMake("playbin", "media_pipeline");

    if (const auto defaultGeometry = getDefaultVideoGeometryFromEnvironment())
    {
        m_context.defaultVideoGeometry = *defaultGeometry;
        RIALTO_SERVER_LOG_INFO("Loaded fallback video geometry from environment: x=%d y=%d width=%d height=%d",
                               defaultGeometry->x, defaultGeometry->y, defaultGeometry->width, defaultGeometry->height);
    }

    applyPlaybinSinkOverride(m_gstWrapper, m_glibWrapper, m_context.pipeline, "RIALTO_PLAYBIN_AUDIO_SINK", "audio-sink");
    applyPlaybinSinkOverride(m_gstWrapper, m_glibWrapper, m_context.pipeline, "RIALTO_PLAYBIN_VIDEO_SINK", "video-sink");

    // Set pipeline flags
    setPlaybinFlags(true);

    // RDKAI prototype: splice a test bin as playbin "audio-sink" (RDKAI_PASSTHROUGH=1 identity, =2 tee, =3 tee+TTS)
    const char *rdkaiEnv = getenv("RDKAI_PASSTHROUGH");
    if (rdkaiEnv && (rdkaiEnv[0] == '1' || rdkaiEnv[0] == '2' || rdkaiEnv[0] == '3'))
    {
        GstElement *rdkaiBin = rdkaiCreateSelectedAudioBin();
        if (rdkaiBin)
        {
            g_object_set(m_context.pipeline, "audio-sink", rdkaiBin, nullptr);
            RIALTO_SERVER_LOG_MIL("RDKAI: test audio-sink bin installed (mode %c)", rdkaiEnv[0]);
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("RDKAI: test bin creation failed, using default audio sink");
        }
    }

    m_context.gstProfiler = m_gstProfilerFactory->createGstProfiler(m_context.pipeline, m_gstWrapper, m_glibWrapper);
    if (!m_context.gstProfiler)
    {
        throw std::runtime_error("Cannot create GstProfiler");
    }

    // Set callbacks
    m_glibWrapper->gSignalConnect(m_context.pipeline, "source-setup", G_CALLBACK(&GstGenericPlayer::setupSource), this);
    m_glibWrapper->gSignalConnect(m_context.pipeline, "element-setup", G_CALLBACK(&GstGenericPlayer::setupElement), this);
    m_glibWrapper->gSignalConnect(m_context.pipeline, "deep-element-added",
                                  G_CALLBACK(&GstGenericPlayer::deepElementAdded), this);

    // Set uri
    m_glibWrapper->gObjectSet(m_context.pipeline, "uri", "rialto://", nullptr);

    // Check playsink
    GstElement *playsink = (m_gstWrapper->gstBinGetByName(GST_BIN(m_context.pipeline), "playsink"));
    if (playsink)
    {
        m_glibWrapper->gObjectSet(G_OBJECT(playsink), "send-event-mode", 0, nullptr);
        m_gstWrapper->gstObjectUnref(playsink);
    }
    else
    {
        GST_WARNING("No playsink ?!?!?");
    }
    if (GST_STATE_CHANGE_FAILURE == m_gstWrapper->gstElementSetState(m_context.pipeline, GST_STATE_READY))
    {
        GST_WARNING("Failed to set pipeline to READY state");
    }
    RIALTO_SERVER_LOG_MIL("New RialtoServer's pipeline created");
    auto recordId = m_context.gstProfiler->createRecord("Pipeline Created");
    if (recordId)
        m_context.gstProfiler->logRecord(recordId.value());
}

void GstGenericPlayer::resetWorkerThread()
{
    // Shutdown task thread
    m_workerThread->enqueueTask(m_taskFactory->createShutdown(*this));
    m_workerThread->join();
    m_workerThread.reset();
}

void GstGenericPlayer::termPipeline()
{
    if (m_finishSourceSetupTimer && m_finishSourceSetupTimer->isActive())
    {
        m_finishSourceSetupTimer->cancel();
    }

    m_finishSourceSetupTimer.reset();

    clearAudioFirstFrameFallbackProbe();
    stopNotifyPlaybackInfoTimer();

    for (auto &elem : m_context.streamInfo)
    {
        StreamInfo &streamInfo = elem.second;
        for (auto &buffer : streamInfo.buffers)
        {
            m_gstWrapper->gstBufferUnref(buffer);
        }

        streamInfo.buffers.clear();
    }

    m_taskFactory->createStop(m_context, *this)->execute();
    GstBus *bus = m_gstWrapper->gstPipelineGetBus(GST_PIPELINE(m_context.pipeline));
    m_gstWrapper->gstBusSetSyncHandler(bus, nullptr, nullptr, nullptr);
    m_gstWrapper->gstObjectUnref(bus);

    if (m_context.source)
    {
        m_gstWrapper->gstObjectUnref(m_context.source);
    }
    if (m_context.subtitleSink)
    {
        m_gstWrapper->gstObjectUnref(m_context.subtitleSink);
        m_context.subtitleSink = nullptr;
    }

    if (m_context.videoSink)
    {
        m_gstWrapper->gstObjectUnref(m_context.videoSink);
        m_context.videoSink = nullptr;
    }
    if (m_context.playbackGroup.m_curAudioPlaysinkBin)
    {
        m_gstWrapper->gstObjectUnref(m_context.playbackGroup.m_curAudioPlaysinkBin);
        m_context.playbackGroup.m_curAudioPlaysinkBin = nullptr;
    }

    auto recordId = m_context.gstProfiler->createRecord("Pipeline Terminated");
    if (recordId)
        m_context.gstProfiler->logRecord(recordId.value());
    m_context.gstProfiler->dumpToFile();

    // Delete the pipeline
    m_gstWrapper->gstObjectUnref(m_context.pipeline);

    m_glibWrapper->gThreadPoolStopUnusedThreads();
    malloc_trim(0);

    RIALTO_SERVER_LOG_MIL("RialtoServer's pipeline terminated");
}

unsigned GstGenericPlayer::getGstPlayFlag(const char *nick)
{
    GFlagsClass *flagsClass =
        static_cast<GFlagsClass *>(m_glibWrapper->gTypeClassRef(m_glibWrapper->gTypeFromName("GstPlayFlags")));
    GFlagsValue *flag = m_glibWrapper->gFlagsGetValueByNick(flagsClass, nick);
    unsigned result = flag ? flag->value : 0;
    m_glibWrapper->gTypeClassUnref(flagsClass);
    return result;
}

void GstGenericPlayer::setupSource(GstElement *pipeline, GstElement *source, GstGenericPlayer *self)
{
    self->m_gstWrapper->gstObjectRef(source);
    if (self->m_workerThread)
    {
        self->m_workerThread->enqueueTask(self->m_taskFactory->createSetupSource(self->m_context, *self, source));
    }
}

void GstGenericPlayer::setupElement(GstElement *pipeline, GstElement *element, GstGenericPlayer *self)
{
    RIALTO_SERVER_LOG_DEBUG("Element %s added to the pipeline", GST_ELEMENT_NAME(element));
    self->m_gstWrapper->gstObjectRef(element);
    if (self->m_workerThread)
    {
        self->m_workerThread->enqueueTask(self->m_taskFactory->createSetupElement(self->m_context, *self, element));
    }
}

void GstGenericPlayer::deepElementAdded(GstBin *pipeline, GstBin *bin, GstElement *element, GstGenericPlayer *self)
{
    RIALTO_SERVER_LOG_DEBUG("Deep element %s added to the pipeline", GST_ELEMENT_NAME(element));
    if (self->m_workerThread)
    {
        self->m_gstWrapper->gstObjectRef(element);
        self->m_workerThread->enqueueTask(
            self->m_taskFactory->createDeepElementAdded(self->m_context, *self, pipeline, bin, element));
    }
}

void GstGenericPlayer::attachSource(const std::unique_ptr<IMediaPipeline::MediaSource> &attachedSource)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createAttachSource(m_context, *this, attachedSource));
    }
}

void GstGenericPlayer::removeSource(const MediaSourceType &mediaSourceType)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createRemoveSource(m_context, *this, mediaSourceType));
    }
}

void GstGenericPlayer::allSourcesAttached()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createFinishSetupSource(m_context, *this));
    }
}

void GstGenericPlayer::attachSamples(const IMediaPipeline::MediaSegmentVector &mediaSegments)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createAttachSamples(m_context, *this, mediaSegments));
    }
}

void GstGenericPlayer::attachSamples(const std::shared_ptr<IDataReader> &dataReader)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createReadShmDataAndAttachSamples(m_context, *this, dataReader));
    }
}

void GstGenericPlayer::setPosition(std::int64_t position)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetPosition(m_context, *this, position));
    }
}

void GstGenericPlayer::setPlaybackRate(double rate)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetPlaybackRate(m_context, rate));
    }
}

bool GstGenericPlayer::getPosition(std::int64_t &position)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    position = getPosition(m_context.pipeline);
    if (position == -1)
    {
        return false;
    }

    return true;
}

bool GstGenericPlayer::getDuration(std::int64_t &duration)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    if (!m_context.pipeline || !m_gstWrapper->gstElementQueryDuration(m_context.pipeline, GST_FORMAT_TIME, &duration))
    {
        RIALTO_SERVER_LOG_WARN("Failed to query duration");
        return false;
    }
    return true;
}

GstElement *GstGenericPlayer::getSink(const MediaSourceType &mediaSourceType) const
{
    const char *kSinkName{nullptr};
    GstElement *sink{nullptr};
    switch (mediaSourceType)
    {
    case MediaSourceType::AUDIO:
        kSinkName = "audio-sink";
        break;
    case MediaSourceType::VIDEO:
        kSinkName = "video-sink";
        break;
    case MediaSourceType::SUBTITLE:
        kSinkName = "text-sink";
        break;
    default:
        break;
    }
    if (!kSinkName)
    {
        RIALTO_SERVER_LOG_WARN("mediaSourceType not supported %d", static_cast<int>(mediaSourceType));
    }
    else
    {
        if (m_context.pipeline == nullptr)
        {
            RIALTO_SERVER_LOG_WARN("Pipeline is NULL!");
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pipeline is valid: %p", m_context.pipeline);
        }
        m_glibWrapper->gObjectGet(m_context.pipeline, kSinkName, &sink, nullptr);
        if (sink && firebolt::rialto::MediaSourceType::SUBTITLE != mediaSourceType)
        {
            GstElement *autoSink{sink};
            if (firebolt::rialto::MediaSourceType::VIDEO == mediaSourceType)
                autoSink = getSinkChildIfAutoVideoSink(sink);
            else if (firebolt::rialto::MediaSourceType::AUDIO == mediaSourceType)
                autoSink = getSinkChildIfAutoAudioSink(sink);

            // Is this an auto-sink?...
            if (autoSink != sink)
            {
                m_gstWrapper->gstObjectUnref(GST_OBJECT(sink));

                // increase the reference count of the auto sink
                sink = GST_ELEMENT(m_gstWrapper->gstObjectRef(GST_OBJECT(autoSink)));
            }
        }
    }
    return sink;
}

void GstGenericPlayer::setSourceFlushed(const MediaSourceType &mediaSourceType)
{
    m_flushWatcher->setFlushed(mediaSourceType);
}

void GstGenericPlayer::notifyPlaybackInfo()
{
    PlaybackInfo info;
    getPosition(info.currentPosition);
    m_context.streamPosition.store(info.currentPosition);
    if (m_context.audioFadeEnabled)
    {
        info.volume = m_context.audioFadeVolume;
    }
    else
    {
        getVolume(info.volume);
    }
    m_gstPlayerClient->notifyPlaybackInfo(info);
}

GstElement *GstGenericPlayer::getDecoder(const MediaSourceType &mediaSourceType)
{
    GstIterator *it = m_gstWrapper->gstBinIterateRecurse(GST_BIN(m_context.pipeline));
    GValue item = G_VALUE_INIT;
    gboolean done = FALSE;

    while (!done)
    {
        switch (m_gstWrapper->gstIteratorNext(it, &item))
        {
        case GST_ITERATOR_OK:
        {
            GstElement *element = GST_ELEMENT(m_glibWrapper->gValueGetObject(&item));
            GstElementFactory *factory = m_gstWrapper->gstElementGetFactory(element);

            if (factory)
            {
                GstElementFactoryListType type = GST_ELEMENT_FACTORY_TYPE_DECODER;
                if (mediaSourceType == MediaSourceType::AUDIO)
                {
                    type |= GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO;
                }
                else if (mediaSourceType == MediaSourceType::VIDEO)
                {
                    type |= GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO;
                }

                if (m_gstWrapper->gstElementFactoryListIsType(factory, type))
                {
                    m_glibWrapper->gValueUnset(&item);
                    m_gstWrapper->gstIteratorFree(it);
                    return GST_ELEMENT(m_gstWrapper->gstObjectRef(element));
                }
            }

            m_glibWrapper->gValueUnset(&item);
            break;
        }
        case GST_ITERATOR_RESYNC:
            m_gstWrapper->gstIteratorResync(it);
            break;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        }
    }

    RIALTO_SERVER_LOG_WARN("Could not find decoder");

    m_glibWrapper->gValueUnset(&item);
    m_gstWrapper->gstIteratorFree(it);

    return nullptr;
}

GstElement *GstGenericPlayer::getParser(const MediaSourceType &mediaSourceType)
{
    GstIterator *it = m_gstWrapper->gstBinIterateRecurse(GST_BIN(m_context.pipeline));
    GValue item = G_VALUE_INIT;
    gboolean done = FALSE;

    while (!done)
    {
        switch (m_gstWrapper->gstIteratorNext(it, &item))
        {
        case GST_ITERATOR_OK:
        {
            GstElement *element = GST_ELEMENT(m_glibWrapper->gValueGetObject(&item));
            GstElementFactory *factory = m_gstWrapper->gstElementGetFactory(element);

            if (factory)
            {
                GstElementFactoryListType type = GST_ELEMENT_FACTORY_TYPE_PARSER;
                if (mediaSourceType == MediaSourceType::AUDIO)
                {
                    type |= GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO;
                }
                else if (mediaSourceType == MediaSourceType::VIDEO)
                {
                    type |= GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO;
                }

                if (m_gstWrapper->gstElementFactoryListIsType(factory, type))
                {
                    m_glibWrapper->gValueUnset(&item);
                    m_gstWrapper->gstIteratorFree(it);
                    return GST_ELEMENT(m_gstWrapper->gstObjectRef(element));
                }
            }

            m_glibWrapper->gValueUnset(&item);
            break;
        }
        case GST_ITERATOR_RESYNC:
            m_gstWrapper->gstIteratorResync(it);
            break;
        case GST_ITERATOR_ERROR:
        case GST_ITERATOR_DONE:
            done = TRUE;
            break;
        }
    }

    RIALTO_SERVER_LOG_WARN("Could not find parser");

    m_glibWrapper->gValueUnset(&item);
    m_gstWrapper->gstIteratorFree(it);

    return nullptr;
}

std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate>
GstGenericPlayer::createAudioAttributes(const std::unique_ptr<IMediaPipeline::MediaSource> &source) const
{
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes;
    const IMediaPipeline::MediaSourceAudio *kSource = dynamic_cast<IMediaPipeline::MediaSourceAudio *>(source.get());
    if (kSource)
    {
        firebolt::rialto::AudioConfig audioConfig = kSource->getAudioConfig();
        audioAttributes =
            firebolt::rialto::wrappers::AudioAttributesPrivate{"", // param set below.
                                                               audioConfig.numberOfChannels, audioConfig.sampleRate,
                                                               0, // used only in one of logs in rdk_gstreamer_utils, no
                                                                  // need to set this param.
                                                               0, // used only in one of logs in rdk_gstreamer_utils, no
                                                                  // need to set this param.
                                                               audioConfig.codecSpecificConfig.data(),
                                                               static_cast<std::uint32_t>(
                                                                   audioConfig.codecSpecificConfig.size())};
        if (source->getMimeType() == "audio/mp4" || source->getMimeType() == "audio/aac")
        {
            audioAttributes->m_codecParam = "mp4a";
        }
        else if (source->getMimeType() == "audio/x-eac3")
        {
            audioAttributes->m_codecParam = "ec-3";
        }
        else if (source->getMimeType() == "audio/b-wav" || source->getMimeType() == "audio/x-raw")
        {
            audioAttributes->m_codecParam = "lpcm";
        }
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to cast source");
    }

    return audioAttributes;
}

void GstGenericPlayer::configAudioCap(firebolt::rialto::wrappers::AudioAttributesPrivate *pAttrib, bool *audioaac,
                                      bool svpenabled, GstCaps **appsrcCaps)
{
    // this function comes from rdk_gstreamer_utils
    if (!pAttrib || !audioaac || !appsrcCaps)
    {
        RIALTO_SERVER_LOG_ERROR("configAudioCap: invalid null parameter");
        return;
    }
    gchar *capsString;
    RIALTO_SERVER_LOG_DEBUG("Config audio codec %s sampling rate %d channel %d alignment %d",
                            pAttrib->m_codecParam.c_str(), pAttrib->m_samplesPerSecond, pAttrib->m_numberOfChannels,
                            pAttrib->m_blockAlignment);
    if (pAttrib->m_codecParam.compare(0, 4, std::string("mp4a")) == 0)
    {
        RIALTO_SERVER_LOG_DEBUG("Using AAC");
        capsString = m_glibWrapper->gStrdupPrintf("audio/mpeg, mpegversion=4, enable-svp=(string)%s",
                                                  svpenabled ? "true" : "false");
        *audioaac = true;
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Using EAC3");
        capsString = m_glibWrapper->gStrdupPrintf("audio/x-eac3, framed=(boolean)true, rate=(int)%u, channels=(int)%u, "
                                                  "alignment=(string)frame, enable-svp=(string)%s",
                                                  pAttrib->m_samplesPerSecond, pAttrib->m_numberOfChannels,
                                                  svpenabled ? "true" : "false");
        *audioaac = false;
    }
    *appsrcCaps = m_gstWrapper->gstCapsFromString(capsString);
    m_glibWrapper->gFree(capsString);
}

void GstGenericPlayer::haltAudioPlayback()
{
    // this function comes from rdk_gstreamer_utils
    if (!m_context.playbackGroup.m_curAudioPlaysinkBin || !m_context.playbackGroup.m_curAudioDecodeBin)
    {
        RIALTO_SERVER_LOG_ERROR("haltAudioPlayback: audio playsink bin or decode bin is null");
        return;
    }
    GstState currentState{GST_STATE_VOID_PENDING}, pending{GST_STATE_VOID_PENDING};

    // Transition Playsink to Ready
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioPlaysinkBin, GST_STATE_READY))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set AudioPlaysinkBin to READY");
        return;
    }
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioPlaysinkBin, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_PAUSED)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioPlaySinkBin State = %d", currentState);
    // Transition Decodebin to Paused
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioDecodeBin, GST_STATE_PAUSED))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set AudioDecodeBin to PAUSED");
        return;
    }
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioDecodeBin, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_PAUSED)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current DecodeBin State = %d", currentState);
}

void GstGenericPlayer::resumeAudioPlayback()
{
    // this function comes from rdk_gstreamer_utils
    if (!m_context.playbackGroup.m_curAudioPlaysinkBin || !m_context.playbackGroup.m_curAudioDecodeBin)
    {
        RIALTO_SERVER_LOG_ERROR("resumeAudioPlayback: audio playsink bin or decode bin is null");
        return;
    }
    GstState currentState{GST_STATE_VOID_PENDING}, pending{GST_STATE_VOID_PENDING};
    m_gstWrapper->gstElementSyncStateWithParent(m_context.playbackGroup.m_curAudioPlaysinkBin);
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioPlaysinkBin, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> AudioPlaysinkbin State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstElementSyncStateWithParent(m_context.playbackGroup.m_curAudioDecodeBin);
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioDecodeBin, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> Decodebin State = %d Pending = %d", currentState, pending);
}

void GstGenericPlayer::firstTimeSwitchFromAC3toAAC(GstCaps *newAudioCaps)
{
    // this function comes from rdk_gstreamer_utils
    if (!m_context.playbackGroup.m_curAudioTypefind || !m_context.playbackGroup.m_curAudioDecodeBin)
    {
        RIALTO_SERVER_LOG_ERROR("firstTimeSwitchFromAC3toAAC: audio typefind or decode bin is null");
        return;
    }
    GstState currentState{GST_STATE_VOID_PENDING}, pending{GST_STATE_VOID_PENDING};
    GstPad *pTypfdSrcPad = NULL;
    GstPad *pTypfdSrcPeerPad = NULL;
    GstPad *pNewAudioDecoderSrcPad = NULL;
    GstElement *newAudioParse = NULL;
    GstElement *newAudioDecoder = NULL;
    GstElement *newQueue = NULL;
    gboolean linkRet = false;

    /* Get the SinkPad of ASink - pTypfdSrcPeerPad */
    if ((pTypfdSrcPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioTypefind, "src")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current Typefind SrcPad = %p", pTypfdSrcPad);
    if ((pTypfdSrcPeerPad = m_gstWrapper->gstPadGetPeer(pTypfdSrcPad)) != NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current Typefind Src Downstream Element Pad = %p", pTypfdSrcPeerPad);
    // AudioDecoder Downstream Unlink
    if (m_gstWrapper->gstPadUnlink(pTypfdSrcPad, pTypfdSrcPeerPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Downstream Unlink Failed");
    newAudioParse = m_gstWrapper->gstElementFactoryMake("aacparse", "aacparse");
    newAudioDecoder = m_gstWrapper->gstElementFactoryMake("avdec_aac", "avdec_aac");
    newQueue = m_gstWrapper->gstElementFactoryMake("queue", "aqueue");
    // Add new Decoder to Decodebin
    if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newAudioDecoder) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Added New AudioDecoder = %p", newAudioDecoder);
    }
    // Add new Parser to Decodebin
    if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newAudioParse) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Added New AudioParser = %p", newAudioParse);
    }
    // Add new Queue to Decodebin
    if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newQueue) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Added New queue = %p", newQueue);
    }
    if ((pNewAudioDecoderSrcPad = m_gstWrapper->gstElementGetStaticPad(newAudioDecoder, "src")) != NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Src Pad = %p", pNewAudioDecoderSrcPad);
    // Connect decoder to ASINK
    if (m_gstWrapper->gstPadLink(pNewAudioDecoderSrcPad, pTypfdSrcPeerPad) != GST_PAD_LINK_OK)
        RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Downstream Link Failed");
    linkRet = m_gstWrapper->gstElementLink(newAudioParse, newQueue) &&
              m_gstWrapper->gstElementLink(newQueue, newAudioDecoder);
    if (!linkRet)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Downstream Link Failed for typefind, parser, decoder");
    /* Force Caps */
    RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Setting to READY");
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioTypefind, GST_STATE_READY))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set Typefind to READY");
        m_gstWrapper->gstObjectUnref(pTypfdSrcPad);
        m_gstWrapper->gstObjectUnref(pTypfdSrcPeerPad);
        m_gstWrapper->gstObjectUnref(pNewAudioDecoderSrcPad);
        return;
    }
    m_glibWrapper->gObjectSet(G_OBJECT(m_context.playbackGroup.m_curAudioTypefind), "force-caps", newAudioCaps, NULL);
    m_gstWrapper->gstElementSyncStateWithParent(m_context.playbackGroup.m_curAudioTypefind);
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioTypefind, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New Typefind State = %d Pending = %d", currentState, pending);
    RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Syncing with Parent");
    m_context.playbackGroup.m_linkTypefindParser = true;
    /* Update the state */
    m_gstWrapper->gstElementSyncStateWithParent(newAudioDecoder);
    m_gstWrapper->gstElementGetState(newAudioDecoder, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstElementSyncStateWithParent(newQueue);
    m_gstWrapper->gstElementGetState(newQueue, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New queue State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstElementSyncStateWithParent(newAudioParse);
    m_gstWrapper->gstElementGetState(newAudioParse, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstObjectUnref(pTypfdSrcPad);
    m_gstWrapper->gstObjectUnref(pTypfdSrcPeerPad);
    m_gstWrapper->gstObjectUnref(pNewAudioDecoderSrcPad);
    return;
}

bool GstGenericPlayer::switchAudioCodec(bool isAudioAAC, GstCaps *newAudioCaps)
{ // this function comes from rdk_gstreamer_utils
    bool ret = false;
    RIALTO_SERVER_LOG_DEBUG("Current Audio Codec AAC = %d Same as Incoming audio Codec AAC = %d",
                            m_context.playbackGroup.m_isAudioAAC, isAudioAAC);
    if (m_context.playbackGroup.m_isAudioAAC == isAudioAAC)
    {
        return ret;
    }
    if ((m_context.playbackGroup.m_curAudioDecoder == NULL) && (!(m_context.playbackGroup.m_isAudioAAC)) && (isAudioAAC))
    {
        firstTimeSwitchFromAC3toAAC(newAudioCaps);
        m_context.playbackGroup.m_isAudioAAC = isAudioAAC;
        return true;
    }
    if (!m_context.playbackGroup.m_curAudioDecoder || !m_context.playbackGroup.m_curAudioParse ||
        !m_context.playbackGroup.m_curAudioDecodeBin)
    {
        RIALTO_SERVER_LOG_ERROR("switchAudioCodec: audio decoder, parser or decode bin is null");
        return false;
    }
    GstElement *newAudioParse = NULL;
    GstElement *newAudioDecoder = NULL;
    GstPad *newAudioParseSrcPad = NULL;
    GstPad *newAudioParseSinkPad = NULL;
    GstPad *newAudioDecoderSrcPad = NULL;
    GstPad *newAudioDecoderSinkPad = NULL;
    GstPad *audioDecSrcPad = NULL;
    GstPad *audioDecSinkPad = NULL;
    GstPad *audioDecSrcPeerPad = NULL;
    GstPad *audioDecSinkPeerPad = NULL;
    GstPad *audioParseSrcPad = NULL;
    GstPad *audioParseSinkPad = NULL;
    GstPad *audioParseSrcPeerPad = NULL;
    GstPad *audioParseSinkPeerPad = NULL;
    GstState currentState{GST_STATE_VOID_PENDING}, pending{GST_STATE_VOID_PENDING};

    // Get AudioDecoder Src Pads
    if ((audioDecSrcPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioDecoder, "src")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder Src Pad = %p", audioDecSrcPad);
    // Get AudioDecoder Sink Pads
    if ((audioDecSinkPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioDecoder, "sink")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder Sink Pad = %p", audioDecSinkPad);
    // Get AudioDecoder Src Peer i.e. Downstream Element Pad
    if ((audioDecSrcPeerPad = m_gstWrapper->gstPadGetPeer(audioDecSrcPad)) != NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder Src Downstream Element Pad = %p", audioDecSrcPeerPad);
    // Get AudioDecoder Sink Peer i.e. Upstream Element Pad
    if ((audioDecSinkPeerPad = m_gstWrapper->gstPadGetPeer(audioDecSinkPad)) != NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder Sink Upstream Element Pad = %p", audioDecSinkPeerPad);
    // Get AudioParser Src Pads
    if ((audioParseSrcPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioParse, "src")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser Src Pad = %p", audioParseSrcPad);
    // Get AudioParser Sink Pads
    if ((audioParseSinkPad = m_gstWrapper->gstElementGetStaticPad(m_context.playbackGroup.m_curAudioParse, "sink")) !=
        NULL) // Unref the Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser Sink Pad = %p", audioParseSinkPad);
    // Get AudioParser Src Peer i.e. Downstream Element Pad
    if ((audioParseSrcPeerPad = m_gstWrapper->gstPadGetPeer(audioParseSrcPad)) != NULL) // Unref the Peer Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser Src Downstream Element Pad = %p", audioParseSrcPeerPad);
    // Get AudioParser Sink Peer i.e. Upstream Element Pad
    if ((audioParseSinkPeerPad = m_gstWrapper->gstPadGetPeer(audioParseSinkPad)) != NULL) // Unref the Peer Pad
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser Sink Upstream Element Pad = %p", audioParseSinkPeerPad);
    // AudioDecoder Downstream Unlink
    if (m_gstWrapper->gstPadUnlink(audioDecSrcPad, audioDecSrcPeerPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> AudioDecoder Downstream Unlink Failed");
    // AudioDecoder Upstream Unlink
    if (m_gstWrapper->gstPadUnlink(audioDecSinkPeerPad, audioDecSinkPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> AudioDecoder Upstream Unlink Failed");
    // AudioParser Downstream Unlink
    if (m_gstWrapper->gstPadUnlink(audioParseSrcPad, audioParseSrcPeerPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> AudioParser Downstream Unlink Failed");
    // AudioParser Upstream Unlink
    if (m_gstWrapper->gstPadUnlink(audioParseSinkPeerPad, audioParseSinkPad) == FALSE)
        RIALTO_SERVER_LOG_DEBUG("OTF -> AudioParser Upstream Unlink Failed");
    // Current Audio Decoder NULL
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioDecoder, GST_STATE_NULL))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set AudioDecoder to NULL");
    }
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioDecoder, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_NULL)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioDecoder State = %d", currentState);
    // Current Audio Parser NULL
    if (GST_STATE_CHANGE_FAILURE ==
        m_gstWrapper->gstElementSetState(m_context.playbackGroup.m_curAudioParse, GST_STATE_NULL))
    {
        RIALTO_SERVER_LOG_WARN("Failed to set AudioParser to NULL");
    }
    m_gstWrapper->gstElementGetState(m_context.playbackGroup.m_curAudioParse, &currentState, &pending,
                                     GST_CLOCK_TIME_NONE);
    if (currentState == GST_STATE_NULL)
        RIALTO_SERVER_LOG_DEBUG("OTF -> Current AudioParser State = %d", currentState);
    // Remove Audio Decoder From Decodebin
    if (m_gstWrapper->gstBinRemove(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()),
                                   m_context.playbackGroup.m_curAudioDecoder) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Removed AudioDecoder = %p", m_context.playbackGroup.m_curAudioDecoder);
        m_context.playbackGroup.m_curAudioDecoder = NULL;
    }
    // Remove Audio Parser From Decodebin
    if (m_gstWrapper->gstBinRemove(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()),
                                   m_context.playbackGroup.m_curAudioParse) == TRUE)
    {
        RIALTO_SERVER_LOG_DEBUG("OTF -> Removed AudioParser = %p", m_context.playbackGroup.m_curAudioParse);
        m_context.playbackGroup.m_curAudioParse = NULL;
    }
    // Create new Audio Decoder and Parser. The inverse of the current
    if (m_context.playbackGroup.m_isAudioAAC)
    {
        newAudioParse = m_gstWrapper->gstElementFactoryMake("ac3parse", "ac3parse");
        newAudioDecoder = m_gstWrapper->gstElementFactoryMake("identity", "fake_aud_ac3dec");
    }
    else
    {
        newAudioParse = m_gstWrapper->gstElementFactoryMake("aacparse", "aacparse");
        newAudioDecoder = m_gstWrapper->gstElementFactoryMake("avdec_aac", "avdec_aac");
    }
    {
        GstPadLinkReturn gstPadLinkRet = GST_PAD_LINK_OK;
        GstElement *audioParseUpstreamEl = NULL;
        // Add new Decoder to Decodebin
        if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newAudioDecoder) == TRUE)
        {
            RIALTO_SERVER_LOG_DEBUG("OTF -> Added New AudioDecoder = %p", newAudioDecoder);
        }
        // Add new Parser to Decodebin
        if (m_gstWrapper->gstBinAdd(GST_BIN(m_context.playbackGroup.m_curAudioDecodeBin.load()), newAudioParse) == TRUE)
        {
            RIALTO_SERVER_LOG_DEBUG("OTF -> Added New AudioParser = %p", newAudioParse);
        }
        if ((newAudioDecoderSrcPad = m_gstWrapper->gstElementGetStaticPad(newAudioDecoder, "src")) !=
            NULL) // Unref the Pad
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Src Pad = %p", newAudioDecoderSrcPad);
        if ((newAudioDecoderSinkPad = m_gstWrapper->gstElementGetStaticPad(newAudioDecoder, "sink")) !=
            NULL) // Unref the Pad
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Sink Pad = %p", newAudioDecoderSinkPad);
        // Link New Decoder to Downstream followed by UpStream
        if ((gstPadLinkRet = m_gstWrapper->gstPadLink(newAudioDecoderSrcPad, audioDecSrcPeerPad)) != GST_PAD_LINK_OK)
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Downstream Link Failed");
        if ((gstPadLinkRet = m_gstWrapper->gstPadLink(audioDecSinkPeerPad, newAudioDecoderSinkPad)) != GST_PAD_LINK_OK)
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder Upstream Link Failed");
        if ((newAudioParseSrcPad = m_gstWrapper->gstElementGetStaticPad(newAudioParse, "src")) != NULL) // Unref the Pad
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser Src Pad = %p", newAudioParseSrcPad);
        if ((newAudioParseSinkPad = m_gstWrapper->gstElementGetStaticPad(newAudioParse, "sink")) != NULL) // Unref the Pad
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser Sink Pad = %p", newAudioParseSinkPad);
        // Link New Parser to Downstream followed by UpStream
        if ((gstPadLinkRet = m_gstWrapper->gstPadLink(newAudioParseSrcPad, audioParseSrcPeerPad)) != GST_PAD_LINK_OK)
            RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser Downstream Link Failed %d", gstPadLinkRet);
        if ((audioParseUpstreamEl = GST_ELEMENT_CAST(m_gstWrapper->gstPadGetParent(audioParseSinkPeerPad))) ==
            m_context.playbackGroup.m_curAudioTypefind)
        {
            RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Setting to READY");
            if (GST_STATE_CHANGE_FAILURE == m_gstWrapper->gstElementSetState(audioParseUpstreamEl, GST_STATE_READY))
            {
                RIALTO_SERVER_LOG_WARN("Failed to set Typefind to READY in switchAudioCodec");
            }
            m_glibWrapper->gObjectSet(G_OBJECT(audioParseUpstreamEl), "force-caps", newAudioCaps, NULL);
            m_gstWrapper->gstElementSyncStateWithParent(audioParseUpstreamEl);
            m_gstWrapper->gstElementGetState(audioParseUpstreamEl, &currentState, &pending, GST_CLOCK_TIME_NONE);
            RIALTO_SERVER_LOG_DEBUG("OTF -> New Typefind State = %d Pending = %d", currentState, pending);
            RIALTO_SERVER_LOG_DEBUG("OTF -> Typefind Syncing with Parent");
            m_context.playbackGroup.m_linkTypefindParser = true;
            m_gstWrapper->gstObjectUnref(audioParseUpstreamEl);
        }
        m_gstWrapper->gstObjectUnref(newAudioDecoderSrcPad);
        m_gstWrapper->gstObjectUnref(newAudioDecoderSinkPad);
        m_gstWrapper->gstObjectUnref(newAudioParseSrcPad);
        m_gstWrapper->gstObjectUnref(newAudioParseSinkPad);
    }
    m_gstWrapper->gstObjectUnref(audioParseSinkPeerPad);
    m_gstWrapper->gstObjectUnref(audioParseSrcPeerPad);
    m_gstWrapper->gstObjectUnref(audioParseSinkPad);
    m_gstWrapper->gstObjectUnref(audioParseSrcPad);
    m_gstWrapper->gstObjectUnref(audioDecSinkPeerPad);
    m_gstWrapper->gstObjectUnref(audioDecSrcPeerPad);
    m_gstWrapper->gstObjectUnref(audioDecSinkPad);
    m_gstWrapper->gstObjectUnref(audioDecSrcPad);
    m_gstWrapper->gstElementSyncStateWithParent(newAudioDecoder);
    m_gstWrapper->gstElementGetState(newAudioDecoder, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioDecoder State = %d Pending = %d", currentState, pending);
    m_gstWrapper->gstElementSyncStateWithParent(newAudioParse);
    m_gstWrapper->gstElementGetState(newAudioParse, &currentState, &pending, GST_CLOCK_TIME_NONE);
    RIALTO_SERVER_LOG_DEBUG("OTF -> New AudioParser State = %d Pending = %d", currentState, pending);
    m_context.playbackGroup.m_isAudioAAC = isAudioAAC;
    return true;
}

bool GstGenericPlayer::performAudioTrackCodecChannelSwitch(const void *pSampleAttr,
                                                           firebolt::rialto::wrappers::AudioAttributesPrivate *pAudioAttr,
                                                           uint32_t *pStatus, unsigned int *pui32Delay,
                                                           long long *pAudioChangeTargetPts, // NOLINT(runtime/int)
                                                           const long long *pcurrentDispPts, // NOLINT(runtime/int)
                                                           unsigned int *audioChangeStage, GstCaps **appsrcCaps,
                                                           bool *audioaac, bool svpenabled, GstElement *aSrc, bool *ret)
{
    // this function comes from rdk_gstreamer_utils
    if (!pStatus || !pui32Delay || !pAudioChangeTargetPts || !pcurrentDispPts || !audioChangeStage || !appsrcCaps ||
        !audioaac || !aSrc || !ret)
    {
        RIALTO_SERVER_LOG_ERROR("performAudioTrackCodecChannelSwitch: invalid null parameter");
        return false;
    }

    constexpr uint32_t kOk = 0;
    constexpr uint32_t kWaitWhileIdling = 100;
    constexpr int kAudioChangeGapThresholdMS = 40;
    constexpr unsigned int kAudchgAlign = 3;

    struct timespec ts, now;
    unsigned int reconfigDelayMs;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    if (*pStatus != kOk || pSampleAttr == nullptr)
    {
        RIALTO_SERVER_LOG_DEBUG("No audio data ready yet");
        *pui32Delay = kWaitWhileIdling;
        *ret = false;
        return true;
    }
    RIALTO_SERVER_LOG_DEBUG("Received first audio packet after a flush, PTS");
    if (pAudioAttr)
    {
        const char *pCodecStr = pAudioAttr->m_codecParam.c_str();
        const char *pCodecAcc = strstr(pCodecStr, "mp4a");
        bool isAudioAAC = (pCodecAcc) ? true : false;
        bool isCodecSwitch = false;
        RIALTO_SERVER_LOG_DEBUG("Audio Attribute format %s channel %d samp %d, bitrate %d blockAlignment %d", pCodecStr,
                                pAudioAttr->m_numberOfChannels, pAudioAttr->m_samplesPerSecond, pAudioAttr->m_bitrate,
                                pAudioAttr->m_blockAlignment);
        *pAudioChangeTargetPts = *pcurrentDispPts;
        *audioChangeStage = kAudchgAlign;
        if (*appsrcCaps)
        {
            m_gstWrapper->gstCapsUnref(*appsrcCaps);
            *appsrcCaps = NULL;
        }
        if (isAudioAAC != *audioaac)
            isCodecSwitch = true;
        configAudioCap(pAudioAttr, audioaac, svpenabled, appsrcCaps);
        {
            gboolean sendRet = FALSE;
            GstEvent *flushStart = NULL;
            GstEvent *flushStop = NULL;
            flushStart = m_gstWrapper->gstEventNewFlushStart();
            sendRet = m_gstWrapper->gstElementSendEvent(aSrc, flushStart);
            if (!sendRet)
                RIALTO_SERVER_LOG_DEBUG("failed to send flush-start event");
            flushStop = m_gstWrapper->gstEventNewFlushStop(TRUE);
            sendRet = m_gstWrapper->gstElementSendEvent(aSrc, flushStop);
            if (!sendRet)
                RIALTO_SERVER_LOG_DEBUG("failed to send flush-stop event");
        }
        if (!isCodecSwitch)
        {
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(aSrc), *appsrcCaps);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("CODEC SWITCH mAudioAAC = %d", *audioaac);
            haltAudioPlayback();
            if (switchAudioCodec(*audioaac, *appsrcCaps) == false)
            {
                RIALTO_SERVER_LOG_DEBUG("CODEC SWITCH FAILED switchAudioCodec mAudioAAC = %d", *audioaac);
            }
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(aSrc), *appsrcCaps);
            resumeAudioPlayback();
        }
        clock_gettime(CLOCK_MONOTONIC, &now);
        reconfigDelayMs = now.tv_nsec > ts.tv_nsec ? (now.tv_nsec - ts.tv_nsec) / 1000000
                                                   : (1000 - (ts.tv_nsec - now.tv_nsec) / 1000000);
        (*pAudioChangeTargetPts) += (reconfigDelayMs + kAudioChangeGapThresholdMS);
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("first audio after change no attribute drop!");
        *pui32Delay = 0;
        *ret = false;
        return true;
    }
    *ret = true;
    return true;
}

bool GstGenericPlayer::setImmediateOutput(const MediaSourceType &mediaSourceType, bool immediateOutputParam)
{
    if (!m_workerThread)
        return false;

    m_workerThread->enqueueTask(
        m_taskFactory->createSetImmediateOutput(m_context, *this, mediaSourceType, immediateOutputParam));
    return true;
}

bool GstGenericPlayer::setReportDecodeErrors(const MediaSourceType &mediaSourceType, bool reportDecodeErrors)
{
    if (!m_workerThread)
        return false;

    m_workerThread->enqueueTask(
        m_taskFactory->createSetReportDecodeErrors(m_context, *this, mediaSourceType, reportDecodeErrors));
    return true;
}

bool GstGenericPlayer::getQueuedFrames(uint32_t &queuedFrames)
{
    bool returnValue{false};
    GstElement *decoder{getDecoder(MediaSourceType::VIDEO)};
    if (decoder)
    {
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "queued-frames"))
        {
            m_glibWrapper->gObjectGet(decoder, "queued-frames", &queuedFrames, nullptr);
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("queued-frames not supported in element %s", GST_ELEMENT_NAME(decoder));
        }
        m_gstWrapper->gstObjectUnref(decoder);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get queued-frames property, decoder is NULL");
    }

    return returnValue;
}

bool GstGenericPlayer::getImmediateOutput(const MediaSourceType &mediaSourceType, bool &immediateOutputRef)
{
    bool returnValue{false};
    GstElement *sink{getSink(mediaSourceType)};
    if (sink)
    {
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "immediate-output"))
        {
            m_glibWrapper->gObjectGet(sink, "immediate-output", &immediateOutputRef, nullptr);
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("immediate-output not supported in element %s", GST_ELEMENT_NAME(sink));
        }
        m_gstWrapper->gstObjectUnref(sink);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set immediate-output property, sink is NULL");
    }

    return returnValue;
}

bool GstGenericPlayer::getStats(const MediaSourceType &mediaSourceType, uint64_t &renderedFrames, uint64_t &droppedFrames)
{
    bool returnValue{false};
    GstElement *sink{getSink(mediaSourceType)};
    if (sink)
    {
        GstStructure *stats{nullptr};
        m_glibWrapper->gObjectGet(sink, "stats", &stats, nullptr);
        if (!stats)
        {
            RIALTO_SERVER_LOG_ERROR("failed to get stats from '%s'", GST_ELEMENT_NAME(sink));
        }
        else
        {
            guint64 renderedFramesTmp;
            guint64 droppedFramesTmp;
            if (m_gstWrapper->gstStructureGetUint64(stats, "rendered", &renderedFramesTmp) &&
                m_gstWrapper->gstStructureGetUint64(stats, "dropped", &droppedFramesTmp))
            {
                renderedFrames = renderedFramesTmp;
                droppedFrames = droppedFramesTmp;
                returnValue = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("failed to get 'rendered' or 'dropped' from structure (%s)",
                                        GST_ELEMENT_NAME(sink));
            }
            m_gstWrapper->gstStructureFree(stats);
        }
        m_gstWrapper->gstObjectUnref(sink);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get stats, sink is NULL");
    }

    return returnValue;
}

GstBuffer *GstGenericPlayer::createBuffer(const IMediaPipeline::MediaSegment &mediaSegment) const
{
    GstBuffer *gstBuffer = m_gstWrapper->gstBufferNewAllocate(nullptr, mediaSegment.getDataLength(), nullptr);
    m_gstWrapper->gstBufferFill(gstBuffer, 0, mediaSegment.getData(), mediaSegment.getDataLength());

    if (mediaSegment.isEncrypted())
    {
        GstBuffer *keyId = m_gstWrapper->gstBufferNewAllocate(nullptr, mediaSegment.getKeyId().size(), nullptr);
        m_gstWrapper->gstBufferFill(keyId, 0, mediaSegment.getKeyId().data(), mediaSegment.getKeyId().size());

        GstBuffer *initVector = m_gstWrapper->gstBufferNewAllocate(nullptr, mediaSegment.getInitVector().size(), nullptr);
        m_gstWrapper->gstBufferFill(initVector, 0, mediaSegment.getInitVector().data(),
                                    mediaSegment.getInitVector().size());
        GstBuffer *subsamples{nullptr};
        if (!mediaSegment.getSubSamples().empty())
        {
            auto subsamplesRawSize = mediaSegment.getSubSamples().size() * (sizeof(guint16) + sizeof(guint32));
            guint8 *subsamplesRaw = static_cast<guint8 *>(m_glibWrapper->gMalloc(subsamplesRawSize));
            GstByteWriter writer;
            m_gstWrapper->gstByteWriterInitWithData(&writer, subsamplesRaw, subsamplesRawSize, FALSE);

            for (const auto &subSample : mediaSegment.getSubSamples())
            {
                m_gstWrapper->gstByteWriterPutUint16Be(&writer, subSample.numClearBytes);
                m_gstWrapper->gstByteWriterPutUint32Be(&writer, subSample.numEncryptedBytes);
            }
            subsamples = m_gstWrapper->gstBufferNewWrapped(subsamplesRaw, subsamplesRawSize);
        }

        uint32_t crypt = 0;
        uint32_t skip = 0;
        bool encryptionPatternSet = mediaSegment.getEncryptionPattern(crypt, skip);

        GstRialtoProtectionData data = {mediaSegment.getMediaKeySessionId(),
                                        static_cast<uint32_t>(mediaSegment.getSubSamples().size()),
                                        mediaSegment.getInitWithLast15(),
                                        keyId,
                                        initVector,
                                        subsamples,
                                        mediaSegment.getCipherMode(),
                                        crypt,
                                        skip,
                                        encryptionPatternSet,
                                        m_context.decryptionService};

        if (!m_protectionMetadataWrapper->addProtectionMetadata(gstBuffer, data))
        {
            RIALTO_SERVER_LOG_ERROR("Failed to add protection metadata");
            if (keyId)
            {
                m_gstWrapper->gstBufferUnref(keyId);
            }
            if (initVector)
            {
                m_gstWrapper->gstBufferUnref(initVector);
            }
            if (subsamples)
            {
                m_gstWrapper->gstBufferUnref(subsamples);
            }
        }
    }

    GST_BUFFER_TIMESTAMP(gstBuffer) = mediaSegment.getTimeStamp();
    GST_BUFFER_DURATION(gstBuffer) = mediaSegment.getDuration();
    return gstBuffer;
}

void GstGenericPlayer::notifyNeedMediaData(const MediaSourceType mediaSource)
{
    auto elem = m_context.streamInfo.find(mediaSource);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;
        streamInfo.isNeedDataPending = false;

        // Send new NeedMediaData if we still need it
        if (m_gstPlayerClient && streamInfo.isDataNeeded)
        {
            streamInfo.isNeedDataPending = m_gstPlayerClient->notifyNeedMediaData(mediaSource);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("Media type %s could not be found", common::convertMediaSourceType(mediaSource));
    }
}

void GstGenericPlayer::notifyNeedMediaDataWithDelay(const MediaSourceType mediaSource)
{
    auto elem = m_context.streamInfo.find(mediaSource);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;
        streamInfo.isNeedDataPending = false;

        // Schedule new NeedMediaData if we still need it
        if (m_gstPlayerClient && streamInfo.isDataNeeded)
        {
            streamInfo.isNeedDataPending = m_gstPlayerClient->notifyNeedMediaDataWithDelay(mediaSource);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("Media type %s could not be found", common::convertMediaSourceType(mediaSource));
    }
}

void GstGenericPlayer::attachData(const firebolt::rialto::MediaSourceType mediaType)
{
    auto elem = m_context.streamInfo.find(mediaType);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;
        if (streamInfo.buffers.empty() || !streamInfo.isDataNeeded)
        {
            return;
        }

        if (firebolt::rialto::MediaSourceType::SUBTITLE == mediaType)
        {
            setTextTrackPositionIfRequired(streamInfo.appSrc);
        }
        else
        {
            pushSampleIfRequired(streamInfo.appSrc, mediaType);
        }
        if (mediaType == firebolt::rialto::MediaSourceType::AUDIO)
        {
            // This needs to be done before gstAppSrcPushBuffer() is
            // called because it can free the memory
            m_context.lastAudioSampleTimestamps = static_cast<int64_t>(GST_BUFFER_PTS(streamInfo.buffers.back()));
        }

        for (GstBuffer *buffer : streamInfo.buffers)
        {
            m_gstWrapper->gstAppSrcPushBuffer(GST_APP_SRC(streamInfo.appSrc), buffer);
        }
        streamInfo.buffers.clear();
        streamInfo.isDataPushed = true;

        const bool kIsSingle = m_context.streamInfo.size() == 1;
        bool allOtherStreamsPushed = std::all_of(m_context.streamInfo.begin(), m_context.streamInfo.end(),
                                                 [](const auto &entry) { return entry.second.isDataPushed; });

        if (!m_context.bufferedNotificationSent && (allOtherStreamsPushed || kIsSingle) && m_gstPlayerClient)
        {
            m_context.bufferedNotificationSent = true;
            m_gstPlayerClient->notifyNetworkState(NetworkState::BUFFERED);
            RIALTO_SERVER_LOG_MIL("Buffered NetworkState reached");
        }
        cancelUnderflow(mediaType);

        const auto eosInfoIt = m_context.endOfStreamInfo.find(mediaType);
        if (eosInfoIt != m_context.endOfStreamInfo.end() && eosInfoIt->second == EosState::PENDING)
        {
            setEos(mediaType);
        }
    }
}

void GstGenericPlayer::updateAudioCaps(int32_t rate, int32_t channels, const std::shared_ptr<CodecData> &codecData)
{
    auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;

        constexpr int kInvalidRate{0}, kInvalidChannels{0};
        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(streamInfo.appSrc));
        GstCaps *newCaps = m_gstWrapper->gstCapsCopy(currentCaps);

        if (rate != kInvalidRate)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "rate", G_TYPE_INT, rate, NULL);
        }

        if (channels != kInvalidChannels)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "channels", G_TYPE_INT, channels, NULL);
        }

        setCodecData(newCaps, codecData);

        if (!m_gstWrapper->gstCapsIsEqual(currentCaps, newCaps))
        {
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(streamInfo.appSrc), newCaps);
        }

        m_gstWrapper->gstCapsUnref(newCaps);
        m_gstWrapper->gstCapsUnref(currentCaps);
    }
}

void GstGenericPlayer::updateVideoCaps(int32_t width, int32_t height, Fraction frameRate,
                                       const std::shared_ptr<CodecData> &codecData)
{
    auto elem = m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;

        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(streamInfo.appSrc));
        GstCaps *newCaps = m_gstWrapper->gstCapsCopy(currentCaps);

        if (width > 0)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "width", G_TYPE_INT, width, NULL);
        }

        if (height > 0)
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "height", G_TYPE_INT, height, NULL);
        }

        if ((kUndefinedSize != frameRate.numerator) && (kUndefinedSize != frameRate.denominator))
        {
            m_gstWrapper->gstCapsSetSimple(newCaps, "framerate", GST_TYPE_FRACTION, frameRate.numerator,
                                           frameRate.denominator, NULL);
        }

        setCodecData(newCaps, codecData);

        if (!m_gstWrapper->gstCapsIsEqual(currentCaps, newCaps))
        {
            m_gstWrapper->gstAppSrcSetCaps(GST_APP_SRC(streamInfo.appSrc), newCaps);
        }

        m_gstWrapper->gstCapsUnref(currentCaps);
        m_gstWrapper->gstCapsUnref(newCaps);
    }
}

void GstGenericPlayer::addAudioClippingToBuffer(GstBuffer *buffer, uint64_t clippingStart, uint64_t clippingEnd) const
{
    if (clippingStart || clippingEnd)
    {
        if (m_gstWrapper->gstBufferAddAudioClippingMeta(buffer, GST_FORMAT_TIME, clippingStart, clippingEnd))
        {
            RIALTO_SERVER_LOG_DEBUG("Added audio clipping to buffer %p, start: %" PRIu64 ", end %" PRIu64, buffer,
                                    clippingStart, clippingEnd);
        }
        else
        {
            RIALTO_SERVER_LOG_WARN("Failed to add audio clipping to buffer %p, start: %" PRIu64 ", end %" PRIu64,
                                   buffer, clippingStart, clippingEnd);
        }
    }
}

bool GstGenericPlayer::setCodecData(GstCaps *caps, const std::shared_ptr<CodecData> &codecData) const
{
    if (codecData && CodecDataType::BUFFER == codecData->type)
    {
        gpointer memory = m_glibWrapper->gMemdup(codecData->data.data(), codecData->data.size());
        GstBuffer *buf = m_gstWrapper->gstBufferNewWrapped(memory, codecData->data.size());
        m_gstWrapper->gstCapsSetSimple(caps, "codec_data", GST_TYPE_BUFFER, buf, nullptr);
        m_gstWrapper->gstBufferUnref(buf);
        return true;
    }
    if (codecData && CodecDataType::STRING == codecData->type)
    {
        std::string codecDataStr(codecData->data.begin(), codecData->data.end());
        m_gstWrapper->gstCapsSetSimple(caps, "codec_data", G_TYPE_STRING, codecDataStr.c_str(), nullptr);
        return true;
    }
    return false;
}

void GstGenericPlayer::pushSampleIfRequired(GstElement *source, const MediaSourceType &mediaSourceType)
{
    const std::string kTypeStr{common::convertMediaSourceType(mediaSourceType)};

    auto initialPosition = m_context.initialPositions.find(source);
    if (m_context.initialPositions.end() == initialPosition)
    {
        // Sending initial sample not needed
        return;
    }
    // GstAppSrc does not replace segment, if it's the same as previous one.
    // It causes problems with position reporing in amlogic devices, so we need to push
    // two segments with different reset time value.
    pushAdditionalSegmentIfRequired(source);

    for (const auto &[position, resetTime, appliedRate, stopPosition] : initialPosition->second)
    {
        GstSeekFlags seekFlag = resetTime ? GST_SEEK_FLAG_FLUSH : GST_SEEK_FLAG_NONE;
        RIALTO_SERVER_LOG_DEBUG("Pushing new %s sample...", kTypeStr.c_str());
        GstSegment *segment{m_gstWrapper->gstSegmentNew()};
        m_gstWrapper->gstSegmentInit(segment, GST_FORMAT_TIME);
        if (!m_gstWrapper->gstSegmentDoSeek(segment, m_context.playbackRate, GST_FORMAT_TIME, seekFlag,
                                            GST_SEEK_TYPE_SET, position, GST_SEEK_TYPE_SET, stopPosition, nullptr))
        {
            RIALTO_SERVER_LOG_WARN("Segment seek failed.");
            m_gstWrapper->gstSegmentFree(segment);
            m_context.initialPositions.erase(initialPosition);
            return;
        }
        segment->applied_rate = appliedRate;
        RIALTO_SERVER_LOG_MIL("New %s segment: [%" GST_TIME_FORMAT ", %" GST_TIME_FORMAT
                              "], rate: %f, appliedRate %f, reset_time: %d\n",
                              kTypeStr.c_str(), GST_TIME_ARGS(segment->start), GST_TIME_ARGS(segment->stop),
                              segment->rate, segment->applied_rate, resetTime);
        auto recordId = m_context.gstProfiler->createRecord("First Segment Received", kTypeStr);
        if (recordId)
            m_context.gstProfiler->logRecord(recordId.value());

        GstCaps *currentCaps = m_gstWrapper->gstAppSrcGetCaps(GST_APP_SRC(source));
        // We can't pass buffer in GstSample, because implementation of gst_app_src_push_sample
        // uses gst_buffer_copy, which loses RialtoProtectionMeta (that causes problems with EME
        // for first frame).
        GstSample *sample = m_gstWrapper->gstSampleNew(nullptr, currentCaps, segment, nullptr);
        m_gstWrapper->gstAppSrcPushSample(GST_APP_SRC(source), sample);
        m_gstWrapper->gstSampleUnref(sample);
        m_gstWrapper->gstCapsUnref(currentCaps);

        m_gstWrapper->gstSegmentFree(segment);

        if (mediaSourceType == MediaSourceType::AUDIO)
        {
            m_context.audioGstSegmentPosition = position;
        }
    }
    m_context.currentPosition[source] = initialPosition->second.back();
    m_context.initialPositions.erase(initialPosition);
    return;
}

void GstGenericPlayer::pushAdditionalSegmentIfRequired(GstElement *source)
{
    auto currentPosition = m_context.currentPosition.find(source);
    if (m_context.currentPosition.end() == currentPosition)
    {
        return;
    }
    auto initialPosition = m_context.initialPositions.find(source);
    if (m_context.initialPositions.end() == initialPosition)
    {
        return;
    }
    const auto &positions = initialPosition->second;
    const bool allMatchCurrentPosition = !positions.empty() &&
                                         std::all_of(positions.begin(), positions.end(), [&](const SegmentData &segment)
                                                     { return segment == currentPosition->second; });
    if (allMatchCurrentPosition && positions.back().resetTime)
    {
        RIALTO_SERVER_LOG_INFO("Adding additional segment with reset_time = false");
        SegmentData additionalSegment = initialPosition->second.back();
        additionalSegment.resetTime = false;
        initialPosition->second.push_back(additionalSegment);
    }
}

void GstGenericPlayer::setTextTrackPositionIfRequired(GstElement *source)
{
    auto initialPosition = m_context.initialPositions.find(source);
    if (m_context.initialPositions.end() == initialPosition)
    {
        // Sending initial sample not needed
        return;
    }

    RIALTO_SERVER_LOG_MIL("New subtitle position set %" GST_TIME_FORMAT,
                          GST_TIME_ARGS(initialPosition->second.back().position));
    m_glibWrapper->gObjectSet(m_context.subtitleSink, "position",
                              static_cast<guint64>(initialPosition->second.back().position), nullptr);

    m_context.initialPositions.erase(initialPosition);
}

bool GstGenericPlayer::reattachSource(const std::unique_ptr<IMediaPipeline::MediaSource> &source)
{
    if (m_context.streamInfo.find(source->getType()) == m_context.streamInfo.end())
    {
        RIALTO_SERVER_LOG_ERROR("Unable to switch source, type does not exist");
        return false;
    }
    if (source->getMimeType().empty())
    {
        RIALTO_SERVER_LOG_WARN("Skip switch audio source. Unknown mime type");
        return false;
    }
    std::optional<firebolt::rialto::wrappers::AudioAttributesPrivate> audioAttributes{createAudioAttributes(source)};
    if (!audioAttributes)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create audio attributes");
        return false;
    }

    long long currentDispPts = getPosition(m_context.pipeline); // NOLINT(runtime/int)
    GstCaps *caps{createCapsFromMediaSource(m_gstWrapper, m_glibWrapper, source)};
    GstAppSrc *appSrc{GST_APP_SRC(m_context.streamInfo[source->getType()].appSrc)};
    GstCaps *oldCaps = m_gstWrapper->gstAppSrcGetCaps(appSrc);

    if ((!oldCaps) || m_context.audioSourceRemoved || (!m_gstWrapper->gstCapsIsEqual(caps, oldCaps)))
    {
        RIALTO_SERVER_LOG_DEBUG("Caps not equal. Perform audio track codec channel switch.");

        GstElement *sink = getSink(MediaSourceType::AUDIO);
        if (!sink)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to get audio sink");
            if (caps)
                m_gstWrapper->gstCapsUnref(caps);
            if (oldCaps)
                m_gstWrapper->gstCapsUnref(oldCaps);
            return false;
        }
        std::string sinkName = GST_ELEMENT_NAME(sink);
        m_gstWrapper->gstObjectUnref(sink);

        int sampleAttributes{
            0}; // rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch checks if this param != NULL only.
        std::uint32_t status{0};   // must be 0 to make rdk_gstreamer_utils::performAudioTrackCodecChannelSwitch work
        unsigned int ui32Delay{0}; // output param
        long long audioChangeTargetPts{-1}; // NOLINT(runtime/int) output param. Set audioChangeTargetPts =
                                            // currentDispPts in rdk_gstreamer_utils function stub
        unsigned int audioChangeStage{0};   // Output param. Set to AUDCHG_ALIGN in rdk_gstreamer_utils function stub
        gchar *oldCapsCStr = m_gstWrapper->gstCapsToString(oldCaps);
        std::string oldCapsStr = std::string(oldCapsCStr);
        m_glibWrapper->gFree(oldCapsCStr);
        bool audioAac{oldCapsStr.find("audio/mpeg") != std::string::npos};
        bool svpEnabled{true}; // assume always true
        bool retVal{false};    // Output param. Set to TRUE in rdk_gstreamer_utils function stub

        bool result = false;
        if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "amlhalasink"))
        {
            // due to problems audio codec change in prerolling, temporarily moved the code from rdk gstreamer utils to
            // Rialto and applied fixes
            result = performAudioTrackCodecChannelSwitch(&sampleAttributes, &(*audioAttributes), &status, &ui32Delay,
                                                         &audioChangeTargetPts, &currentDispPts, &audioChangeStage,
                                                         &caps, &audioAac, svpEnabled, GST_ELEMENT(appSrc), &retVal);
        }
        else
        {
            result = m_rdkGstreamerUtilsWrapper->performAudioTrackCodecChannelSwitch(&m_context.playbackGroup,
                                                                                     &sampleAttributes,
                                                                                     &(*audioAttributes), &status,
                                                                                     &ui32Delay, &audioChangeTargetPts,
                                                                                     &currentDispPts, &audioChangeStage,
                                                                                     &caps, &audioAac, svpEnabled,
                                                                                     GST_ELEMENT(appSrc), &retVal);
        }

        if (!result || !retVal)
        {
            RIALTO_SERVER_LOG_WARN("performAudioTrackCodecChannelSwitch failed! Result: %d, retval %d", result, retVal);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Skip switching audio source - caps are the same.");
    }

    m_context.lastAudioSampleTimestamps = currentDispPts;
    if (caps)
        m_gstWrapper->gstCapsUnref(caps);
    if (oldCaps)
        m_gstWrapper->gstCapsUnref(oldCaps);

    return true;
}

bool GstGenericPlayer::hasSourceType(const MediaSourceType &mediaSourceType) const
{
    return m_context.streamInfo.find(mediaSourceType) != m_context.streamInfo.end();
}

void GstGenericPlayer::scheduleNeedMediaData(GstAppSrc *src)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createNeedData(m_context, *this, src));
    }
}

void GstGenericPlayer::scheduleEnoughData(GstAppSrc *src)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createEnoughData(m_context, src));
    }
}

void GstGenericPlayer::scheduleAudioUnderflow()
{
    if (m_workerThread)
    {
        bool underflowEnabled = m_context.isPlaying && !m_context.audioSourceRemoved;
        m_workerThread->enqueueTask(
            m_taskFactory->createUnderflow(m_context, *this, underflowEnabled, MediaSourceType::AUDIO));
    }
}

void GstGenericPlayer::scheduleVideoUnderflow()
{
    if (m_workerThread)
    {
        bool underflowEnabled = m_context.isPlaying;
        m_workerThread->enqueueTask(
            m_taskFactory->createUnderflow(m_context, *this, underflowEnabled, MediaSourceType::VIDEO));
    }
}

void GstGenericPlayer::scheduleFirstVideoFrameReceived()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createFirstFrameReceived(m_context, *this, MediaSourceType::VIDEO));
    }
}

void GstGenericPlayer::scheduleFirstAudioFrameReceived(AudioFirstFrameAction audioAction)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createFirstFrameReceived(m_context, *this, MediaSourceType::AUDIO, audioAction));
    }
}

void GstGenericPlayer::setAudioFirstFrameFallbackProbe(GstPad *pad, gulong id)
{
    GstPad *oldPad{m_context.audioFirstFrameProbePad};
    gulong oldId{m_context.audioFirstFrameProbeId};
    m_context.audioFirstFrameProbePad = pad;
    m_context.audioFirstFrameProbeId = id;

    if (oldPad && oldId != 0)
    {
        m_gstWrapper->gstPadRemoveProbe(oldPad, oldId);
    }

    if (oldPad)
    {
        m_gstWrapper->gstObjectUnref(oldPad);
    }
}

void GstGenericPlayer::clearAudioFirstFrameFallbackProbe()
{
    GstPad *pad{m_context.audioFirstFrameProbePad};
    gulong id{m_context.audioFirstFrameProbeId};
    m_context.audioFirstFrameProbePad = nullptr;
    m_context.audioFirstFrameProbeId = 0;

    if (pad && id != 0)
    {
        m_gstWrapper->gstPadRemoveProbe(pad, id);
    }

    if (pad)
    {
        m_gstWrapper->gstObjectUnref(pad);
    }
}

void GstGenericPlayer::clearAudioFirstFrameFallbackProbeState()
{
    GstPad *pad{m_context.audioFirstFrameProbePad};
    m_context.audioFirstFrameProbePad = nullptr;
    m_context.audioFirstFrameProbeId = 0;

    if (pad)
    {
        m_gstWrapper->gstObjectUnref(pad);
    }
}

void GstGenericPlayer::scheduleAllSourcesAttached()
{
    allSourcesAttached();
}

void GstGenericPlayer::cancelUnderflow(firebolt::rialto::MediaSourceType mediaSource)
{
    auto elem = m_context.streamInfo.find(mediaSource);
    if (elem != m_context.streamInfo.end())
    {
        StreamInfo &streamInfo = elem->second;
        if (!streamInfo.underflowOccured)
        {
            return;
        }

        RIALTO_SERVER_LOG_DEBUG("Cancelling %s underflow", common::convertMediaSourceType(mediaSource));
        streamInfo.underflowOccured = false;
    }
}

void GstGenericPlayer::play(bool &async)
{
    async = true;
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPlay(*this));
    }
}

void GstGenericPlayer::pause()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPause(m_context, *this));
    }
}

void GstGenericPlayer::stop()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createStop(m_context, *this));
    }
}

GstStateChangeReturn GstGenericPlayer::changePipelineState(GstState newState)
{
    if (!m_context.pipeline)
    {
        RIALTO_SERVER_LOG_ERROR("Change state failed - pipeline is nullptr");
        if (m_gstPlayerClient)
            m_gstPlayerClient->notifyPlaybackState(PlaybackState::FAILURE);
        return GST_STATE_CHANGE_FAILURE;
    }
    m_context.flushOnPrerollController->setTargetState(newState);
    const GstStateChangeReturn result{m_gstWrapper->gstElementSetState(m_context.pipeline, newState)};
    if (result == GST_STATE_CHANGE_FAILURE)
    {
        RIALTO_SERVER_LOG_ERROR("Change state failed - Gstreamer returned an error");
        if (m_gstPlayerClient)
            m_gstPlayerClient->notifyPlaybackState(PlaybackState::FAILURE);
    }
    return result;
}

int64_t GstGenericPlayer::getPosition(GstElement *element)
{
    if (!element)
    {
        RIALTO_SERVER_LOG_WARN("Element is null");
        return -1;
    }

    m_gstWrapper->gstStateLock(element);

    if (m_gstWrapper->gstElementGetState(element) < GST_STATE_PAUSED ||
        (m_gstWrapper->gstElementGetStateReturn(element) == GST_STATE_CHANGE_ASYNC &&
         m_gstWrapper->gstElementGetStateNext(element) == GST_STATE_PAUSED))
    {
        RIALTO_SERVER_LOG_WARN("Element is prerolling or in invalid state - state: %s, return: %s, next: %s",
                               m_gstWrapper->gstElementStateGetName(m_gstWrapper->gstElementGetState(element)),
                               m_gstWrapper->gstElementStateChangeReturnGetName(
                                   m_gstWrapper->gstElementGetStateReturn(element)),
                               m_gstWrapper->gstElementStateGetName(m_gstWrapper->gstElementGetStateNext(element)));

        m_gstWrapper->gstStateUnlock(element);
        return -1;
    }
    m_gstWrapper->gstStateUnlock(element);

    gint64 position = -1;
    if (!m_gstWrapper->gstElementQueryPosition(m_context.pipeline, GST_FORMAT_TIME, &position))
    {
        RIALTO_SERVER_LOG_WARN("Failed to query position");
        return -1;
    }

    return position;
}

void GstGenericPlayer::setVideoGeometry(int x, int y, int width, int height)
{
    m_context.videoGeometrySetByApi.store(true);
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createSetVideoGeometry(m_context, *this, Rectangle{x, y, width, height}));
    }
}

void GstGenericPlayer::setEos(const firebolt::rialto::MediaSourceType &type)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createEos(m_context, *this, type));
    }
}

bool GstGenericPlayer::setVideoSinkRectangle()
{
    bool result = false;
    GstElement *videoSink{getSink(MediaSourceType::VIDEO)};
    if (videoSink)
    {
        const Rectangle pendingGeometry = m_context.pendingGeometry;
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), "rectangle"))
        {
            std::string rect = std::to_string(pendingGeometry.x) + ',' + std::to_string(pendingGeometry.y) + ',' +
                               std::to_string(pendingGeometry.width) + ',' + std::to_string(pendingGeometry.height);
            m_glibWrapper->gObjectSet(videoSink, "rectangle", rect.c_str(), nullptr);
            result = true;
        }
        else if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), "render-rectangle"))
        {
            result = setRenderRectangleProperty(m_gstWrapper, m_glibWrapper, videoSink, pendingGeometry);
        }

        if (result)
        {
            RIALTO_SERVER_LOG_MIL("Applied video geometry x=%d y=%d width=%d height=%d to sink '%s'", pendingGeometry.x,
                                  pendingGeometry.y, pendingGeometry.width, pendingGeometry.height,
                                  GST_ELEMENT_NAME(videoSink));
            m_context.pendingGeometry.clear();
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set video geometry on sink '%s'", GST_ELEMENT_NAME(videoSink));
        }
        m_gstWrapper->gstObjectUnref(videoSink);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to set video rectangle, sink is NULL");
    }

    return result;
}

bool GstGenericPlayer::setImmediateOutput()
{
    bool result{false};
    if (m_context.pendingImmediateOutputForVideo.has_value())
    {
        GstElement *sink{getSink(MediaSourceType::VIDEO)};
        if (sink)
        {
            bool immediateOutput{m_context.pendingImmediateOutputForVideo.value()};
            RIALTO_SERVER_LOG_DEBUG("Set immediate-output to %s", immediateOutput ? "TRUE" : "FALSE");

            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "immediate-output"))
            {
                gboolean immediateOutputGboolean{immediateOutput ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(sink, "immediate-output", immediateOutputGboolean, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set immediate-output property on sink '%s'", GST_ELEMENT_NAME(sink));
            }
            m_context.pendingImmediateOutputForVideo.reset();
            m_gstWrapper->gstObjectUnref(sink);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending an immediate-output, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setLowLatencyAudioDecoder()
{
    bool result{true};
    if (m_context.pendingLowLatencyForAudioDecoder.has_value())
    {
        GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
        if (decoder)
        {
            bool lowLatencyAudioDecoder{m_context.pendingLowLatencyForAudioDecoder.value()};
            RIALTO_SERVER_LOG_DEBUG("Set low latency properties on audio decoder to %s",
                                    lowLatencyAudioDecoder ? "TRUE" : "FALSE");

            const auto setDecoderBoolPropertyIfExists = [&](const char *property, bool value, bool &ret)
            {
                if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), property))
                {
                    gboolean propertyValue{value ? TRUE : FALSE};
                    m_glibWrapper->gObjectSet(decoder, property, propertyValue, nullptr);
                }
                else
                {
                    ret = false;
                }
            };

            const auto setDecoderIntPropertyIfExists = [&](const char *property, gint value, bool &ret)
            {
                if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), property))
                {
                    m_glibWrapper->gObjectSet(decoder, property, value, nullptr);
                }
                else
                {
                    ret = false;
                }
            };

            GstElementFactory *factory = m_gstWrapper->gstElementGetFactory(decoder);
            const std::string decoderName = std::string(m_gstWrapper->gstPluginFeatureGetName(factory));

            if (m_glibWrapper->gStrHasPrefix(decoderName.c_str(), "brcmaudiodecoder"))
            {
                setDecoderBoolPropertyIfExists("sync-off", lowLatencyAudioDecoder, result);
                setDecoderIntPropertyIfExists("stream_sync_mode", lowLatencyAudioDecoder ? 1 : 0, result);
            }

            m_context.pendingLowLatencyForAudioDecoder.reset();
            m_gstWrapper->gstObjectUnref(decoder);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending an low latency audio decoder, decoder is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setLowLatencyAudioSink()
{
    bool result{true};
    if (m_context.pendingLowLatencyForAudioSink.has_value())
    {
        GstElement *sink{getSink(MediaSourceType::AUDIO)};
        if (sink)
        {
            bool lowLatencyAudioSink{m_context.pendingLowLatencyForAudioSink.value()};
            RIALTO_SERVER_LOG_DEBUG("Set low latency properties on audio sink to %s",
                                    lowLatencyAudioSink ? "TRUE" : "FALSE");

            const auto setSinkBoolPropertyIfExists = [&](const char *property, bool value, bool &ret)
            {
                if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), property))
                {
                    gboolean propertyValue{value ? TRUE : FALSE};
                    m_glibWrapper->gObjectSet(sink, property, propertyValue, nullptr);
                }
                else
                {
                    ret = false;
                }
            };

            const auto setSinkIntPropertyIfExists = [&](const char *property, gint value, bool &ret)
            {
                if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), property))
                {
                    m_glibWrapper->gObjectSet(sink, property, value, nullptr);
                }
                else
                {
                    ret = false;
                }
            };

            GstElementFactory *factory = m_gstWrapper->gstElementGetFactory(sink);
            const std::string sinkName = std::string(m_gstWrapper->gstPluginFeatureGetName(factory));

            if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "brcmaudiosink"))
            {
                setSinkBoolPropertyIfExists("low-latency", lowLatencyAudioSink, result);
                setSinkBoolPropertyIfExists("sync", !lowLatencyAudioSink, result);
            }
            else if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "rtkaudiosink"))
            {
                setSinkBoolPropertyIfExists("media-tunnel", false, result);
                setSinkBoolPropertyIfExists("audio-service", true, result);
                setSinkIntPropertyIfExists("lowdelay-sync-mode", lowLatencyAudioSink ? 0 : 1, result);
                setSinkBoolPropertyIfExists("sync", !lowLatencyAudioSink, result);
            }
            else if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "amlhalasink"))
            {
                setSinkBoolPropertyIfExists("llp-mode", lowLatencyAudioSink, result);
                setSinkBoolPropertyIfExists("sync", !lowLatencyAudioSink, result);
            }
            else if (m_glibWrapper->gStrHasPrefix(sinkName.c_str(), "mtkaudiosink"))
            {
                setSinkBoolPropertyIfExists("llp-mode", lowLatencyAudioSink, result);
            }
            else
            {
                RIALTO_SERVER_LOG_WARN("No low latency vendor mapping applied for sink '%s'", sinkName.c_str());
            }

            m_context.pendingLowLatencyForAudioSink.reset();
            m_gstWrapper->gstObjectUnref(sink);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending an low latency audio sink, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setReportDecodeErrors()
{
    bool result{false};
    bool reportDecodeErrors{false};

    {
        std::unique_lock lock{m_context.propertyMutex};
        if (!m_context.pendingReportDecodeErrorsForVideo.has_value())
        {
            return false;
        }
        reportDecodeErrors = m_context.pendingReportDecodeErrorsForVideo.value();
    }

    GstElement *decoder = getDecoder(MediaSourceType::VIDEO);
    if (decoder)
    {
        RIALTO_SERVER_LOG_DEBUG("Set report decode errors to %s", reportDecodeErrors ? "TRUE" : "FALSE");

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "report-decode-errors"))
        {
            gboolean reportDecodeErrorsGboolean{reportDecodeErrors ? TRUE : FALSE};
            m_glibWrapper->gObjectSet(decoder, "report-decode-errors", reportDecodeErrorsGboolean, nullptr);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set report-decode-errors property on decoder '%s'",
                                    GST_ELEMENT_NAME(decoder));
        }

        m_gstWrapper->gstObjectUnref(decoder);

        {
            std::unique_lock lock{m_context.propertyMutex};
            m_context.pendingReportDecodeErrorsForVideo.reset();
        }
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Pending report-decode-errors, decoder is NULL");
    }
    return result;
}

bool GstGenericPlayer::setShowVideoWindow()
{
    if (!m_context.pendingShowVideoWindow.has_value())
    {
        RIALTO_SERVER_LOG_WARN("No show video window value to be set. Aborting...");
        return false;
    }

    GstElement *videoSink{getSink(MediaSourceType::VIDEO)};
    if (!videoSink)
    {
        RIALTO_SERVER_LOG_DEBUG("Setting show video window queued. Video sink is NULL");
        return false;
    }
    bool result{false};
    if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), "show-video-window"))
    {
        m_glibWrapper->gObjectSet(videoSink, "show-video-window", m_context.pendingShowVideoWindow.value(), nullptr);
        result = true;
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Setting show video window failed. Property does not exist");
    }
    m_context.pendingShowVideoWindow.reset();
    m_gstWrapper->gstObjectUnref(GST_OBJECT(videoSink));
    return result;
}

bool GstGenericPlayer::setLowLatency()
{
    bool result{false};
    if (m_context.pendingLowLatency.has_value())
    {
        GstElement *sink{getSink(MediaSourceType::AUDIO)};
        if (sink)
        {
            bool lowLatency{m_context.pendingLowLatency.value()};
            RIALTO_SERVER_LOG_DEBUG("Set low-latency to %s", lowLatency ? "TRUE" : "FALSE");

            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "low-latency"))
            {
                gboolean lowLatencyGboolean{lowLatency ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(sink, "low-latency", lowLatencyGboolean, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set low-latency property on sink '%s'", GST_ELEMENT_NAME(sink));
            }
            m_context.pendingLowLatency.reset();
            m_gstWrapper->gstObjectUnref(sink);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending low-latency, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setSync()
{
    bool result{false};
    if (m_context.pendingSync.has_value())
    {
        GstElement *sink{getSink(MediaSourceType::AUDIO)};
        if (sink)
        {
            bool sync{m_context.pendingSync.value()};
            RIALTO_SERVER_LOG_DEBUG("Set sync to %s", sync ? "TRUE" : "FALSE");

            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "sync"))
            {
                gboolean syncGboolean{sync ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(sink, "sync", syncGboolean, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set sync property on sink '%s'", GST_ELEMENT_NAME(sink));
            }
            m_context.pendingSync.reset();
            m_gstWrapper->gstObjectUnref(sink);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending sync, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setSyncOff()
{
    bool result{false};
    if (m_context.pendingSyncOff.has_value())
    {
        GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
        if (decoder)
        {
            bool syncOff{m_context.pendingSyncOff.value()};
            RIALTO_SERVER_LOG_DEBUG("Set sync-off to %s", syncOff ? "TRUE" : "FALSE");

            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "sync-off"))
            {
                gboolean syncOffGboolean{syncOff ? TRUE : FALSE};
                m_glibWrapper->gObjectSet(decoder, "sync-off", syncOffGboolean, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set sync-off property on decoder '%s'", GST_ELEMENT_NAME(decoder));
            }
            m_context.pendingSyncOff.reset();
            m_gstWrapper->gstObjectUnref(decoder);
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending sync-off, decoder is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setStreamSyncMode(const MediaSourceType &type)
{
    bool result{false};
    int32_t streamSyncMode{0};
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (m_context.pendingStreamSyncMode.find(type) == m_context.pendingStreamSyncMode.end())
        {
            return false;
        }
        streamSyncMode = m_context.pendingStreamSyncMode[type];
    }
    if (MediaSourceType::AUDIO == type)
    {
        GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
        if (!decoder)
        {
            RIALTO_SERVER_LOG_DEBUG("Pending stream-sync-mode, decoder is NULL");
            return false;
        }

        RIALTO_SERVER_LOG_DEBUG("Set stream-sync-mode to %d", streamSyncMode);

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "stream-sync-mode"))
        {
            gint streamSyncModeGint{static_cast<gint>(streamSyncMode)};
            m_glibWrapper->gObjectSet(decoder, "stream-sync-mode", streamSyncModeGint, nullptr);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set stream-sync-mode property on decoder '%s'", GST_ELEMENT_NAME(decoder));
        }
        m_gstWrapper->gstObjectUnref(decoder);
        std::unique_lock lock{m_context.propertyMutex};
        m_context.pendingStreamSyncMode.erase(type);
    }
    else if (MediaSourceType::VIDEO == type)
    {
        GstElement *parser = getParser(MediaSourceType::VIDEO);
        if (!parser)
        {
            RIALTO_SERVER_LOG_DEBUG("Pending syncmode-streaming, parser is NULL");
            return false;
        }

        gboolean streamSyncModeBoolean{static_cast<gboolean>(streamSyncMode)};
        RIALTO_SERVER_LOG_DEBUG("Set syncmode-streaming to %d", streamSyncMode);

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(parser), "syncmode-streaming"))
        {
            m_glibWrapper->gObjectSet(parser, "syncmode-streaming", streamSyncModeBoolean, nullptr);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set syncmode-streaming property on parser '%s'", GST_ELEMENT_NAME(parser));
        }
        m_gstWrapper->gstObjectUnref(parser);
        std::unique_lock lock{m_context.propertyMutex};
        m_context.pendingStreamSyncMode.erase(type);
    }
    return result;
}

bool GstGenericPlayer::setRenderFrame()
{
    bool result{false};
    if (m_context.pendingRenderFrame)
    {
        static const std::string kStepOnPrerollPropertyName = "frame-step-on-preroll";
        GstElement *sink{getSink(MediaSourceType::VIDEO)};
        if (sink)
        {
            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), kStepOnPrerollPropertyName.c_str()))
            {
                RIALTO_SERVER_LOG_INFO("Rendering preroll");

                m_glibWrapper->gObjectSet(sink, kStepOnPrerollPropertyName.c_str(), 1, nullptr);
                gboolean sendRet =
                    m_gstWrapper->gstElementSendEvent(sink, m_gstWrapper->gstEventNewStep(GST_FORMAT_BUFFERS, 1, 1.0,
                                                                                          true, false));
                if (!sendRet)
                {
                    RIALTO_SERVER_LOG_WARN("Failed to send step event for rendering preroll frame");
                }
                m_glibWrapper->gObjectSet(sink, kStepOnPrerollPropertyName.c_str(), 0, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Video sink doesn't have property `%s`", kStepOnPrerollPropertyName.c_str());
            }
            m_gstWrapper->gstObjectUnref(sink);
            m_context.pendingRenderFrame = false;
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending render frame, sink is NULL");
        }
    }
    return result;
}

bool GstGenericPlayer::setBufferingLimit()
{
    bool result{false};
    guint bufferingLimit{0};
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (!m_context.pendingBufferingLimit.has_value())
        {
            return false;
        }
        bufferingLimit = static_cast<guint>(m_context.pendingBufferingLimit.value());
    }

    GstElement *decoder{getDecoder(MediaSourceType::AUDIO)};
    if (decoder)
    {
        RIALTO_SERVER_LOG_DEBUG("Set limit-buffering-ms to %u", bufferingLimit);

        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "limit-buffering-ms"))
        {
            m_glibWrapper->gObjectSet(decoder, "limit-buffering-ms", bufferingLimit, nullptr);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to set limit-buffering-ms property on decoder '%s'",
                                    GST_ELEMENT_NAME(decoder));
        }
        m_gstWrapper->gstObjectUnref(decoder);
        std::unique_lock lock{m_context.propertyMutex};
        m_context.pendingBufferingLimit.reset();
    }
    else
    {
        RIALTO_SERVER_LOG_DEBUG("Pending limit-buffering-ms, decoder is NULL");
    }
    return result;
}

bool GstGenericPlayer::setUseBuffering()
{
    std::unique_lock lock{m_context.propertyMutex};
    if (m_context.pendingUseBuffering.has_value())
    {
        if (m_context.playbackGroup.m_curAudioDecodeBin)
        {
            gboolean useBufferingGboolean{m_context.pendingUseBuffering.value() ? TRUE : FALSE};
            RIALTO_SERVER_LOG_DEBUG("Set use-buffering to %d", useBufferingGboolean);
            m_glibWrapper->gObjectSet(m_context.playbackGroup.m_curAudioDecodeBin, "use-buffering",
                                      useBufferingGboolean, nullptr);
            m_context.pendingUseBuffering.reset();
            return true;
        }
        else
        {
            RIALTO_SERVER_LOG_DEBUG("Pending use-buffering, decodebin is NULL");
        }
    }
    return false;
}

bool GstGenericPlayer::setWesterossinkSecondaryVideo()
{
    bool result = false;
    GstElementFactory *factory = m_gstWrapper->gstElementFactoryFind("westerossink");
    if (factory)
    {
        GstElement *videoSink = m_gstWrapper->gstElementFactoryCreate(factory, nullptr);
        if (videoSink)
        {
            if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), "res-usage"))
            {
                m_glibWrapper->gObjectSet(videoSink, "res-usage", 0x0u, nullptr);
                m_glibWrapper->gObjectSet(m_context.pipeline, "video-sink", videoSink, nullptr);
                result = true;
            }
            else
            {
                RIALTO_SERVER_LOG_ERROR("Failed to set the westerossink res-usage");
                m_gstWrapper->gstObjectUnref(GST_OBJECT(videoSink));
            }
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the westerossink");
        }

        m_gstWrapper->gstObjectUnref(GST_OBJECT(factory));
    }
    else
    {
        // No westeros sink
        result = true;
    }

    return result;
}

bool GstGenericPlayer::setErmContext()
{
    bool result = false;
    GstContext *context = m_gstWrapper->gstContextNew("erm", false);
    if (context)
    {
        GstStructure *contextStructure = m_gstWrapper->gstContextWritableStructure(context);
        if (contextStructure)
        {
            m_gstWrapper->gstStructureSet(contextStructure, "res-usage", G_TYPE_UINT, 0x0u, nullptr);
            m_gstWrapper->gstElementSetContext(GST_ELEMENT(m_context.pipeline), context);
            result = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Failed to create the erm structure");
        }
        m_gstWrapper->gstContextUnref(context);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the erm context");
    }

    return result;
}

void GstGenericPlayer::startPositionReportingAndCheckAudioUnderflowTimer()
{
    if (m_positionReportingAndCheckAudioUnderflowTimer && m_positionReportingAndCheckAudioUnderflowTimer->isActive())
    {
        return;
    }

    m_positionReportingAndCheckAudioUnderflowTimer = m_timerFactory->createTimer(
        kPositionReportTimerMs,
        [this]()
        {
            if (m_workerThread)
            {
                m_workerThread->enqueueTask(m_taskFactory->createReportPosition(m_context, *this));
                m_workerThread->enqueueTask(m_taskFactory->createCheckAudioUnderflow(m_context, *this));
            }
        },
        firebolt::rialto::common::TimerType::PERIODIC);
}

void GstGenericPlayer::stopPositionReportingAndCheckAudioUnderflowTimer()
{
    if (m_positionReportingAndCheckAudioUnderflowTimer && m_positionReportingAndCheckAudioUnderflowTimer->isActive())
    {
        m_positionReportingAndCheckAudioUnderflowTimer->cancel();
        m_positionReportingAndCheckAudioUnderflowTimer.reset();
    }
}

void GstGenericPlayer::startNotifyPlaybackInfoTimer()
{
    static constexpr std::chrono::milliseconds kPlaybackInfoTimerMs{32};
    if (m_playbackInfoTimer && m_playbackInfoTimer->isActive())
    {
        return;
    }

    notifyPlaybackInfo();

    const auto kNotifyPlaybackInfo = [this]() { notifyPlaybackInfo(); };
    m_playbackInfoTimer = m_timerFactory->createTimer(kPlaybackInfoTimerMs, kNotifyPlaybackInfo,
                                                      firebolt::rialto::common::TimerType::PERIODIC);
}

void GstGenericPlayer::stopNotifyPlaybackInfoTimer()
{
    if (m_playbackInfoTimer && m_playbackInfoTimer->isActive())
    {
        m_playbackInfoTimer->cancel();
        m_playbackInfoTimer.reset();
    }
}

void GstGenericPlayer::startSubtitleClockResyncTimer()
{
    if (m_subtitleClockResyncTimer && m_subtitleClockResyncTimer->isActive())
    {
        return;
    }

    m_subtitleClockResyncTimer = m_timerFactory->createTimer(
        kSubtitleClockResyncInterval,
        [this]()
        {
            if (m_workerThread)
            {
                m_workerThread->enqueueTask(m_taskFactory->createSynchroniseSubtitleClock(m_context, *this));
            }
        },
        firebolt::rialto::common::TimerType::PERIODIC);
}

void GstGenericPlayer::stopSubtitleClockResyncTimer()
{
    if (m_subtitleClockResyncTimer && m_subtitleClockResyncTimer->isActive())
    {
        m_subtitleClockResyncTimer->cancel();
        m_subtitleClockResyncTimer.reset();
    }
}

void GstGenericPlayer::stopWorkerThread()
{
    if (m_workerThread)
    {
        m_workerThread->stop();
    }
}

void GstGenericPlayer::setPendingPlaybackRate()
{
    RIALTO_SERVER_LOG_INFO("Setting pending playback rate");
    setPlaybackRate(m_context.pendingPlaybackRate);
}

void GstGenericPlayer::renderFrame()
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createRenderFrame(m_context, *this));
    }
}

void GstGenericPlayer::setVolume(double targetVolume, uint32_t volumeDuration, firebolt::rialto::EaseType easeType)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createSetVolume(m_context, *this, targetVolume, volumeDuration, easeType));
    }
}

bool GstGenericPlayer::getVolume(double &currentVolume)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    if (!m_context.pipeline)
    {
        return false;
    }

    // NOTE: No gstreamer documentation for "fade-volume" could be found at the time this code was written.
    // Therefore the author performed several tests on a supported platform (Flex2) to determine the behaviour of this property.
    // The code has been written to be backwardly compatible on platforms that don't have this property.
    // The observed behaviour was:
    //    - if the returned fade volume is negative then audio-fade is not active. In this case the usual technique
    //      to find volume in the pipeline works and is used.
    //    - if the returned fade volume is positive then audio-fade is active. In this case the returned fade volume
    //      directly returns the current volume level 0=min to 100=max (and the pipeline's current volume level is
    //      meaningless and doesn't contribute in this case).
    GstElement *sink{getSink(MediaSourceType::AUDIO)};
    if (m_context.audioFadeEnabled && sink &&
        m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "fade-volume"))
    {
        gint fadeVolume{-100};
        m_glibWrapper->gObjectGet(sink, "fade-volume", &fadeVolume, NULL);
        if (fadeVolume < 0)
        {
            currentVolume = m_gstWrapper->gstStreamVolumeGetVolume(GST_STREAM_VOLUME(m_context.pipeline),
                                                                   GST_STREAM_VOLUME_FORMAT_LINEAR);
            RIALTO_SERVER_LOG_INFO("Fade volume is negative, using volume from pipeline: %f", currentVolume);
        }
        else
        {
            currentVolume = static_cast<double>(fadeVolume) / 100.0;
            RIALTO_SERVER_LOG_INFO("Fade volume is supported: %f", currentVolume);
        }
        m_context.audioFadeVolume = currentVolume;
    }
    else
    {
        currentVolume = m_gstWrapper->gstStreamVolumeGetVolume(GST_STREAM_VOLUME(m_context.pipeline),
                                                               GST_STREAM_VOLUME_FORMAT_LINEAR);
        RIALTO_SERVER_LOG_INFO("Fade volume is not supported, using volume from pipeline: %f", currentVolume);
    }

    if (sink)
        m_gstWrapper->gstObjectUnref(sink);

    return true;
}

void GstGenericPlayer::setMute(const MediaSourceType &mediaSourceType, bool mute)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetMute(m_context, *this, mediaSourceType, mute));
    }
}

bool GstGenericPlayer::getMute(const MediaSourceType &mediaSourceType, bool &mute)
{
    // We are on main thread here, but m_context.pipeline can be used, because it's modified only in GstGenericPlayer
    // constructor and destructor. GstGenericPlayer is created/destructed on main thread, so we won't have a crash here.
    if (mediaSourceType == MediaSourceType::SUBTITLE)
    {
        if (!m_context.subtitleSink)
        {
            RIALTO_SERVER_LOG_ERROR("There is no subtitle sink");
            return false;
        }
        gboolean muteValue{FALSE};
        m_glibWrapper->gObjectGet(m_context.subtitleSink, "mute", &muteValue, nullptr);
        mute = muteValue;
    }
    else if (mediaSourceType == MediaSourceType::AUDIO)
    {
        if (!m_context.pipeline)
        {
            return false;
        }
        mute = m_gstWrapper->gstStreamVolumeGetMute(GST_STREAM_VOLUME(m_context.pipeline));
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Getting mute for type %s unsupported", common::convertMediaSourceType(mediaSourceType));
        return false;
    }

    return true;
}

bool GstGenericPlayer::isAsync(const MediaSourceType &mediaSourceType) const
{
    GstElement *sink = getSink(mediaSourceType);
    if (!sink)
    {
        RIALTO_SERVER_LOG_WARN("Sink not found for %s", common::convertMediaSourceType(mediaSourceType));
        return true; // Our sinks are async by default
    }
    gboolean returnValue{TRUE};
    m_glibWrapper->gObjectGet(sink, "async", &returnValue, nullptr);
    m_gstWrapper->gstObjectUnref(sink);
    return returnValue == TRUE;
}

void GstGenericPlayer::setTextTrackIdentifier(const std::string &textTrackIdentifier)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetTextTrackIdentifier(m_context, textTrackIdentifier));
    }
}

bool GstGenericPlayer::getTextTrackIdentifier(std::string &textTrackIdentifier)
{
    if (!m_context.subtitleSink)
    {
        RIALTO_SERVER_LOG_ERROR("There is no subtitle sink");
        return false;
    }

    gchar *identifier = nullptr;
    m_glibWrapper->gObjectGet(m_context.subtitleSink, "text-track-identifier", &identifier, nullptr);

    if (identifier)
    {
        textTrackIdentifier = identifier;
        m_glibWrapper->gFree(identifier);
        return true;
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to get text track identifier");
        return false;
    }
}

bool GstGenericPlayer::setLowLatency(bool lowLatency)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetLowLatency(m_context, *this, lowLatency));
    }
    return true;
}

bool GstGenericPlayer::setSync(bool sync)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSync(m_context, *this, sync));
    }
    return true;
}

bool GstGenericPlayer::getSync(bool &sync)
{
    bool returnValue{false};
    GstElement *sink{getSink(MediaSourceType::AUDIO)};
    if (sink)
    {
        if (m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(sink), "sync"))
        {
            m_glibWrapper->gObjectGet(sink, "sync", &sync, nullptr);
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Sync not supported in sink '%s'", GST_ELEMENT_NAME(sink));
        }
        m_gstWrapper->gstObjectUnref(sink);
    }
    else if (m_context.pendingSync.has_value())
    {
        RIALTO_SERVER_LOG_DEBUG("Returning queued value");
        sync = m_context.pendingSync.value();
        returnValue = true;
    }
    else
    {
        // We dont know the default setting on the sync, so return failure here
        RIALTO_SERVER_LOG_WARN("No audio sink attached or queued value");
    }

    return returnValue;
}

bool GstGenericPlayer::setSyncOff(bool syncOff)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSyncOff(m_context, *this, syncOff));
    }
    return true;
}

bool GstGenericPlayer::setStreamSyncMode(const MediaSourceType &mediaSourceType, int32_t streamSyncMode)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createSetStreamSyncMode(m_context, *this, mediaSourceType, streamSyncMode));
    }
    return true;
}

bool GstGenericPlayer::getStreamSyncMode(int32_t &streamSyncMode)
{
    bool returnValue{false};
    GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
    if (decoder && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "stream-sync-mode"))
    {
        m_glibWrapper->gObjectGet(decoder, "stream-sync-mode", &streamSyncMode, nullptr);
        returnValue = true;
    }
    else
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (m_context.pendingStreamSyncMode.find(MediaSourceType::AUDIO) != m_context.pendingStreamSyncMode.end())
        {
            RIALTO_SERVER_LOG_DEBUG("Returning queued value");
            streamSyncMode = m_context.pendingStreamSyncMode[MediaSourceType::AUDIO];
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("Stream sync mode not supported in decoder '%s'",
                                    (decoder ? GST_ELEMENT_NAME(decoder) : "null"));
        }
    }

    if (decoder)
        m_gstWrapper->gstObjectUnref(GST_OBJECT(decoder));

    return returnValue;
}

void GstGenericPlayer::ping(std::unique_ptr<IHeartbeatHandler> &&heartbeatHandler)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createPing(std::move(heartbeatHandler)));
    }
}

void GstGenericPlayer::flush(const MediaSourceType &mediaSourceType, bool resetTime, bool &async)
{
    if (m_workerThread)
    {
        async = isAsync(mediaSourceType);
        m_flushWatcher->setFlushing(mediaSourceType, async);
        m_workerThread->enqueueTask(m_taskFactory->createFlush(m_context, *this, mediaSourceType, resetTime, async));
    }
}

void GstGenericPlayer::setSourcePosition(const MediaSourceType &mediaSourceType, int64_t position, bool resetTime,
                                         double appliedRate, uint64_t stopPosition)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSourcePosition(m_context, mediaSourceType, position,
                                                                           resetTime, appliedRate, stopPosition));
    }
}

void GstGenericPlayer::setSubtitleOffset(int64_t position)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetSubtitleOffset(m_context, position));
    }
}

void GstGenericPlayer::processAudioGap(int64_t position, uint32_t duration, int64_t discontinuityGap, bool audioAac)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(
            m_taskFactory->createProcessAudioGap(m_context, position, duration, discontinuityGap, audioAac));
    }
}

void GstGenericPlayer::setBufferingLimit(uint32_t limitBufferingMs)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetBufferingLimit(m_context, *this, limitBufferingMs));
    }
}

bool GstGenericPlayer::getBufferingLimit(uint32_t &limitBufferingMs)
{
    bool returnValue{false};
    GstElement *decoder = getDecoder(MediaSourceType::AUDIO);
    if (decoder && m_glibWrapper->gObjectClassFindProperty(G_OBJECT_GET_CLASS(decoder), "limit-buffering-ms"))
    {
        m_glibWrapper->gObjectGet(decoder, "limit-buffering-ms", &limitBufferingMs, nullptr);
        returnValue = true;
    }
    else
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (m_context.pendingBufferingLimit.has_value())
        {
            RIALTO_SERVER_LOG_DEBUG("Returning queued value");
            limitBufferingMs = m_context.pendingBufferingLimit.value();
            returnValue = true;
        }
        else
        {
            RIALTO_SERVER_LOG_ERROR("buffering limit not supported in decoder '%s'",
                                    (decoder ? GST_ELEMENT_NAME(decoder) : "null"));
        }
    }

    if (decoder)
        m_gstWrapper->gstObjectUnref(GST_OBJECT(decoder));

    return returnValue;
}

void GstGenericPlayer::setUseBuffering(bool useBuffering)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSetUseBuffering(m_context, *this, useBuffering));
    }
}

bool GstGenericPlayer::getUseBuffering(bool &useBuffering)
{
    if (m_context.playbackGroup.m_curAudioDecodeBin)
    {
        m_glibWrapper->gObjectGet(m_context.playbackGroup.m_curAudioDecodeBin, "use-buffering", &useBuffering, nullptr);
        return true;
    }
    else
    {
        std::unique_lock lock{m_context.propertyMutex};
        if (m_context.pendingUseBuffering.has_value())
        {
            RIALTO_SERVER_LOG_DEBUG("Returning queued value");
            useBuffering = m_context.pendingUseBuffering.value();
            return true;
        }
    }
    return false;
}

void GstGenericPlayer::switchSource(const std::unique_ptr<IMediaPipeline::MediaSource> &mediaSource)
{
    if (m_workerThread)
    {
        m_workerThread->enqueueTask(m_taskFactory->createSwitchSource(*this, mediaSource));
    }
}

void GstGenericPlayer::handleBusMessage(GstMessage *message)
{
    m_workerThread->enqueueTask(m_taskFactory->createHandleBusMessage(m_context, *this, message, *m_flushWatcher));
}

void GstGenericPlayer::updatePlaybackGroup(GstElement *typefind, const GstCaps *caps)
{
    if (m_workerThread)
    {
        m_gstWrapper->gstObjectRef(typefind);
        GstCaps *ownedCaps{caps ? m_gstWrapper->gstCapsCopy(caps) : nullptr};
        m_workerThread->enqueueTask(m_taskFactory->createUpdatePlaybackGroup(m_context, *this, typefind, ownedCaps));
    }
}

void GstGenericPlayer::addAutoVideoSinkChild(GObject *object)
{
    // Only add children that are sinks
    if (GST_OBJECT_FLAG_IS_SET(GST_ELEMENT(object), GST_ELEMENT_FLAG_SINK))
    {
        RIALTO_SERVER_LOG_DEBUG("Store AutoVideoSink child sink");

        if (m_context.autoVideoChildSink && m_context.autoVideoChildSink != GST_ELEMENT(object))
        {
            RIALTO_SERVER_LOG_MIL("AutoVideoSink child is been overwritten");
        }
        m_context.autoVideoChildSink = GST_ELEMENT(object);
    }
}

void GstGenericPlayer::addAutoAudioSinkChild(GObject *object)
{
    // Only add children that are sinks
    if (GST_OBJECT_FLAG_IS_SET(GST_ELEMENT(object), GST_ELEMENT_FLAG_SINK))
    {
        RIALTO_SERVER_LOG_DEBUG("Store AutoAudioSink child sink");

        if (m_context.autoAudioChildSink && m_context.autoAudioChildSink != GST_ELEMENT(object))
        {
            RIALTO_SERVER_LOG_MIL("AutoAudioSink child is been overwritten");
        }
        m_context.autoAudioChildSink = GST_ELEMENT(object);
    }
}

void GstGenericPlayer::removeAutoVideoSinkChild(GObject *object)
{
    if (GST_OBJECT_FLAG_IS_SET(GST_ELEMENT(object), GST_ELEMENT_FLAG_SINK))
    {
        RIALTO_SERVER_LOG_DEBUG("Remove AutoVideoSink child sink");

        if (m_context.autoVideoChildSink && m_context.autoVideoChildSink != GST_ELEMENT(object))
        {
            RIALTO_SERVER_LOG_MIL("AutoVideoSink child sink is not the same as the one stored");
            return;
        }

        m_context.autoVideoChildSink = nullptr;
    }
}

void GstGenericPlayer::removeAutoAudioSinkChild(GObject *object)
{
    if (GST_OBJECT_FLAG_IS_SET(GST_ELEMENT(object), GST_ELEMENT_FLAG_SINK))
    {
        RIALTO_SERVER_LOG_DEBUG("Remove AutoAudioSink child sink");

        if (m_context.autoAudioChildSink && m_context.autoAudioChildSink != GST_ELEMENT(object))
        {
            RIALTO_SERVER_LOG_MIL("AutoAudioSink child sink is not the same as the one stored");
            return;
        }

        m_context.autoAudioChildSink = nullptr;
    }
}

GstElement *GstGenericPlayer::getSinkChildIfAutoVideoSink(GstElement *sink) const
{
    const gchar *kTmpName = m_glibWrapper->gTypeName(G_OBJECT_TYPE(sink));
    if (!kTmpName)
        return sink;

    const std::string kElementTypeName{kTmpName};
    if (kElementTypeName == "GstAutoVideoSink")
    {
        if (!m_context.autoVideoChildSink)
        {
            RIALTO_SERVER_LOG_WARN("No child sink has been added to the autovideosink");
        }
        else
        {
            return m_context.autoVideoChildSink;
        }
    }
    return sink;
}

GstElement *GstGenericPlayer::getSinkChildIfAutoAudioSink(GstElement *sink) const
{
    const gchar *kTmpName = m_glibWrapper->gTypeName(G_OBJECT_TYPE(sink));
    if (!kTmpName)
        return sink;

    const std::string kElementTypeName{kTmpName};
    if (kElementTypeName == "GstAutoAudioSink")
    {
        if (!m_context.autoAudioChildSink)
        {
            RIALTO_SERVER_LOG_WARN("No child sink has been added to the autoaudiosink");
        }
        else
        {
            return m_context.autoAudioChildSink;
        }
    }
    return sink;
}

void GstGenericPlayer::setPlaybinFlags(bool enableAudio)
{
    unsigned flags = getGstPlayFlag("video") | getGstPlayFlag("native-video") | getGstPlayFlag("text");

    if (enableAudio)
    {
        flags |= getGstPlayFlag("audio");
        flags |= shouldEnableNativeAudio() ? getGstPlayFlag("native-audio") : 0;
    }

    m_glibWrapper->gObjectSet(m_context.pipeline, "flags", flags, nullptr);
}

bool GstGenericPlayer::shouldEnableNativeAudio()
{
    const char *rdkaiNoNative = getenv("RDKAI_NO_NATIVE_AUDIO"); // RDKAI prototype
    if (rdkaiNoNative && rdkaiNoNative[0] == '1')
    {
        return false;
    }
    GstElementFactory *factory = m_gstWrapper->gstElementFactoryFind("brcmaudiosink");
    if (factory)
    {
        m_gstWrapper->gstObjectUnref(GST_OBJECT(factory));
        return true;
    }
    return false;
}

}; // namespace firebolt::rialto::server
