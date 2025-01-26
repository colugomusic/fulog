#pragma once

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>

#if defined(_WIN32) //////////////////////////////////////////////

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <winerror.h>
#include <stringapiset.h>
#include <shlobj.h>

namespace fu::detail::os {

namespace fs = std::filesystem;

inline
auto shorten(std::wstring_view s) -> std::string {
	if (s.empty()) {
		return {};
	}
	int n = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), NULL, 0, NULL, NULL);
	std::string buf;
	buf.resize(n);
	WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), &buf[0], n, NULL, NULL);
	return buf;
}

inline 
auto get_known_folder(REFKNOWNFOLDERID folder_id, std::string_view error_msg) -> fs::path {
	struct scope_co_free {
		LPWSTR pointer = NULL;
		scope_co_free(LPWSTR pointer) : pointer(pointer) {};
		~scope_co_free() { CoTaskMemFree(pointer); }
	};
	LPWSTR wsz_path = NULL;
	const auto hr = SHGetKnownFolderPath(folder_id, KF_FLAG_CREATE, NULL, &wsz_path);
	scope_co_free mem{wsz_path};
	if (!SUCCEEDED(hr)) {
		throw std::runtime_error(std::string(error_msg));
	}
	return shorten(wsz_path);
}

inline
auto get_pid() -> int {
	return int(GetCurrentProcessId());
}

inline
auto get_data_home_dir() -> fs::path {
	return get_known_folder(FOLDERID_RoamingAppData, "RoamingAppData could not be found");
}

} // fu::detail::os

#else // unix ////////////////////////////////////////////////////

#include <pwd.h>
#include <unistd.h>

namespace fu::detail::os {

inline
auto get_pid() -> int {
	return int(getpid());
}

inline
auto get_home_dir() -> fs::path {
	std::string res;
	int uid = getuid();
	const char* home_env = std::getenv("HOME");
	if (uid != 0 && home_env) {
		res = home_env;
		return res;
	}
	struct passwd* pw = nullptr;
	struct passwd pwd;
	long bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
	if (bufsize < 0) {
		bufsize = 16384;
	}
	std::vector<char> buffer;
	buffer.resize(bufsize);
	int error_code = getpwuid_r(uid, &pwd, buffer.data(), buffer.size(), &pw);
	if (error_code) {
		throw std::runtime_error("Unable to get passwd struct.");
	}
	const char* temp_res = pw->pw_dir;
	if (!temp_res) {
		throw std::runtime_error("User has no home directory");
	}
	res = temp_res;
	return res;
}

#if defined(__APPLE__) ///////////////////////////////////////////

inline
auto get_data_home_dir() -> fs::path {
	return get_home_dir() / "Library" / "Application Support";
}

#else // linux ///////////////////////////////////////////////////

inline
auto get_linux_folder_default(std::string_view env_name, const fs::path& default_relative_path) -> fs::path {
	static constexpr auto ERR_ENV_NO_FWDSLASH =
		"Environment \"{}\" does not start with '/'. XDG specifies that the value must be absolute. The current value is: \"{}\"";
	std::string res;
	const char* temp_res = std::getenv(env_name.data());
	if (temp_res) {
		if (temp_res[0] != '/') {
			throw std::runtime_error(std::format(ERR_ENV_NO_FWDSLASH, env_name, temp_res));
		}
		res = temp_res;
		return res;
	}
	res = get_home_dir() / default_relative_path;
	return res;
}

inline
auto get_data_home_dir() -> fs::path {
	return get_linux_folder_default("XDG_DATA_HOME", ".local/share");
}

#endif ///////////////////////////////////////////////////////////

} // fu::detail::os

#endif ///////////////////////////////////////////////////////////

namespace fu::detail::global {

namespace fs = std::filesystem;

inline fs::path dir;
inline std::mutex mutex;
inline std::ofstream file;
inline std::string app_name;
inline std::string log_name;
inline std::string timestamp_format;

} // fu::detail::global

namespace fu::detail {

namespace fs = std::filesystem;

inline static const     auto DEFAULT_DIR              = os::get_data_home_dir();
inline static constexpr auto DEFAULT_APP_NAME         = "fulog";
inline static const     auto DEFAULT_LOG_NAME         = std::to_string(os::get_pid());
inline static constexpr auto DEFAULT_TIMESTAMP_FORMAT = "{:%Y-%m-%d %H:%M:%S}";

inline
auto resolve_dir() -> fs::path {
	if (global::dir.empty()) { return DEFAULT_DIR; }
	else                     { return global::dir; }
}

inline
auto resolve_app_name() -> std::string_view {
	if (global::app_name.empty()) { return DEFAULT_APP_NAME; }
	else                          { return global::app_name; }
}

inline
auto resolve_log_name() -> std::string {
	if (global::log_name.empty()) { return DEFAULT_LOG_NAME; }
	else                          { return global::log_name; }
}

inline
auto resolve_timestamp_format() -> std::string_view {
	if (global::timestamp_format.empty()) { return DEFAULT_TIMESTAMP_FORMAT; }
	else                                  { return global::timestamp_format; }
}

inline
auto get_file_path() -> fs::path {
	return resolve_dir() / resolve_app_name() / resolve_log_name().append(".log");
}

inline
auto open_file() -> void {
	const auto path = get_file_path();
	if (!fs::exists(path.parent_path())) {
		fs::create_directories(get_file_path().parent_path());
	}
	global::file.open(path, std::ios::app);
	if (!global::file.is_open()) {
		throw std::runtime_error(std::format("Could not open file: {}", path.string()));
	}
}

inline
auto reopen_file_if_it_is_already_open() -> void {
	if (detail::global::file.is_open()) {
		detail::global::file.close();
		open_file();
	}
}

inline
auto open_file_if_it_is_not_already_open() -> void {
	if (!detail::global::file.is_open()) {
		open_file();
	}
}

inline
auto file_time_to_system_time(const fs::file_time_type& file_time) -> std::chrono::system_clock::time_point {
#if __APPLE__
	auto duration_since_epoch = file_time.time_since_epoch();
	auto system_time_duration = std::chrono::duration_cast<std::chrono::system_clock::duration>(duration_since_epoch);
	return std::chrono::system_clock::time_point{system_time_duration};
#else
	return std::chrono::clock_cast<std::chrono::system_clock>(file_time);
#endif
}

inline
auto delete_old_files(const fs::path& dir, std::chrono::system_clock::duration older_than) -> void {
	const auto now = std::chrono::system_clock::now();
	if (!fs::exists(dir)) {
		return;
	}
	for (const auto& entry : fs::directory_iterator(dir)) {
		if (fs::is_regular_file(entry)) {
			const auto last_write_time = fs::last_write_time(entry);
			const auto last_write_time_system = detail::file_time_to_system_time(last_write_time);
			const auto age = now - last_write_time_system;
			if (age > std::chrono::hours{48}) {
				fs::remove(entry);
			}
		}
	}
}

} // fu::detail

namespace fu {

namespace fs = std::filesystem;

inline
auto set_dir(const fs::path& path) -> void {
	auto lock = std::lock_guard{detail::global::mutex};
	if (path == detail::global::dir) {
		return;
	}
	detail::global::dir = path;
	detail::reopen_file_if_it_is_already_open();
}

inline
auto set_application_name(std::string_view name) -> void {
	auto lock = std::lock_guard{detail::global::mutex};
	if (name == detail::global::app_name) {
		return;
	}
	detail::global::app_name = name;
	detail::reopen_file_if_it_is_already_open();
}

inline
auto set_log_name(std::string_view name) -> void {
	auto lock = std::lock_guard{detail::global::mutex};
	if (name == detail::global::log_name) {
		return;
	}
	detail::global::log_name = name;
	detail::reopen_file_if_it_is_already_open();
}

inline
auto set_timestamp_format(std::string_view format) -> void {
	auto lock = std::lock_guard{detail::global::mutex};
	detail::global::timestamp_format = format;
}

inline
auto delete_old_files(std::chrono::system_clock::duration older_than) -> void {
	auto lock = std::lock_guard{detail::global::mutex};
	detail::delete_old_files(detail::resolve_dir() / detail::resolve_app_name(), older_than);
}

inline
auto log(std::string_view msg) -> void {
	auto lock = std::lock_guard{detail::global::mutex};
	auto now = std::chrono::system_clock::now();
	auto now_seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	auto timestamp_fmt = detail::resolve_timestamp_format();
	auto timestamp = std::vformat(timestamp_fmt, std::make_format_args(now_seconds));
	auto line = std::format("{} {}\n", timestamp, msg);
	detail::open_file_if_it_is_not_already_open();
	detail::global::file << line;
	detail::global::file.flush();
}

inline
auto log(std::string_view msg, std::source_location loc) -> void {
	log(std::format("{} [{}:{}]\n", msg, loc.file_name(), loc.line()));
}

inline
auto debug_log(std::string_view msg) -> void {
#if defined(_DEBUG)
	log(msg);
#endif
}

inline
auto debug_log(std::string_view msg, std::source_location loc) -> void {
#if defined(_DEBUG)
	log(msg, loc);
#endif
}

} // fu