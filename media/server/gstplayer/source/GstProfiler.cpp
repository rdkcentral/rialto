#include "GstProfiler.h"

#include <mutex>
#include <string>

#include <gst/gst.h>
#include <glib.h>

namespace firebolt::rialto::server
{
inline constexpr std::array kKlassTokens
{
    std::string_view{"Source"},
    std::string_view{"Decryptor"},
    std::string_view{"Decoder"},
};

GstProfiler::GstProfiler(GstElement* pipeline,
                        const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                        const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper)
    :   m_pipeline{pipeline},
        m_id{generateId()},
        m_profiler{k_module, m_id},
        m_gstWrapper{gstWrapper},
        m_glibWrapper{glibWrapper}
{
    if (m_pipeline)
        m_gstWrapper->gstObjectRef(m_pipeline);
}


GstProfiler::~GstProfiler()
{
    if (m_pipeline)
        m_gstWrapper->gstObjectUnref(m_pipeline);
}

uint64_t GstProfiler::generateId()
{
    using namespace std::chrono;

    return duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();
}

void GstProfiler::createRecord(std::string stage)
{
    m_profiler.record(stage);
}

void GstProfiler::scheduleGstElementRecord(GstElement* element)
{
    if (!element)
        return;

    if(!checkElement(element))
        return;

    GstPad* pad = m_gstWrapper->gstElementGetStaticPad(element, "src");
    if (!pad)
        return;

    auto* probeCtx = new ProbeCtx{this, std::string(m_gstWrapper->gstElementGetName(element))};

    gst_pad_add_probe(pad,
                    GST_PAD_PROBE_TYPE_BUFFER,
                    &GstProfiler::probeCb,
                    probeCtx,
                    &GstProfiler::probeCtxDestroy);

    m_gstWrapper->gstObjectUnref(pad);
}

GstPadProbeReturn GstProfiler::probeCb(GstPad* pad, GstPadProbeInfo* info, gpointer user_data)
{
    auto* probeCtx = static_cast<ProbeCtx*>(user_data);
    GstProfiler* self = probeCtx->self;
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    if (!(info->type & GST_PAD_PROBE_TYPE_BUFFER))
      return GST_PAD_PROBE_OK;

    if (!buffer)
      return GST_PAD_PROBE_OK;

    self->m_profiler.record(probeCtx->stage);

    return GST_PAD_PROBE_REMOVE;
}

void GstProfiler::probeCtxDestroy(gpointer data)
{
    delete static_cast<ProbeCtx*>(data);
}

bool GstProfiler::checkElement(GstElement* element)
{
    GstElementFactory* factory = m_gstWrapper->gstElementGetFactory(element);
    if (!factory)
        return false;

    const gchar* klass = gst_element_factory_get_klass(factory);
    if (!klass)
        return false;

    for (auto token : kKlassTokens)
    {
        if (g_strrstr(klass, token.data()) != nullptr)
        {
            return true;
        }
    }

    return false;
}

} // namespace firebolt::rialto::server
