#include "pch/pch.h"
#include "physics/Physics.h"

namespace ORNG {
	using namespace physx;
	void Physics::IShutdown() {
		//mp_physics->release();
		//PxCloseExtensions();
		//mp_foundation->release();
	}

	void Physics::I_Init() {
		mp_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_default_allocator_callback, g_default_error_callback);
		if (!mp_foundation) {
			ORNG_CORE_CRITICAL("PxCreateFoundation failed");
			BREAKPOINT;
		}

		constexpr bool record_memory_allocations = false;
		mp_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *mp_foundation, PxTolerancesScale(), record_memory_allocations, nullptr);

		ASSERT(vehicle2::PxInitVehicleExtension(*mp_foundation));

		if (!mp_physics) {
			ORNG_CORE_CRITICAL("PxCreatePhysics failed");
			BREAKPOINT;
		}

		const PxU32 num_threads = glm::max((int)std::thread::hardware_concurrency() - 2, 1);
		mp_dispatcher = PxDefaultCpuDispatcherCreate(num_threads);

		if (!PxInitExtensions(*mp_physics, nullptr)) {
			ORNG_CORE_CRITICAL("PxInitExtensions failed");
			BREAKPOINT;
		}

#ifdef PHYSX_GPU_ACCELERATION_AVAILABLE
		if (PxGetSuggestedCudaDeviceOrdinal(mp_foundation->getErrorCallback()) >= 0) {
			PxCudaContextManagerDesc cudaContextManagerDesc;
			mp_cuda_context_manager = PxCreateCudaContextManager(*Physics::GetFoundation(), cudaContextManagerDesc, PxGetProfilerCallback());
			ASSERT(mp_cuda_context_manager && mp_cuda_context_manager->contextIsValid());
		}
#endif
	}
}