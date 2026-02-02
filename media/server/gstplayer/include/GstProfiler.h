#ifndef FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
#define FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_

#include "Profiler.h"
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
    using Profiler = firebolt::rialto::common::Profiler;
    using RecordId = Profiler::RecordId;

    enum class Stage {AppSrc, Decryptor, Decoder};

    GstProfiler(GstElement* pipeline,
                const std::shared_ptr<IGstWrapper> &gstWrapper,
                const std::shared_ptr<IGlibWrapper> &glibWrapper);
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
    Profiler m_profiler;
    std::shared_ptr<IGstWrapper> m_gstWrapper;
    std::shared_ptr<IGlibWrapper> m_glibWrapper;
    static constexpr std::string_view k_module = "GstProfiler";
};
} // namespace firebolt::rialto::server
#endif // FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
