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

	bool PathEqualTo(const std::string& path1, const std::string& path2) {
		if (path1.empty() || path2.empty())
			return false;

		bool ret = false;
		try {
			ret = std::filesystem::equivalent(path1, path2);
		}
		catch (std::exception e) {
			ORNG_CORE_ERROR("PathEqualTo err: '{0}'", e.what());
		}
		return ret;
	}
}