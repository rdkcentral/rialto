#include "IProfiler.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::common
{
class ProfilerFactoryMock : public IProfilerFactory
{
public:
    ProfilerFactoryMock() = default;
    virtual ~ProfilerFactoryMock() = default;

    MOCK_METHOD(std::unique_ptr<IProfiler>, createProfiler, (std::string moduleName), (const, override));
};
} // namespace firebolt::rialto::common
