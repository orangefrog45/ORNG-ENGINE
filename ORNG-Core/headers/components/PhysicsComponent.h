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
		explicit PhysicsComponent(SceneEntity* p_entity) : Component(p_entity) {};
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

		PhysicsComponent(SceneEntity* p_entity, bool is_trigger, GeometryType geom_type, RigidBodyType body_type) : Component(p_entity), m_is_trigger(is_trigger),
			m_geometry_type(geom_type), m_body_type(body_type) {};

		void SetVelocity(glm::vec3 v);
		void SetAngularVelocity(glm::vec3 v);
		glm::vec3 GetVelocity() const;
		glm::vec3 GetAngularVelocity() const;

		void AddForce(glm::vec3 force);
		void ToggleGravity(bool on);
		void SetMass(float mass);

		void UpdateGeometry(GeometryType type);
		void SetBodyType(RigidBodyType type);

		void SetTrigger(bool is_trigger);
		bool IsTrigger() { return m_is_trigger; }

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
		explicit CharacterControllerComponent(SceneEntity* p_entity) : Component(p_entity) {};

		// Movement will update transform at the end of each frame and can be overwritten by calling functions like LookAt afterwards
		// To avoid overwriting, ensure this is called AFTER any transform updates to the entity it is attached to
		void Move(glm::vec3 disp, float minDist, float elapsedTime);
		bool moved_during_frame = false;
	};

	enum JointEventType {
		CONNECT,
		BREAK
	};

	struct JointComponent final : Component {
		explicit JointComponent(SceneEntity* p_ent) : Component(p_ent) {};
		~JointComponent() override = default;

	};

}

#endif