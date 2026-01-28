#ifndef FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
#define FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_

#include "Profiler.h"
#include "IGstWrapper.h"
#include "IGlibWrapper.h"

#include <gst/gst.h>

#include <memory>

namespace firebolt::rialto::server
{
class GstProfiler
{
public:
    enum class Stage {AppSrc, Decryptor, Decoder};

    GstProfiler(GstElement* pipeline,
                const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper);
    ~GstProfiler();

    void createRecord(std::string stage);
    void scheduleGstElementRecord(GstElement* element);

private:
    struct ProbeCtx {
        GstProfiler* self;
        std::string stage;
    };

    static uint64_t generateId();
    bool checkElement(GstElement* element);

    static GstPadProbeReturn probeCb(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static void probeCtxDestroy(gpointer data);

    GstElement* m_pipeline = nullptr;
    uint64_t m_id;
    Profiler m_profiler;
    std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> m_gstWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> m_glibWrapper;
    static constexpr std::string_view k_module = "GstProfiler";
};
} // namespace firebolt::rialto::server
#endif // FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
