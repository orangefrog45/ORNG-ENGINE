#include "pch/pch.h"
#include "util/UUID.h"

namespace ORNG {
	static std::random_device s_random_device;
	static std::mt19937 s_engine(s_random_device());
	static std::uniform_int_distribution<uint64_t> s_uniform_distribution;

	UUID::UUID() : m_uuid(s_uniform_distribution(s_engine)) {

	}
}