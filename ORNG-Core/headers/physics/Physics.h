#pragma once
#include <PxPhysicsAPI.h>


#include "util/Log.h"
#include "util/util.h"
#include "rendering/VAO.h"



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

	inline static physx::PxVec3 ToPxVec3(const glm::vec3& glmVec) {
		return physx::PxVec3(glmVec.x, glmVec.y, glmVec.z);
	}

	// Conversion from PxVec3 to glm::vec3
	inline static glm::vec3 ToGlmVec3(const physx::PxVec3& pxVec) {
		return glm::vec3(pxVec.x, pxVec.y, pxVec.z);
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
		physx::PxPvd* mp_pvd = nullptr;
		physx::PxPhysics* mp_physics = nullptr;
		physx::PxPvdTransport* mp_transport = nullptr;
		//physx::PxCooking* mp_cooking = nullptr;
		physx::PxCpuDispatcher* mp_dispatcher = nullptr;
		physx::PxCudaContextManager* mp_cuda_context_manager = nullptr;

		float m_tolerances_scale = 1.f;
	};
}
