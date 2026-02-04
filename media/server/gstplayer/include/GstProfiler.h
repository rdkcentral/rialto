#ifndef FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
#define FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_

#include "IProfiler.h"
#include "IGstWrapper.h"
#include "IGlibWrapper.h"

#include <gst/gst.h>

#include <memory>
#include <optional>

namespace firebolt::rialto::server
{
class GstProfiler
{
public:
    using IGstWrapper = firebolt::rialto::wrappers::IGstWrapper;
    using IGlibWrapper = firebolt::rialto::wrappers::IGlibWrapper;
    using IProfilerFactory = firebolt::rialto::common::IProfilerFactory;
    using IProfiler = firebolt::rialto::common::IProfiler;
    using RecordId = IProfiler::RecordId;

    enum class Stage {AppSrc, Decryptor, Decoder};

    GstProfiler(GstElement* pipeline,
                const std::shared_ptr<IGstWrapper> &gstWrapper,
                const std::shared_ptr<IGlibWrapper> &glibWrapper,
                std::shared_ptr<IProfilerFactory> profilerFactory);
    ~GstProfiler();

    std::optional<RecordId> createRecord(std::string stage);
    std::optional<RecordId> createRecord(std::string stage, std::string info);

    void scheduleGstElementRecord(GstElement* element);

    void logRecord(const RecordId id);

private:
    struct ProbeCtx {
        GstProfiler* self;
        std::string stage;
        std::string info;
    };

    std::optional<std::string> checkElement(GstElement* element);

    static GstPadProbeReturn probeCb(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static void probeCtxDestroy(gpointer data);

    GstElement* m_pipeline = nullptr;
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
    std::shared_ptr<IProfilerFactory> m_profilerFactory;
    std::unique_ptr<IProfiler> m_profiler;
    static constexpr std::string_view k_module = "GstProfiler";
};
} // namespace firebolt::rialto::server
#endif // FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
