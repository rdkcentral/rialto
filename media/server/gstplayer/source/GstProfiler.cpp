/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "GstProfiler.h"
#include "RialtoCommonLogging.h"

#include <mutex>
#include <string>

#include <glib.h>
#include <gst/gst.h>

namespace firebolt::rialto::server
{
inline constexpr std::array kKlassTokens{
    std::string_view{"Source"},
    std::string_view{"Decryptor"},
    std::string_view{"Decoder"},
};

GstProfiler::GstProfiler(GstElement *pipeline, const std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> &gstWrapper,
                         const std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> &glibWrapper)
    : m_pipeline{pipeline}, m_gstWrapper{gstWrapper}, m_glibWrapper{glibWrapper}
{
    auto profilerFactory = firebolt::rialto::common::IProfilerFactory::createFactory();
    m_profiler = profilerFactory ? profilerFactory->createProfiler(std::string{k_module}) : nullptr;
    m_enabled = (m_profiler != nullptr) && m_profiler->enabled();

    if (m_enabled && m_pipeline)
        m_gstWrapper->gstObjectRef(m_pipeline);
}

GstProfiler::~GstProfiler()
{
    if (m_enabled && m_pipeline)
        m_gstWrapper->gstObjectUnref(m_pipeline);
}

std::optional<GstProfiler::RecordId> GstProfiler::createRecord(std::string stage)
{
    if (!m_enabled || !m_profiler)
        return std::nullopt;

    auto id = m_profiler->record(std::move(stage));
    if (!id)
        return std::nullopt;

    return static_cast<GstProfiler::RecordId>(*id);
}

std::optional<GstProfiler::RecordId> GstProfiler::createRecord(std::string stage, std::string info)
{
    if (!m_enabled || !m_profiler)
        return std::nullopt;

    auto id = m_profiler->record(std::move(stage), std::move(info));
    if (!id)
        return std::nullopt;

    return static_cast<GstProfiler::RecordId>(*id);
}

void GstProfiler::scheduleGstElementRecord(GstElement *element)
{
    if (!m_enabled || !m_profiler)
        return;

    if (!element)
        return;

    auto stage = checkElement(element);
    if (!stage)
        return;

    GstPad *pad = m_gstWrapper->gstElementGetStaticPad(element, "src");
    if (!pad)
        return;

    gchar *rawName = m_gstWrapper->gstElementGetName(element);
    std::string elementName = rawName ? rawName : "<null>";
    if (rawName)
        m_glibWrapper->gFree(rawName);

    auto *probeCtx = new ProbeCtx{this, stage.value(), std::move(elementName)};
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, &GstProfiler::probeCb, probeCtx, &GstProfiler::probeCtxDestroy);

    m_gstWrapper->gstObjectUnref(pad);
}

void GstProfiler::logRecord(GstProfiler::RecordId id)
{
    if (!m_enabled || !m_profiler)
        return;

    m_profiler->log(static_cast<firebolt::rialto::common::IProfiler::RecordId>(id));
}

GstPadProbeReturn GstProfiler::probeCb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
    auto *probeCtx = static_cast<ProbeCtx *>(user_data);
    GstProfiler *self = probeCtx->self;
    GstBuffer *buffer = GST_PAD_PROBE_INFO_BUFFER(info);

    if (!(info->type & GST_PAD_PROBE_TYPE_BUFFER))
        return GST_PAD_PROBE_OK;

    if (!buffer)
        return GST_PAD_PROBE_OK;

    const auto id = self->m_profiler->record(probeCtx->stage, probeCtx->info);
    if (id)
    {
        self->m_profiler->log(id.value());
    }

    return GST_PAD_PROBE_REMOVE;
}

void GstProfiler::probeCtxDestroy(gpointer data)
{
    delete static_cast<ProbeCtx *>(data);
}

std::optional<std::string> GstProfiler::checkElement(GstElement *element)
{
    GstElementFactory *factory = m_gstWrapper->gstElementGetFactory(element);
    if (!factory)
        return std::nullopt;

    const gchar *klass = gst_element_factory_get_klass(factory);
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
