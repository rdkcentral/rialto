#include "GstProfiler.h"
#include "RialtoCommonLogging.h"

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
                        const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper,
                        std::shared_ptr<IProfilerFactory> profilerFactory)
    :   m_pipeline{pipeline},
        m_gstWrapper{gstWrapper},
        m_glibWrapper{glibWrapper},
        m_profilerFactory{profilerFactory}
{
    if (m_pipeline)
        m_gstWrapper->gstObjectRef(m_pipeline);

    if (m_profilerFactory)
        m_profiler = m_profilerFactory->createProfiler(std::string{k_module});

    if (m_profiler)
        m_enabled = m_profiler->enabled();
}


GstProfiler::~GstProfiler()
{
    if (m_pipeline)
        m_gstWrapper->gstObjectUnref(m_pipeline);
}

std::optional<GstProfiler::RecordId> GstProfiler::createRecord(std::string stage)
{
    if (!m_enabled) return std::nullopt;

    return m_profiler->record(stage);
}

std::optional<GstProfiler::RecordId> GstProfiler::createRecord(std::string stage, std::string info)
{
    if (!m_enabled) return std::nullopt;

    return m_profiler->record(stage, info);
}

void GstProfiler::scheduleGstElementRecord(GstElement* element)
{
    if (!m_enabled) return;

    if (!element)
        return;

    auto stage = checkElement(element);
    if(!stage)
        return;

    GstPad* pad = m_gstWrapper->gstElementGetStaticPad(element, "src");
    if (!pad)
        return;

    auto* probeCtx = new ProbeCtx{this, stage.value_or(""), std::string(m_gstWrapper->gstElementGetName(element))};
    gst_pad_add_probe(pad,
                    GST_PAD_PROBE_TYPE_BUFFER,
                    &GstProfiler::probeCb,
                    probeCtx,
                    &GstProfiler::probeCtxDestroy);

    m_gstWrapper->gstObjectUnref(pad);
}

void GstProfiler::logRecord(GstProfiler::RecordId id)
{
    if (!m_enabled) return;

    m_profiler->log(id);
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

    const auto id = self->m_profiler->record(probeCtx->stage, probeCtx->info);
    if(id)
    {
        self->m_profiler->log(id.value());
    }

    return GST_PAD_PROBE_REMOVE;
}

void GstProfiler::probeCtxDestroy(gpointer data)
{
    delete static_cast<ProbeCtx*>(data);
}

std::optional<std::string> GstProfiler::checkElement(GstElement* element)
{
    GstElementFactory* factory = m_gstWrapper->gstElementGetFactory(element);
    if (!factory)
        return std::nullopt;

    const gchar* klass = gst_element_factory_get_klass(factory);
    if (!klass)
        return std::nullopt;

    for (auto token : kKlassTokens)
    {
        if (g_strrstr(klass, token.data()) != nullptr)
        {
            return std::string(token.data()) + " FB Exit";
        }
    }

    return std::nullopt;
}

} // namespace firebolt::rialto::server
