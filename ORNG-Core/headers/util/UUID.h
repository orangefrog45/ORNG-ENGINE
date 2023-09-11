#pragma once
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/string.h>

namespace ORNG {

	class UUID {
	public:
		friend class SceneSerializer;
		UUID();
		explicit UUID(uint64_t uuid) : m_uuid(uuid) {}
		UUID(const UUID&) { UUID(); }

		explicit operator uint64_t() const { return m_uuid; }
		uint64_t operator() () const { return m_uuid; };

		template<typename S>
		void serialize(S& s) {
			s.value8b(m_uuid);
		}
	private:
		uint64_t m_uuid;
	};

}

namespace std {

	template<>
	struct hash<ORNG::UUID> {
		std::size_t operator()(const ORNG::UUID& uuid) const {
			return hash<uint64_t>()((uint64_t)uuid);
		}
	};
}