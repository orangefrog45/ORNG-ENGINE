#include "util/util.h"
namespace ORNG {
	bool FileCopy(const std::string& file_to_copy, const std::string& copy_location, bool recursive) {
		try {
			if (recursive)
				std::filesystem::copy(file_to_copy, copy_location, std::filesystem::copy_options::recursive);
			else
				std::filesystem::copy_file(file_to_copy, copy_location);

			return true;
		}
		catch (const std::exception& e) {
			ORNG_CORE_ERROR("std::filesystem::copy_file failed : '{0}'", e.what());
			return false;
		}
	}

	void FileDelete(const std::string& filepath) {
		try {
			std::filesystem::remove(filepath);
		}
		catch (std::exception& e) {
			ORNG_CORE_ERROR("std::filesystem::remove error, '{0}'", e.what());
		}
	}

	bool FileExists(const std::string& filepath) {
		try {
			return std::filesystem::exists(filepath);
		}
		catch (std::exception& e) {
			ORNG_CORE_ERROR("std::filesystem::remove error, '{0}'", e.what());
			return false;
		}
	}


	std::string GetFileDirectory(const std::string& filepath) {
		size_t forward_pos = filepath.rfind("/");
		size_t back_pos = filepath.rfind("\\");

		if (back_pos == std::string::npos) {
			if (forward_pos == std::string::npos) {
				ORNG_CORE_ERROR("GetFileDirectory with filepath '{0}' failed, no directory found", filepath);
				return filepath;
			}
			return filepath.substr(0, forward_pos);
		}
		else if (forward_pos == std::string::npos) {
			return filepath.substr(0, back_pos);
		}


		if (forward_pos > back_pos)
			return filepath.substr(0, forward_pos);
		else
			return filepath.substr(0, back_pos);
	}

	bool PathEqualTo(const std::string& path1, const std::string& path2) {
		if (path1.empty() || path2.empty())
			return false;
		std::string c1 = path1;
		std::string c2 = path2;

		std::ranges::remove_if(c1, [](char c) {return c == '\\' || c == '/'; });
		std::ranges::remove_if(c2, [](char c) {return c == '\\' || c == '/'; });

		auto cur = std::filesystem::current_path().string();
		std::ranges::remove_if(cur, [](char c) {return c == '\\' || c == '/'; });

		return c1 == c2 || cur + c1 == c2 || c1 == cur + c2;
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

	void PushMatrixIntoArray(const glm::mat4& m, float* array_ptr) {
		*array_ptr++ = m[0][0];
		*array_ptr++ = m[0][1];
		*array_ptr++ = m[0][2];
		*array_ptr++ = m[0][3];

		*array_ptr++ = m[1][0];
		*array_ptr++ = m[1][1];
		*array_ptr++ = m[1][2];
		*array_ptr++ = m[1][3];

		*array_ptr++ = m[2][0];
		*array_ptr++ = m[2][1];
		*array_ptr++ = m[2][2];
		*array_ptr++ = m[2][3];

		*array_ptr++ = m[3][0];
		*array_ptr++ = m[3][1];
		*array_ptr++ = m[3][2];
		*array_ptr++ = m[3][3];
	}
}