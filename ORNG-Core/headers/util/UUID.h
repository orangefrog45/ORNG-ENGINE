#pragma once
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <bitsery/adapter/buffer.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace ORNG {
	template<std::integral T>
	requires(std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>)
	class UUID {
	public:
		friend class SceneSerializer;
		UUID() {
			static std::random_device s_random_device;
			static std::mt19937 s_engine(s_random_device());
			static std::uniform_int_distribution<T> s_uniform_distribution;

			m_uuid = s_uniform_distribution(s_engine);
		}

		explicit UUID(T uuid) : m_uuid(uuid) {}
		explicit UUID(const UUID& other) { UUID(other.m_uuid); }
		UUID& operator=(const UUID& other) { this->m_uuid = other.m_uuid; return *this; }

		explicit operator T() const { return m_uuid; }
		T operator() () const { return m_uuid; }

		template<typename S>
		void serialize(S& s) {
			s.value8b(m_uuid);
		}

	private:
		T m_uuid;
	};

}

namespace std {

	template<>
	struct hash<ORNG::UUID<uint64_t>> {
		std::size_t operator()(const ORNG::UUID<uint64_t>& uuid) const {
			return hash<uint64_t>()(static_cast<uint64_t>(uuid));
		}
	};

	template<>
	struct hash<ORNG::UUID<uint32_t>> {
		std::size_t operator()(const ORNG::UUID<uint32_t>& uuid) const {
			return hash<uint32_t>()(static_cast<uint32_t>(uuid));
		}
	};
}
