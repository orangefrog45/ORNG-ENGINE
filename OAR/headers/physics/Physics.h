#pragma once
#include "PxPhysicsAPI.h"
#include "extensions/PxExtensionsAPI.h"


#include "util/Log.h"
#include "util/util.h"


class UserErrorCallback : public physx::PxErrorCallback
{
public:
	virtual void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
	{
		// error processing implementation
		ORNG::OAR_CORE_CRITICAL("Nvidia PhysX error: '{0}', '{1}', '{2}', '{3}'", code, message, file, line);
		BREAKPOINT;
	}
};

namespace ORNG {
	using namespace physx;

	class Physics {
	public:
		static void Init() {
			Get().I_Init();
		}

		static void Shutdown() {
			Get().IShutdown();
		}

		static PxPhysics* GetPhysics() {
			return Get().mp_physics;
		}

		static PxCudaContextManager* GetCudaContextManager() {
			return Get().mp_cuda_context_manager;
		}

		static PxFoundation* GetFoundation() {
			return Get().mp_foundation;
		}

		static PxCpuDispatcher* GetCPUDispatcher() {
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

		PxDefaultErrorCallback g_default_error_callback;
		PxDefaultAllocator g_default_allocator_callback;

		PxFoundation* mp_foundation = nullptr;
		PxPvd* mp_pvd = nullptr;
		PxPvdTransport* mp_pvd_transport = nullptr;
		PxPhysics* mp_physics = nullptr;
		PxCooking* mp_cooking = nullptr;
		PxCpuDispatcher* mp_dispatcher = nullptr;
		PxCudaContextManager* mp_cuda_context_manager = nullptr;
		float m_tolerances_scale = 1.f;

	};

}