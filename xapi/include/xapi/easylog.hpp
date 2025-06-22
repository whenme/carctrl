// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <iostream>

#include <cstdlib>

extern char *__progname;
namespace {
struct easylog_options {
  spdlog::level::level_enum log_level = spdlog::level::debug;
  std::string app_log_name = __progname;
  std::string log_dir = "/var/log/";
  bool always_flush = false;
  int flush_interval = 5;
  int max_size = 5 * 1024 * 1024;
  int max_files = 5;
};

struct source_location {
  constexpr source_location(const char *file_name = __builtin_FILE(),
                            const char *function_name = __builtin_FUNCTION(),
                            unsigned int line = __builtin_LINE()) noexcept
      : file_name_(file_name), function_name_(function_name), line_(line) {}
  constexpr const char *file_name() const noexcept { return file_name_; }
  constexpr const char *function_name() const noexcept {
    return function_name_;
  }
  constexpr unsigned int line() const noexcept { return line_; }

 private:
  const char *file_name_;
  const char *function_name_;
  const unsigned int line_;
};


[[nodiscard]] inline constexpr auto get_log_source_location(
    const source_location &location)
{
  return spdlog::source_loc{location.file_name(),
                            static_cast<std::int32_t>(location.line()),
                            location.function_name()};
}

inline bool always_flush_ = false;

inline std::vector<spdlog::sink_ptr> get_sinks(const easylog_options &options)
{
    std::vector<spdlog::sink_ptr> sinks;

    std::string filename = options.log_dir + options.app_log_name + ".log";
    //auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    //    filename, options.max_size, options.max_files);
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(filename, true);
    file_sink->set_level(spdlog::level::trace);
    sinks.push_back(file_sink);

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(options.log_level);
    sinks.push_back(console_sink);

    return sinks;
}

template <typename... Args>
inline void log(const std::string &name, spdlog::level::level_enum level, source_location location,
                fmt::format_string<Args...> fmt, Args &&...args)
{
  switch (level) {
    case spdlog::level::trace:
    case spdlog::level::debug:
    case spdlog::level::info:
    case spdlog::level::warn:
    case spdlog::level::err:
    case spdlog::level::critical:
	  spdlog::get(name)->log(get_log_source_location(location),
                             level, fmt, std::forward<Args>(args)...);
      break;
    default:
      break;
  }

  if (always_flush_) {
    spdlog::default_logger()->flush();
  }

  //critical or off level
  if (level > spdlog::level::err) {
    spdlog::default_logger()->flush();
    if (level == spdlog::level::off) {
      std::exit(EXIT_FAILURE);
    }
  }
}
}  // namespace


inline void init_log(easylog_options options = {}, bool over_write = true)
{
  static bool has_init_ = false;
  if (has_init_ && !over_write) {
    return;
  }

  auto sinks = get_sinks(options);
  auto ctrlLogger = std::make_shared<spdlog::logger>("ctrl", sinks.begin(), sinks.end());
  auto apiLogger = std::make_shared<spdlog::logger>("api", sinks.begin(), sinks.end());

  // log pattern: [time] [module name] color begin [log level] [file:line] content / color end
  spdlog::set_pattern("[%H:%M:%S.%e] [%n] %^[%l] [%s:%#] %v%$");
  spdlog::initialize_logger(ctrlLogger);

  ctrlLogger->set_level(options.log_level);
  apiLogger->set_level(options.log_level);

  spdlog::set_level(options.log_level);
  spdlog::set_default_logger(ctrlLogger);
  spdlog::register_logger(apiLogger);

  always_flush_ = options.always_flush;
  if (!options.always_flush && options.flush_interval > 0) {
    spdlog::flush_every(std::chrono::seconds(options.flush_interval));
  }

  has_init_ = true;
}

namespace ctrllog {
template <typename... Args>
struct trace {
  constexpr trace(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {})
  {
    log("ctrl", spdlog::level::trace, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
trace(fmt::format_string<Args...> fmt, Args &&...args) -> trace<Args...>;

template <typename... Args>
struct debug {
  constexpr debug(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {})
  {
    log("ctrl", spdlog::level::debug, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
debug(fmt::format_string<Args...> fmt, Args &&...args) -> debug<Args...>;

template <typename... Args>
struct info {
  constexpr info(fmt::format_string<Args...> fmt, Args &&...args,
                 source_location location = {})
  {
    log("ctrl", spdlog::level::info, location, fmt, std::forward<Args>(args)...);
  }

  constexpr info(source_location location, fmt::format_string<Args...> fmt,
                 Args &&...args)
  {
    log("ctrl", spdlog::level::info, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
info(fmt::format_string<Args...> fmt, Args &&...args) -> info<Args...>;

template <typename... Args>
info(source_location location, fmt::format_string<Args...> fmt, Args &&...args)
    -> info<Args...>;

template <typename... Args>
struct warn {
  constexpr warn(fmt::format_string<Args...> fmt, Args &&...args,
                 source_location location = {})
  {
    log("ctrl", spdlog::level::warn, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
warn(fmt::format_string<Args...> fmt, Args &&...args) -> warn<Args...>;

template <typename... Args>
struct error {
  constexpr error(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {})
  {
    log("ctrl", spdlog::level::err, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
error(fmt::format_string<Args...> fmt, Args &&...args) -> error<Args...>;

template <typename... Args>
struct critical {
  constexpr critical(fmt::format_string<Args...> fmt, Args &&...args,
                     source_location location = {})
  {
    log("ctrl", spdlog::level::critical, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
critical(fmt::format_string<Args...> fmt, Args &&...args) -> critical<Args...>;
}  // namespace ctrllog


namespace apilog {
template <typename... Args>
struct trace {
  constexpr trace(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {})
  {
    log("api", spdlog::level::trace, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
trace(fmt::format_string<Args...> fmt, Args &&...args) -> trace<Args...>;

template <typename... Args>
struct debug {
  constexpr debug(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {})
  {
    log("api", spdlog::level::debug, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
debug(fmt::format_string<Args...> fmt, Args &&...args) -> debug<Args...>;

template <typename... Args>
struct info {
  constexpr info(fmt::format_string<Args...> fmt, Args &&...args,
                 source_location location = {})
  {
    log("api", spdlog::level::info, location, fmt, std::forward<Args>(args)...);
  }

  constexpr info(source_location location, fmt::format_string<Args...> fmt,
                 Args &&...args)
  {
    log("api", spdlog::level::info, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
info(fmt::format_string<Args...> fmt, Args &&...args) -> info<Args...>;

template <typename... Args>
info(source_location location, fmt::format_string<Args...> fmt, Args &&...args)
    -> info<Args...>;

template <typename... Args>
struct warn {
  constexpr warn(fmt::format_string<Args...> fmt, Args &&...args,
                 source_location location = {})
  {
    log("api", spdlog::level::warn, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
warn(fmt::format_string<Args...> fmt, Args &&...args) -> warn<Args...>;

template <typename... Args>
struct error {
  constexpr error(fmt::format_string<Args...> fmt, Args &&...args,
                  source_location location = {})
  {
    log("api", spdlog::level::err, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
error(fmt::format_string<Args...> fmt, Args &&...args) -> error<Args...>;

template <typename... Args>
struct critical {
  constexpr critical(fmt::format_string<Args...> fmt, Args &&...args,
                     source_location location = {})
  {
    log("api", spdlog::level::critical, location, fmt, std::forward<Args>(args)...);
  }
};

template <typename... Args>
critical(fmt::format_string<Args...> fmt, Args &&...args) -> critical<Args...>;
}  // namespace apilog
