#include "util/util.h"
namespace ORNG {
	void HandledFileSystemCopy(const std::string& file_to_copy, const std::string& copy_location, bool recursive) {
		try {
			if (recursive)
				std::filesystem::copy(file_to_copy, copy_location, std::filesystem::copy_options::recursive);
			else
				std::filesystem::copy_file(file_to_copy, copy_location);
		}
		catch (const std::exception& e) {
			ORNG_CORE_ERROR("std::filesystem::copy_file failed : '{0}'", e.what());
		}
	}

	void HandledFileDelete(const std::string& filepath) {
		try {
			std::filesystem::remove(filepath);
		}
		catch (std::exception& e) {
			ORNG_CORE_ERROR("std::filesystem::remove error, '{0}'", e.what());
		}
	}

	bool PathEqualTo(const std::string& path1, const std::string& path2) {
		if (path1.empty() || path2.empty())
			return false;

		bool ret = false;
		try {
			ret = std::filesystem::equivalent(path1, path2);
		}
		catch (std::exception e) {
			ORNG_CORE_ERROR("PathEqualTo err: '{0}' with '{1}' : '{2}'", e.what(), path1, path2);
		}
		return ret;
	}

	std::string GetFileLastWriteTime(const std::string& filepath) {
		std::string formatted = "";

		try {
			std::filesystem::file_time_type file_time = std::filesystem::last_write_time(filepath);
			auto sys_time = std::chrono::clock_cast<std::chrono::system_clock>(file_time);
			std::time_t time = std::chrono::system_clock::to_time_t(sys_time);

			char buffer[80];
			std::tm* time_info = std::localtime(&time);
			std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);
			formatted = buffer;
		}
		catch (std::exception& e) {
			ORNG_CORE_ERROR("std::filesystem::last_write_time error, '{0}'", e.what());
		}

		return formatted;
	}
}