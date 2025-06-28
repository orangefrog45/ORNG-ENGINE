#pragma once
#include <PxPhysicsAPI.h>
#include <glm/glm/gtc/quaternion.hpp>
#include <glm/glm/trigonometric.hpp>

#include "util/Log.h"
#include "util/util.h"
#include "components/TransformComponent.h"

class UserErrorCallback : public physx::PxErrorCallback
{
public:
	virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		// error processing implementation
		ORNG_CORE_CRITICAL("Nvidia PhysX error: '{0}', '{1}', '{2}', '{3}'", code, message, file, line);
		BREAKPOINT;
	}
};

namespace ORNG {
	class Texture2D;
	class Shader;
	class Framebuffer;

	inline static physx::PxTransform TransformComponentToPxTransform(TransformComponent& t) {
		glm::vec3 p = t.GetAbsPosition();
		glm::quat r = t.GetAbsOrientationQuat();

		return { ConvertVec3<physx::PxVec3>(p), physx::PxQuat{ r.x, r.y, r.z, r.w }};
	}

	class Physics {
	public:
		static void Init() {
			Get().I_Init();
		}

		static void Shutdown() {
			Get().IShutdown();
		}

		static physx::PxPhysics* GetPhysics() {
			return Get().mp_physics;
		}

		static physx::PxCudaContextManager* GetCudaContextManager() {
			return Get().mp_cuda_context_manager;
		}

		static physx::PxFoundation* GetFoundation() {
			return Get().mp_foundation;
		}

		static physx::PxCpuDispatcher* GetCPUDispatcher() {
			return Get().mp_dispatcher;
		}

		static float GetToleranceScale() {
			return Get().m_tolerances_scale;
		}


	private:
		void I_Init();

		void IShutdown();

		static Physics& Get() {
			static Physics s_instance;
			return s_instance;
		}

		physx::PxDefaultErrorCallback g_default_error_callback;
		physx::PxDefaultAllocator g_default_allocator_callback;

		physx::PxFoundation* mp_foundation = nullptr;
		physx::PxPhysics* mp_physics = nullptr;
		physx::PxCpuDispatcher* mp_dispatcher = nullptr;
		physx::PxCudaContextManager* mp_cuda_context_manager = nullptr;

		float m_tolerances_scale = 1.f;
	};
}
