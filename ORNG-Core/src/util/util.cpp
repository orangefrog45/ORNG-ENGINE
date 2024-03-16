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

	bool TryFileDelete(const std::string& filepath) {
		if (FileExists(filepath)) {
			FileDelete(filepath);
			return true;
		}

		return false;
	}

	bool TryDirectoryDelete(const std::string& filepath) {
		if (FileExists(filepath)) {
			try {
				std::filesystem::remove_all(filepath);
			}
			catch (std::exception& e) {
				ORNG_CORE_ERROR("std::filesystem::remove_all failed : '{0}'", e.what());
			}
			return true;
		}

		return false;
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

	unsigned StringReplace(std::string& input, const std::string& text_to_replace, const std::string& replacement_text) {
		size_t pos = input.find(text_to_replace, 0);
		unsigned num_replacements = 0;

		while (pos < input.size() && pos != std::string::npos) {
			ORNG_CORE_TRACE(pos);
			input.replace(pos, text_to_replace.size(), replacement_text);
			num_replacements++;

			pos = input.find(text_to_replace, pos + replacement_text.size());
		}

		return num_replacements;
	}


	void Create_Directory(const std::string& path) {
		try {
			if (!std::filesystem::exists(path))
				std::filesystem::create_directory(path);
		}
		catch (std::exception& e) {
			ORNG_CORE_ERROR("std::filesystem::create_directory error: '{0}'", e.what());
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
		c1.erase(std::remove_if(c1.begin(), c1.end(), [](char c) { return c == '\\' || c == '/' || c == '.'; }), c1.end());
		c2.erase(std::remove_if(c2.begin(), c2.end(), [](char c) { return c == '\\' || c == '/' || c == '.'; }), c2.end());

		auto cur = std::filesystem::current_path().string();
		cur.erase(std::remove_if(cur.begin(), cur.end(), [](char c) { return c == '\\' || c == '/' || c == '.'; }), cur.end());

		return c1 == c2 || cur + c1 == c2 || c1 == cur + c2;
	}

	bool IsEntryAFile(const std::filesystem::directory_entry& entry) {
		try {
			return std::filesystem::is_regular_file(entry);
		}
		catch (std::exception& e) {
			ORNG_CORE_ERROR("std::filesystem::is_regular_file err with path '{0}', '{1}'", entry.path().string(), e.what());
		}
	}

	std::string GetApplicationExecutableDirectory() {
		char buffer[MAX_PATH];
		GetModuleFileNameA(NULL, buffer, MAX_PATH);

		std::string directory = buffer;
		directory = directory.substr(0, directory.find_last_of("\\"));

		return directory;
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