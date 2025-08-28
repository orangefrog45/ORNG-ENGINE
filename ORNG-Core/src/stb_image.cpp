#include "pch/pch.h"
#pragma warning(disable:4996)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4244 ) // 'argument': conversion from 'int' to 'short', possible loss of data
#endif

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning( pop )
#endif

