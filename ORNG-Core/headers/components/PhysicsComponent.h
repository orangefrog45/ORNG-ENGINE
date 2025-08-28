#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H

#include <Jolt/Jolt.h>

#include "Component.h"
#include "Jolt/Physics/Body/Body.h"
#include "util/UUID.h"

namespace ORNG {
	class PhysicsSystem;
	class TransformComponent;

	class PhysicsComponent final : public Component {
		friend class EditorLayer;
		friend class SceneSerializer;
		friend class FixedJointComponent;
		friend class PhysicsSystem;
	public:
		explicit PhysicsComponent(SceneEntity* p_entity) : Component(p_entity) {}
		PhysicsComponent& operator=(const PhysicsComponent&) = default;
		PhysicsComponent(const PhysicsComponent&) = default;
		~PhysicsComponent() override = default;

		enum GeometryType {
			BOX = 0,
			SPHERE = 1,
			TRIANGLE_MESH = 2,
		};
		enum RigidBodyType {
			STATIC = 0,
			DYNAMIC = 1,
		};

		PhysicsComponent(SceneEntity* p_entity, bool is_trigger, GeometryType geom_type, RigidBodyType body_type) : Component(p_entity),
			m_geometry_type(geom_type), m_body_type(body_type), m_is_trigger(is_trigger) {}

		void UpdateGeometry(GeometryType type);
		void SetBodyType(RigidBodyType type);

		void SetTrigger(bool is_trigger);
		bool IsTrigger() const { return m_is_trigger; }

		JPH::BodyID body_id{};
	private:
		void SendUpdateEvent();

		GeometryType m_geometry_type = BOX;
		RigidBodyType m_body_type = STATIC;
		bool m_is_trigger = false;
	};

	struct CharacterControllerComponent : public Component {
	public:
		friend class PhysicsSystem;
		explicit CharacterControllerComponent(SceneEntity* p_entity) : Component(p_entity) {}
		CharacterControllerComponent& operator=(const CharacterControllerComponent&) = default;
		CharacterControllerComponent(const CharacterControllerComponent&) = default;
		~CharacterControllerComponent() override = default;

		bool moved_during_frame = false;
	};

	enum JointEventType {
		CONNECT,
		BREAK
	};

	struct JointComponent final : Component {
		explicit JointComponent(SceneEntity* p_ent) : Component(p_ent) {}
		JointComponent& operator=(const JointComponent&) = default;
		JointComponent(const JointComponent&) = default;
		~JointComponent() override = default;
	};

}

#endif
