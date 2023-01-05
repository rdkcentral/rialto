

#include "RialtoServerLogging.h"
#include "TracingCategories.h"
#include "RialtoPerfetto.h"
#include <condition_variable>

//#define ENABLE_BACKEND_TRACING
#define ENABLE_SYSTEM_TRACING

/*
 * 
data_sources: {
    config {
        name: "linux.process_stats"
        target_buffer: 1
        process_stats_config {
            scan_all_processes_on_start: true
        }
    }
}
data_sources: {
    config {
        name: "linux.sys_stats"
        sys_stats_config {
            stat_period_ms: 1000
            stat_counters: STAT_CPU_TIMES
            stat_counters: STAT_FORK_COUNT
        }
    }
}
data_sources: {
    config {
        name: "linux.ftrace"
        ftrace_config {
            ftrace_events: "sched/sched_switch"
            ftrace_events: "sched/sched_wakeup"
            ftrace_events: "sched/sched_wakeup_new"
            ftrace_events: "sched/sched_waking"
            ftrace_events: "power/suspend_resume"
            ftrace_events: "power/cpu_frequency"
            ftrace_events: "power/cpu_idle"
            ftrace_events: "sched/sched_process_exit"
            ftrace_events: "sched/sched_process_free"
            ftrace_events: "task/task_newtask"
            ftrace_events: "task/task_rename"
        }
    }
}
*/
namespace firebolt::rialto::server
{
#ifdef ENABLE_BACKEND_TRACING
    class Observer : public perfetto::TrackEventSessionObserver 
    {
        public:
            Observer() { perfetto::TrackEvent::AddSessionObserver(this); }
            ~Observer() override { perfetto::TrackEvent::RemoveSessionObserver(this); }

            void OnStart(const perfetto::DataSourceBase::StartArgs&) override {
                std::unique_lock<std::mutex> lock(mutex);
                cv.notify_one();
            }

            void WaitForTracingStart() {
                PERFETTO_LOG("Waiting for tracing to start...");
                std::unique_lock<std::mutex> lock(mutex);
                cv.wait(lock, [] { return perfetto::TrackEvent::IsEnabled(); });
                PERFETTO_LOG("Tracing started");
            }

            void OnStop(const perfetto::DataSourceBase::StopArgs&) override {

            }
            std::mutex mutex;
            std::condition_variable cv;
    };
#endif
    // The definition of rialto data source. Instances of this class will be
    // automatically created and destroyed by Perfetto.
    class RialtoDataSource : public perfetto::DataSource<RialtoDataSource> {
        public:
            void OnSetup(const SetupArgs&) override {
                // Use this callback to apply any custom configuration to your data source
                // based on the TraceConfig in SetupArgs.
            }

            // Optional callbacks for tracking the lifecycle of the data source.
            void OnStart(const StartArgs&) override {}
            void OnStop(const StopArgs&) override {}
    };

    RialtoPerfetto::RialtoPerfetto()
    {
        
    }

    RialtoPerfetto::~RialtoPerfetto()
    {
#ifndef ENABLE_BACKEND_TRACING
        PERFETTO_LOG("Stopping tracing session...");
        RIALTO_SERVER_LOG_DEBUG("Stopping tracing session %p", m_tracingSession.get());
        StopTracing(std::move(m_tracingSession));
#endif
    }

    void RialtoPerfetto::InitializePerfetto() 
    {
        RIALTO_SERVER_LOG_DEBUG("Initializing Perfetto");
        PERFETTO_LOG("Initializing Perfetto");
        perfetto::TracingInitArgs args;
        // The backends determine where trace events are recorded. For this example we
        // are going to use the in-process tracing service, which only includes in-app
        // events.
#ifdef ENABLE_BACKEND_TRACING
        args.backends = perfetto::kSystemBackend;
#else
        //args.backends |= perfetto::kInProcessBackend;
        args.backends = perfetto::kSystemBackend;
#endif
        perfetto::Tracing::Initialize(args);
#if 0
        // Register our custom data source. Only the name is required, but other
        // properties can be advertised too.
        perfetto::DataSourceDescriptor dsd;
        dsd.set_name("rialto_data_source");
        RialtoDataSource::Register(dsd);
#endif
        perfetto::TrackEvent::Register();
    }

    //std::unique_ptr<perfetto::TracingSession> RialtoPerfetto::StartTracing() 
    void RialtoPerfetto::StartTracing() 
    {
        PERFETTO_LOG("Starting Tracing");
        // The trace config defines which types of data sources are enabled for
        // recording. In this example we enable the rialto data source we registered
        // above.
        perfetto::TraceConfig cfg;
        cfg.set_duration_ms(60000);
        //cfg.add_buffers()->set_fill_policy(perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy_RING_BUFFER);
        //cfg.add_buffers()->set_fill_policy(perfetto::protos::gen::TraceConfig_BufferConfig_FillPolicy_DISCARD);
        cfg.add_buffers()->set_size_kb(20480);
        //cfg.add_buffers()->set_size_kb(1048);
#if 0
        auto* ds_cfg_1 = cfg.add_data_sources()->mutable_config();
        ds_cfg_1->set_name("rialto_data_source");
        ds_cfg_1->set_target_buffer(0);
#endif
#ifdef ENABLE_SYSTEM_TRACING
        auto* ds_cfg_2 = cfg.add_data_sources()->mutable_config();
        ds_cfg_2->set_name("linux.ftrace");
        ds_cfg_2->set_target_buffer(0);

        perfetto::protos::gen::FtraceConfig ftrace_config;
        ftrace_config.add_ftrace_events("sched/sched_switch");
        ftrace_config.add_ftrace_events("sched/sched_waking");
        ftrace_config.add_ftrace_events("sched/sched_wakeup_new");
        ftrace_config.add_ftrace_events("task/task_newtask");
        ftrace_config.add_ftrace_events("task/task_rename");
        ftrace_config.add_ftrace_events("sched/sched_process_exec");
        ftrace_config.add_ftrace_events("sched/sched_process_exit");
        ftrace_config.add_ftrace_events("sched/sched_process_fork");
        ftrace_config.add_ftrace_events("sched/sched_process_free");
        ftrace_config.add_ftrace_events("sched/sched_process_hang");
        ftrace_config.add_ftrace_events("sched/sched_process_wait");
        ftrace_config.add_ftrace_events("power/suspend_resume");
        ftrace_config.add_ftrace_events("power/cpu_frequency");
        ftrace_config.add_ftrace_events("power/cpu_idle");
           
        ds_cfg_2->set_ftrace_config_raw(ftrace_config.SerializeAsString());
#if 1
        auto* ds_cfg_4 = cfg.add_data_sources()->mutable_config();
        ds_cfg_4->set_name("linux.sys_stats");

        perfetto::protos::gen::SysStatsConfig sys_stats_config;
        sys_stats_config.set_stat_period_ms(1000);
        sys_stats_config.add_stat_counters(perfetto::protos::gen::SysStatsConfig::STAT_FORK_COUNT);
        sys_stats_config.add_stat_counters(perfetto::protos::gen::SysStatsConfig::STAT_CPU_TIMES);
        ds_cfg_4->set_sys_stats_config_raw(sys_stats_config.SerializeAsString());
 
        auto* ds_cfg_5 = cfg.add_data_sources()->mutable_config();
        ds_cfg_5->set_name("linux.process_stats");

        perfetto::protos::gen::ProcessStatsConfig process_stats_config;
        process_stats_config.set_scan_all_processes_on_start(true);
        ds_cfg_5->set_process_stats_config_raw(process_stats_config.SerializeAsString());
#endif
#endif

#ifndef ENABLE_BACKEND_TRACING
        auto* ds_cfg_3 = cfg.add_data_sources()->mutable_config();
        ds_cfg_3->set_name("track_event");
        //ds_cfg_3->set_target_buffer(0);

        //perfetto::protos::gen::TrackEventConfig track_event_config;
        //track_event_config.add_disabled_categories("*");
        //track_event_config.add_enabled_categories("GstMediaPipeline");
        //ds_cfg_3->set_track_event_config_raw(track_event_config.SerializeAsString());
#endif
        m_tracingSession = perfetto::Tracing::NewTrace();
        m_tracingSession->Setup(cfg);
        m_tracingSession->StartBlocking();
        RIALTO_SERVER_LOG_DEBUG("Tracing session %p started ", m_tracingSession.get());
        PERFETTO_LOG("Tracing session %p started ", m_tracingSession.get());
        //return m_tracingSession;
    }

    void RialtoPerfetto::StopTracing(std::unique_ptr<perfetto::TracingSession> tracingSession) 
    {
#if 0
        // Flush to make sure the last written event ends up in the trace.
        RialtoDataSource::Trace(
                [](RialtoDataSource::TraceContext ctx) { ctx.Flush(); });
#endif
#ifndef ENABLE_BACKEND_TRACING
        perfetto::TrackEvent::Flush();
#endif
        // Stop tracing and read the trace data.
        tracingSession->StopBlocking();
        std::vector<char> traceData(tracingSession->ReadTraceBlocking());

        // Write the result into a file.
        // Note: To save memory with longer traces, you can tell Perfetto to write
        // directly into a file by passing a file descriptor into Setup() above.
        const char* filename = "/tmp/rialto_scheduling.pftrace";
        m_traceFile.open(filename, std::ios::out | std::ios::binary);
        m_traceFile.write(&traceData[0], static_cast<std::streamsize>(traceData.size()));
        m_traceFile.close();
        PERFETTO_LOG(
                "Trace written in %s file. To read this trace in "
                "text form, run `./tools/traceconv text %s`",
                filename, filename);

        RIALTO_SERVER_LOG_DEBUG("Tracing session %p stopped", tracingSession.get());
        PERFETTO_LOG("Tracing session %p stopped", tracingSession.get());
    }


} //firebolt::rialto::server


PERFETTO_DECLARE_DATA_SOURCE_STATIC_MEMBERS(firebolt::rialto::server::RialtoDataSource);
PERFETTO_DEFINE_DATA_SOURCE_STATIC_MEMBERS(firebolt::rialto::server::RialtoDataSource);

std::shared_ptr<firebolt::rialto::server::RialtoPerfetto> initializePerfetto()
{
    PERFETTO_LOG(" InititalizePerfetto called \n");
    std::shared_ptr<firebolt::rialto::server::RialtoPerfetto> rialtoPerfetto = std::make_shared<firebolt::rialto::server::RialtoPerfetto>();
    rialtoPerfetto->InitializePerfetto();
#ifdef ENABLE_BACKEND_TRACING
    firebolt::rialto::server::Observer observer;
    observer.WaitForTracingStart();
#else
    (void) rialtoPerfetto->StartTracing();
#endif

    // Give a custom name for the traced process.
    perfetto::ProcessTrack process_track = perfetto::ProcessTrack::Current();
    perfetto::protos::gen::TrackDescriptor desc = process_track.Serialize();
    desc.mutable_process()->set_process_name("RialtoServerProcess");
    perfetto::TrackEvent::SetTrackDescriptor(process_track, desc);

#if 0
    // Write an event using our custom data source.
    firebolt::rialto::server::RialtoDataSource::Trace([](firebolt::rialto::server::RialtoDataSource::TraceContext ctx) 
            {
            auto packet = ctx.NewTracePacket();
            packet->set_timestamp(42);
            packet->set_for_testing()->set_str("Hello world!");
            });
#endif
    return rialtoPerfetto;
}

void setTraceEvent(std::string message)
{
    PERFETTO_LOG(" Writing event %s \n", message.c_str());
    // Write an event using our custom data source.
    firebolt::rialto::server::RialtoDataSource::Trace([message](firebolt::rialto::server::RialtoDataSource::TraceContext ctx) 
            {
            auto packet = ctx.NewTracePacket();
            packet->set_timestamp(42);
            packet->set_for_testing()->set_str(message);
            });

}
