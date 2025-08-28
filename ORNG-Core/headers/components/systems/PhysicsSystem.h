#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

#include "components/systems/ComponentSystem.h"
#include "scene/SceneSerializer.h"
#include "components/PhysicsComponent.h"
#include "components/TransformComponent.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "scripting/ScriptShared.h"

namespace YAML {
	class Emitter;
	class Node;
}

namespace ORNG {
	// Layer that objects can be in, determines which other objects it can collide with
	// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
	// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
	// but only if you do collision testing).
	namespace Layers {
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
	};

	/// Class that determines if two object layers can collide
	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
	public:
		bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
			switch (inObject1) {
				case Layers::NON_MOVING:
					return inObject2 == Layers::MOVING; // Non moving only collides with moving
				case Layers::MOVING:
					return true; // Moving collides with everything
				default:
					JPH_ASSERT(false);
					return false;
			}
		}
	};

	namespace BroadPhaseLayers {
		static constexpr JPH::BroadPhaseLayer NON_MOVING{0};
		static constexpr JPH::BroadPhaseLayer MOVING{1};
		static constexpr unsigned NUM_LAYERS = 2;
	};

	// BroadPhaseLayerInterface implementation
	// This defines a mapping between object and broadphase layers.
	class BPLayerInterfaceImpl final : public JPH::BroadPhaseLayerInterface {
	public:
		BPLayerInterfaceImpl() {
			// Create a mapping table from object to broad phase layer
			mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
		}

		JPH::uint GetNumBroadPhaseLayers() const override {
			return BroadPhaseLayers::NUM_LAYERS;
		}

		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
			JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
			return mObjectToBroadPhase[inLayer];
		}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
			switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer)) {
				case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING):	return "NON_MOVING";
				case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):		return "MOVING";
				default: JPH_ASSERT(false); return "INVALID";
			}
		}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
	};

	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
	public:
		bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
			switch (inLayer1) {
				case Layers::NON_MOVING:
					return inLayer2 == BroadPhaseLayers::MOVING; // Non-moving only collides with moving
				case Layers::MOVING:
					return true; // Moving collides with everything
				default:
					JPH_ASSERT(false);
					return false;
			}
		}
	};





	struct PhysicsCollisionEvent : public Events::Event {
		PhysicsCollisionEvent(SceneEntity* _p0, SceneEntity* _p1) : p0(_p0), p1(_p1) {}
		SceneEntity* p0 = nullptr;
		SceneEntity* p1 = nullptr;
	};

	class PhysicsSystem : public ComponentSystem {
		friend class EditorLayer;
	public:
		explicit PhysicsSystem(Scene* p_scene);
		~PhysicsSystem() override = default;

		void OnUpdate() override;
		void OnUnload() override;
		void OnLoad() override;

		inline static constexpr uint64_t GetSystemUUID() { return 29348475677; }

		enum class ActorType : uint8_t {
			RIGID_BODY,
			CHARACTER_CONTROLLER,
			VEHICLE
		};

		float step_size = 1.f / 90.f;
		JPH::PhysicsSystem physics_system;
	private:
		void Tick();

		void InitComponent(PhysicsComponent* p_comp);

		void UpdateComponentState(PhysicsComponent* p_comp);
		void OnTransformEvent(const Events::ECS_Event<TransformComponent>& t_event);

		void RemoveComponent(PhysicsComponent* p_comp);

		void InitListeners();
		void DeinitListeners();

		void SerializeEntity(SceneEntity& entity, YAML::Emitter* p_emitter);
		void DeserializeEntity(SceneEntity& entity, const YAML::Node* p_node);

		float m_accumulator = 0.f;
		bool m_is_updating = true;
		// Increments whenever a body is added or deleted, resets to 0 upon optimization
		int num_body_events_since_last_broadphase_optimization = 0;

		TransformComponent* mp_currently_updating_transform = nullptr;

		// These can only be created after the Factory singleton, so they're kept as unique ptrs.
		std::unique_ptr<JPH::TempAllocatorImpl> mp_temp_allocator = nullptr;
		std::unique_ptr<JPH::JobSystemThreadPool> mp_job_system = nullptr;

		BPLayerInterfaceImpl m_broad_phase_layer_interface;
		ObjectVsBroadPhaseLayerFilterImpl m_object_vs_broadphase_layer_filter;
		ObjectLayerPairFilterImpl m_object_vs_object_layer_filter;

		std::array<entt::connection, 8> m_connections;
		Events::ECS_EventListener<PhysicsComponent> m_phys_listener;
		Events::ECS_EventListener<TransformComponent> m_transform_listener;
		Events::EventListener<EntitySerializationEvent> m_serialization_listener;

		// Number of loaded instances of this class, used for managing the factory singleton
		inline static int s_num_loaded_instances = 0;
	};
}
