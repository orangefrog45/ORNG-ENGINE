#pragma once
#include "Log.h"


#define BREAKPOINT ORNG_CORE_ERROR("Breakpoint hit at '{0}', '{1}'", FUNC_NAME, __LINE__); ORNG::Log::Flush(); __debugbreak()
#define ASSERT(x) if (!(x)) { ORNG_CORE_ERROR("Assertion failed '{0}' at '{1}', '{2}'", #x, FUNC_NAME, __LINE__); ORNG::Log::Flush(); __debugbreak();}

#ifdef NDEBUG
#define DEBUG_ASSERT(x)
#else
#define DEBUG_ASSERT(x) ASSERT(x)
#endif

#define VEC_PUSH_VEC3(vector, vec3) vector.push_back(vec3.x); vector.push_back(vec3.y); vector.push_back(vec3.z)
#define VEC_PUSH_VEC2(vector, vec2) vector.push_back(vec2.x); vector.push_back(vec2.y)
#define ORNG_MAX_FILEPATH_SIZE 500
#define ORNG_MAX_NAME_SIZE 500

#define CONCAT(x, y) CONCAT_IMPL(x, y)
#define CONCAT_IMPL(x, y) x##y

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))


#ifdef ORNG_ENABLE_TRACY_PROFILE
#include "Tracy.hpp"
#include "TracyOpenGL.hpp"
#define ORNG_TRACY_PROFILE ZoneScoped
#define ORNG_TRACY_PROFILEN(x) ZoneScopedN(x)
#else
#define ORNG_TRACY_PROFILE
#define ORNG_TRACY_PROFILEN
#endif


namespace ORNG {



	namespace detail {
		template<typename T>
		concept HasConvertToBytesOverride = requires(T t) {
			t.ConvertSelfToBytes(std::declval<std::byte*&>());
		};

		template<typename T>
		void ConvertToBytes(std::byte*& byte, T&& val) {
			if constexpr (HasConvertToBytesOverride<T>) {
				val.ConvertSelfToBytes(byte);
			}
			else {
				std::memcpy(byte, &val, sizeof(T));
				byte += sizeof(T);
			}

		}

	}

	template<typename T>
	concept Vec2Type = requires(T vec) {
		{ vec.x };
		{ vec.y };
	};

	template<typename T>
	concept Vec3Type = Vec2Type<T> && requires(T vec) {
		{ vec.z };
	};

	template<typename T>
	concept Vec4Type = Vec3Type<T> && requires(T vec) {
		{ vec.w };
	};

	template<Vec4Type DestType, Vec4Type SourceType>
	DestType ConvertVec4(SourceType&& vec) {
		return DestType{ vec.x, vec.y, vec.z, vec.w };
	}

	template<Vec3Type DestType, Vec3Type SourceType>
	DestType ConvertVec3(SourceType&& vec) {
		return DestType{ vec.x, vec.y, vec.z };
	}

	template<Vec2Type DestType, Vec2Type SrcType>
	DestType ConvertVec2(SrcType&& vec) {
		return DestType{ vec.x, vec.y };
	}


	/*
	FILE UTILS
	*/
	bool FileCopy(const std::string& file_to_copy, const std::string& copy_location, bool recursive = false);

	void FileDelete(const std::string& filepath);

	bool PathEqualTo(const std::string& path1, const std::string& path2);

	bool FileExists(const std::string& filepath);

	bool TryFileDelete(const std::string& filepath);

	bool TryDirectoryDelete(const std::string& filepath);

	std::string GetFileDirectory(const std::string& filepath);

	std::string GetFileLastWriteTime(const std::string& filepath);

	std::string GetApplicationExecutableDirectory();

	void Create_Directory(const std::string& path);

	bool IsEntryAFile(const std::filesystem::directory_entry& entry);


	/*
	MISC UTILS
	*/

	template<typename T>
	bool VectorContains(const std::vector<T>& vec, const T& value) {
		if (vec.empty())
			return false;

		return std::ranges::find(vec, value) != vec.end();
	}

	// Replaces all instances of "text_to_replace" with "replacement_text"
	// Modifies "input" string directly, returns number of replacement operations
	unsigned StringReplace(std::string& input, const std::string& text_to_replace, const std::string& replacement_text);

	// Copies raw bytes of types into array of std::byte, this will increment the ptr provided by sizeof(T)
	template<typename... Args>
	void ConvertToBytes(std::byte*& byte, Args&&... args) {
		(detail::ConvertToBytes(byte, std::forward<Args>(args)), ...);
	}

	// E.g converts '9' to integer value 9, not ascii code
	inline int CharToInt(char c) {
		return c - '0';
	}

	template<typename T>
	concept HasPushBack = requires(T t) {
		t.push_back(std::declval<typename T::value_type>());
	};

	template<typename T>
	concept HasBegin = requires(T t) {
		t.begin();
	};

	template<typename T>
	concept HasEnd = requires(T t) {
		t.end();
	};

	template<typename T>
	concept HasEraseMethod = requires(T t) {
		t.erase(std::declval<typename T::iterator>());
	};


	template<HasPushBack T, typename... Args>
		requires((std::is_convertible_v<Args, typename T::value_type>, ...))
	void PushBackMultiple(T& container, Args&&... args) {
		(container.push_back(std::forward<Args>(args)), ...);
	};

	template<typename T>
	size_t VectorFindIndex(const std::vector<T>& v, const T& search_for) {
		return std::distance(v.begin(), std::ranges::find(v, search_for));
	}

	template<typename T>
	T* VectorFind(const std::vector<T>& v, const T& search_for) {
		if (auto it = std::ranges::find(v, search_for); it != v.end())
			return &(*it);
		else
			return nullptr;
	}

	template<typename Container>
	requires(HasBegin<Container>, HasEnd<Container>, HasEraseMethod<Container>)
	void RemoveIfContains(Container container, typename Container::value_type find) {
		auto it = std::ranges::find(container, find);
		if (it != container.end()) {
			container.erase(it);
		}
	}

}
