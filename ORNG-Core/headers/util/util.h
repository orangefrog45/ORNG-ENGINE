#pragma once
#include "Log.h"
namespace ORNG {
#define BREAKPOINT ORNG_CORE_ERROR("Breakpoint hit at '{0}', '{1}'", FUNC_NAME, __LINE__); ORNG::Log::Flush(); __debugbreak()
#define ASSERT(x) if (!(x)) { ORNG_CORE_ERROR("Assertion failed '{0}' at '{1}', '{2}'", #x, FUNC_NAME, __LINE__); ORNG::Log::Flush(); __debugbreak();}
#define VEC_PUSH_VEC3(vector, vec3) vector.push_back(vec3.x); vector.push_back(vec3.y); vector.push_back(vec3.z)
#define VEC_PUSH_VEC2(vector, vec2) vector.push_back(vec2.x); vector.push_back(vec2.y)
#define ORNG_MAX_FILEPATH_SIZE 500
#define ORNG_MAX_NAME_SIZE 500

#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define CONCAT_IMPL(x, y) x##y

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))

	constexpr bool SHADER_DEBUG_MODE = false; // true = CreateUniform throws error if uniform doesn't exist

	template<typename T>
	bool VectorContains(const std::vector<T>& vec, const T& value) {
		if (vec.empty())
			return false;

		return std::ranges::find(vec, value) != vec.end();
	}

	bool FileCopy(const std::string& file_to_copy, const std::string& copy_location, bool recursive = false);
	void FileDelete(const std::string& filepath);
	bool PathEqualTo(const std::string& path1, const std::string& path2);
	bool FileExists(const std::string& filepath);

	// Replaces all instances of "text_to_replace" with "replacement_text"
	// Modifies "input" string directly, returns number of replacement operations
	unsigned StringReplace(std::string& input, const std::string& text_to_replace, const std::string& replacement_text);

	bool TryFileDelete(const std::string& filepath);

	bool TryDirectoryDelete(const std::string& filepath);

	std::string GetFileDirectory(const std::string& filepath);
	std::string GetFileLastWriteTime(const std::string& filepath);

	std::string GetApplicationExecutableDirectory();

	void Create_Directory(const std::string& path);

	bool IsEntryAFile(const std::filesystem::directory_entry& entry);

	void PushMatrixIntoArray(const glm::mat4& m, float* array_ptr);

	// Copies raw bytes of matrix into array of std::byte, this will increment the ptr provided by sizeof(mat4)
	void PushMatrixIntoArrayBytes(const glm::mat4& m, std::byte*& array_ptr);

	template <typename T>
	// Copies raw bytes of type into array of std::byte, this will increment the ptr provided by sizeof(T)
	void ConvertToBytes(const T& value, std::byte*& bytes) {
		std::memcpy(bytes, &value, sizeof(T));
		bytes += sizeof(T);
	}
}
