#pragma once
#define WIN32_LEAN_AND_MEAN
#ifndef _MSC_VER
#define FUNC_NAME __PRETTY_FUNCTION__
#endif
//#define ORNG_ENABLE_TRACY_PROFILE

#define NO_INLINE __declspec(noinline)

#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <locale>
#include <codecvt>

#include <GL/glew.h>
#include <lml/core.h>
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
#include <iostream>

#include <sstream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>
#include <unordered_set>
#include <string>
#include <array>
#include <variant>
#include <deque>
#include <span>
