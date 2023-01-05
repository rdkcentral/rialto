
#include "perfetto.h"
#include <fstream>
#include <thread>
#include <memory>
#include <string>
namespace firebolt::rialto::server
{
    class RialtoPerfetto
    {
        public:

            RialtoPerfetto();
            virtual ~RialtoPerfetto();
            void InitializePerfetto();
            //std::unique_ptr<perfetto::TracingSession> StartTracing();
            void StartTracing();
            void StopTracing(std::unique_ptr<perfetto::TracingSession> tracingSession);
            //void SetTracingSession(std::unique_ptr<perfetto::TracingSession> tracingSession);
        private:
            std::unique_ptr<perfetto::TracingSession> m_tracingSession;
            std::ofstream m_traceFile; 
    };
} // firebolt::rialto::server

std::shared_ptr<firebolt::rialto::server::RialtoPerfetto> initializePerfetto();
void setTraceEvent(std::string message);
