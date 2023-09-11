#pragma once
#include "Log.h"
namespace ORNG {

#define BREAKPOINT __debugbreak()
#define ASSERT(x) if (!(x)) __debugbreak()
#define VEC_PUSH_VEC3(vector, vec3) vector.push_back(vec3.x); vector.push_back(vec3.y); vector.push_back(vec3.z)
#define VEC_PUSH_VEC2(vector, vec2) vector.push_back(vec2.x); vector.push_back(vec2.y)
#define ORNG_MAX_FILEPATH_SIZE 500
#define ORNG_MAX_NAME_SIZE 500

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

	constexpr bool SHADER_DEBUG_MODE = false; // true = CreateUniform throws error if uniform doesn't exist

	template<typename T>
	bool VectorContains(const std::vector<T>& vec, const T& value) {
		if (vec.empty())
			return false;

		return std::ranges::find(vec, value) != vec.end();
	}

	static void HandledFileSystemCopy(const std::string& file_to_copy, const std::string& copy_location, bool recursive = false) {
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


}

