#ifndef FIREBOLT_RIALTO_COMMON_I_PROFILER_H_
#define FIREBOLT_RIALTO_COMMON_I_PROFILER_H_

#include <functional>
#include <memory>
#include <string>
#include <optional>

namespace firebolt::rialto::common
{
class IProfiler;

/**
 * @brief IProfiler factory class, returns a concrete implementation of IProfiler
 */
class IProfilerFactory
{
public:
    IProfilerFactory() = default;
    virtual ~IProfilerFactory() = default;

    /**
     * @brief Creates a IProfilerFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IProfilerFactory> createFactory();

    /**
     * @brief Creates an IProfiler object.
     *
     * @param[in] moduleName    : The name of the module
     *
     * @retval the new profiler instance or null on error.
     */
    virtual std::unique_ptr<IProfiler> createProfiler(std::string moduleName) const = 0;
};

class IProfiler
{
public:
    using RecordId = std::uint64_t;

    virtual ~IProfiler() = default;

    IProfiler(const IProfiler &) = delete;
    IProfiler &operator=(const IProfiler &) = delete;
    IProfiler(IProfiler &&) = delete;
    IProfiler &operator=(IProfiler &&) = delete;

    /**
     * @brief Checks if profiler is enabled.
     *
     * @retval true if profiler is enabled, false otherwise.
     */
    virtual bool enabled() const noexcept = 0;

    /**
     * @brief Creates a record for given stage.
     *
     * @param[in] stage : Stage name used for record creation
     *
     * @retval Record identificator for created record or std::nullopt.
     */
    virtual std::optional<RecordId> record(std::string stage) = 0;

    /**
     * @brief Creates a record for given stage and info.
     *
     * @param[in] stage : Stage name used for record creation
     * @param[in] info  : Additional information used for record creation
     *
     * @retval Record identificator for created record or std::nullopt.
     */
    virtual std::optional<RecordId> record(std::string stage, std::string info) = 0;

    /**
     * @brief Finds an existing record for given stage.
     *
     * @param[in] stage : Stage name of the record to be found
     *
     * @retval Record identificator for found record or std::nullopt.
     */
    virtual std::optional<RecordId> find(std::string stage) = 0;

    /**
     * @brief Finds an existing record for given stage and info.
     *
     * @param[in] stage : Stage name of the record to be found
     * @param[in] info  : Additional information of the record to be found
     *
     * @retval Record identificator for found record or std::nullopt.
     */
    virtual std::optional<RecordId> find(std::string stage, std::string info) = 0;

    /**
     * @brief Logs a record for given identificator.
     *
     * @param[in] id : Record identificator
     */
    virtual void log(RecordId id) = 0;

    /**
     * @brief Dumps all records into file.
     *
     * @param[in] path : Full path to the directory where the file will be stored
     *
     * @retval true if file is created and records are dumped, false otherwise.
     */
    virtual bool dump(const std::string& path) const = 0;

protected:
    IProfiler() = default;
};

} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_I_PROFILER_H_
