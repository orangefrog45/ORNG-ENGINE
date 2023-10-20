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

	void HandledFileSystemCopy(const std::string& file_to_copy, const std::string& copy_location, bool recursive = false);
	void HandledFileDelete(const std::string& filepath);
	bool PathEqualTo(const std::string& path1, const std::string& path2);

}

