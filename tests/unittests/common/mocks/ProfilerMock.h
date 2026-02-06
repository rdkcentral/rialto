#include "IProfiler.h"
#include <gmock/gmock.h>
#include <optional>
#include <string>

namespace firebolt::rialto::common
{
class ProfilerMock : public IProfiler
{
public:
    ProfilerMock() = default;
    virtual ~ProfilerMock() = default;

    MOCK_METHOD(bool, enabled, (), (const, noexcept, override));

    MOCK_METHOD(std::optional<RecordId>, record, (std::string stage), (override));
    MOCK_METHOD(std::optional<RecordId>, record, (std::string stage, std::string info), (override));

    MOCK_METHOD(std::optional<RecordId>, find, (std::string stage), (override));
    MOCK_METHOD(std::optional<RecordId>, find, (std::string stage, std::string info), (override));

    MOCK_METHOD(void, log, (RecordId id), (override));

    MOCK_METHOD(bool, dump, (const std::string &path), (const, override));
};
} // namespace firebolt::rialto::common
