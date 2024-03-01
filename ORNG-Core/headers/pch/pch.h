#pragma once
#define GLEW_STATIC
#define FASTNOISE_STATIC_LIB
#ifdef _MSC_VER
#define FUNC_NAME __FUNCTION__ // or __FUNCSIG__
#define WIN32_LEAN_AND_MEAN
#else
#define FUNC_NAME __PRETTY_FUNCTION__
#endif
#include <Windows.h>
#include <commdlg.h>
#include <locale>
#include <codecvt>

#include <GL/glew.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtx/transform.hpp>
#include <glm/glm/gtx/matrix_major_storage.hpp>
#include <glm/glm/gtc/quaternion.hpp>
#include <stb/stb_image.h>

#include <any>
#include <cstdint>
#include <random>
#include <memory>
#include <filesystem>
#include <concepts>
#include <functional>
#include <type_traits>
#include <future>
#include <chrono>

#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <string>
#include <array>
#include <variant>
#include <deque>
