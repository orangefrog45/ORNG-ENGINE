#ifndef PHYSICSCOMPONENT_H
#define PHYSICSCOMPONENT_H

#include "Component.h"
#include "physx/extensions/PxD6Joint.h"

#include "scene/EntityNodeRef.h"
#include "util/UUID.h"

namespace physx {
	class PxRigidDynamic;
	class PxRigidStatic;
	class PxRigidActor;
	class PxShape;
	class PxMaterial;
	class PxScene;
	class PxController;
	class PxActor;
	class PxD6Joint;
}

namespace ORNG {
	class PhysicsSystem;
	class TransformComponent;
	struct PhysXMaterialAsset;



	class PhysicsComponent : public Component {
		friend class PhysicsSystem;
		friend class EditorLayer;
		friend class SceneSerializer;
		friend class FixedJointComponent;
	public:
		PhysicsComponent(SceneEntity* p_entity) : Component(p_entity) {};

		enum GeometryType {
			BOX = 0,
			SPHERE = 1,
			TRIANGLE_MESH = 2,
		};
		enum RigidBodyType {
			STATIC = 0,
			DYNAMIC = 1,
		};

		PhysicsComponent(SceneEntity* p_entity, bool is_trigger, GeometryType geom_type, RigidBodyType body_type, PhysXMaterialAsset* _material) : Component(p_entity), m_is_trigger(is_trigger),
			m_geometry_type(geom_type), m_body_type(body_type), p_material(_material) {};

		void SetVelocity(glm::vec3 v);
		void SetAngularVelocity(glm::vec3 v);
		glm::vec3 GetVelocity() const;
		glm::vec3 GetAngularVelocity() const;

		void SetMaterial(PhysXMaterialAsset& material);

		void AddForce(glm::vec3 force);
		void ToggleGravity(bool on);
		void SetMass(float mass);

		void UpdateGeometry(GeometryType type);
		void SetBodyType(RigidBodyType type);

		void SetTrigger(bool is_trigger);
		bool IsTrigger() { return m_is_trigger; }

		PhysXMaterialAsset* GetMaterial() {
			return p_material;
		}


		physx::PxRigidActor* p_rigid_actor = nullptr;
	private:
		void SendUpdateEvent();

		physx::PxShape* p_shape = nullptr;

		PhysXMaterialAsset* p_material = nullptr;
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

		physx::PxController* p_controller = nullptr;
	};

	enum JointEventType {
		CONNECT,
		BREAK
	};

	struct JointComponent : public Component {

		class Joint {
			friend class SceneSerializer;
			friend class PhysicsSystem;
			friend class EditorLayer;
		public:
			Joint(SceneEntity* _owner) : p_a0(_owner) {
				std::fill_n(m_motion.data(), 6, physx::PxD6Motion::eLOCKED);
			};

			// Sets motion and caches it for serialization, this->p_joint->setMotion can be called directly if serialization isn't needed
			inline void SetMotionCached(physx::PxD6Axis::Enum axis, physx::PxD6Motion::Enum _motion) {
				if (p_joint)
					p_joint->setMotion(axis, _motion);

				m_motion[axis] = _motion;
			}

			// Sets local pose and caches it for serialization, this->p_joint->setLocalPose can be called directly if serialization isn't needed
			inline void SetLocalPoseCached(unsigned actor_idx, glm::vec3 p) {
				if (p_joint) 
					p_joint->setLocalPose((physx::PxJointActorIndex::Enum)actor_idx, physx::PxTransform{ physx::PxVec3(p.x, p.y, p.z), p_joint->getLocalPose((physx::PxJointActorIndex::Enum)actor_idx).q });

				m_poses[actor_idx] = p;
			}

			// Sets break force and caches it for serialization, this->p_joint->setBreakForce can be called directly if serialization isn't needed
			inline void SetBreakForceCached(float _force_threshold, float _torque_threshold) {
				m_force_threshold = _force_threshold;
				m_torque_threshold = _torque_threshold;

				if (p_joint) {
					p_joint->setBreakForce(m_force_threshold, m_torque_threshold);
				}
			}

			// A0 = attachment point 0 (idx 0 on physx joint), this is the owning entity of the joint
			SceneEntity* GetA0() {
				return p_a0;
			}

			// A1 = attachment point 1 (idx 1 on physx joint), does not own joint
			SceneEntity* GetA1() {
				return p_a1;
			}

			// Connects attachment point 1 to p_target, does nothing if either doesn't have a physics component
			// p_target must be related to this->p_owner, e.g it can be reached soley with GetParent()/GetChild() calls
			void Connect(JointComponent* p_target, bool use_stored_poses = false);

			void Break();

			physx::PxD6Joint* p_joint = nullptr;

		private:
			// Owner of the joint, the joint cannot be detached from this entity unless the joint itself breaks and is deleted
			// Will never be nullptr or invalid, if this entity dies the joint will be broken
			SceneEntity* p_a0 = nullptr;

			// Attachment 1, non-owning, Can be detached from entity if Connect() is called with a different p_target
			// Can be nullptr if joint hasn't been connected yet
			SceneEntity* p_a1 = nullptr;

			/* The fields below are only used for serialization */
			std::array<glm::vec3, 2> m_poses = { glm::vec3(0) };

			/*
				eX      = [0],
				eY      = [1],
				eZ      = [2],
				eTWIST  = [3],
				eSWING1 = [4],
				eSWING2 = [5],
				eCOUNT	= 6
			*/
			std::array<physx::PxD6Motion::Enum, 6> m_motion;

			float m_force_threshold = PX_MAX_F32;
			float m_torque_threshold = PX_MAX_F32;
		};

		struct ConnectionData {
			ConnectionData(SceneEntity* _src, JointComponent* _target, Joint* _joint, bool _use_poses) : p_src(_src), p_target(_target), p_joint(_joint), use_stored_poses(_use_poses) {};

			SceneEntity* p_src = nullptr;
			JointComponent* p_target = nullptr;

			Joint* p_joint = nullptr;

			bool use_stored_poses = false;
		};

		class JointAttachment {
			friend class SceneSerializer;
			friend class Scene;
		public:
			JointAttachment() = default;
			JointAttachment(uint64_t target_uuid, Joint* _p_data) : m_target_uuid(target_uuid), p_joint(_p_data) {}

			Joint* p_joint = nullptr;

			uint64_t GetTargetUUID() const noexcept {
				return m_target_uuid;
			}

		private:
			static constexpr uint64_t INVALID_UUID = 0;
			uint64_t m_target_uuid = INVALID_UUID;
		};

		// Returns an uninitialized joint, Pose/Motion can be set up with SetMotion/LocalPose before or after joint is connected
		// The memory for this is deallocated automatically by PhysicsSystem when the joint breaks or the JointComponent it belongs to is destroyed, and if it's attached to another JointComponent it is removed from it
		JointAttachment& CreateJoint() {
			auto* p_joint = new Joint(GetEntity());
			attachments[p_joint].p_joint = p_joint;

			return attachments[p_joint];
		}

		std::unordered_map<Joint*, JointAttachment> attachments;
	};

}

#endif