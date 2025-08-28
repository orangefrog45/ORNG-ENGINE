#ifndef COMPONENT_H
#define COMPONENT_H
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include "entt/EnttSingleInclude.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace ORNG {
	class SceneEntity;

	class Component {
	public:
		friend class Scene;
		explicit Component(SceneEntity* p_entity);
		Component(const Component&) = default;
		Component& operator=(const Component&) = default;
		virtual ~Component() = default;

		[[nodiscard]] uint64_t GetEntityUUID() const;
		[[nodiscard]] entt::entity GetEnttHandle() const;
		[[nodiscard]] uint64_t GetStaticSceneUUID() const;
		[[nodiscard]] SceneEntity* GetEntity() { return mp_entity; }
		[[nodiscard]] std::string GetEntityName() const;
	private:
		SceneEntity* mp_entity = nullptr;
	};

	struct RelationshipComponent : public Component {
		RelationshipComponent(SceneEntity* p_entity) : Component(p_entity) {}
		int num_children = 0;
		entt::entity first{ entt::null };
		entt::entity prev{ entt::null };
		entt::entity next{ entt::null };
		entt::entity parent{ entt::null };
	};
}
#endif
