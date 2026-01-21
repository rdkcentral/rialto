#ifndef FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
#define FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_

#include "Profiler.h"

#include <gst/gst.h>

#include <map>
#include <mutex>

namespace firebolt::rialto::server
{
class GstProfiler
{
public:
    enum class Stage {AppSrc, Decryptor, Decoder};

    GstProfiler(GstElement* pipeline);
    ~GstProfiler();

    void scheduleRecord(GstElement* element);

private:
    struct ProbeCtx {
        GstProfiler* self;
        std::string stage;
    };

    static uint64_t generateId();
    static bool checkElement(GstElement* element);

    static GstPadProbeReturn probeCb(GstPad* pad, GstPadProbeInfo* info, gpointer user_data);
    static void probeCtxDestroy(gpointer data);

    GstElement* m_pipeline = nullptr;
    uint64_t m_id;
    Profiler m_profiler;
    static constexpr std::string_view k_module = "GstProfiler";
};
} // namespace firebolt::rialto::server
#endif // FIREBOLT_RIALTO_SERVER_GST_PROFILER_H_
