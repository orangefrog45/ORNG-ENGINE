#pragma once
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>

namespace ORNG {
	template<std::integral T>
	requires(std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>)
	class UUID {
	public:
		friend class SceneSerializer;
		UUID();
		explicit UUID(T uuid) : m_uuid(uuid) {
		}
		UUID(const UUID&) { UUID(); }

		explicit operator T() const { return m_uuid; }
		T operator() () const { return m_uuid; };

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
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};

	template<>
	struct hash<ORNG::UUID<uint32_t>> {
		std::size_t operator()(const ORNG::UUID<uint32_t>& uuid) const {
			return hash<uint32_t>()((uint32_t)uuid);
		}
	};
}