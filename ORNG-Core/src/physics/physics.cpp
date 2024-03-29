#include "pch/pch.h"
#include "physics/Physics.h"
#include "pvd/PxPvd.h"


#define PHYSX_DEBUG

namespace ORNG {
	using namespace physx;
	void Physics::IShutdown() {
		/*#ifdef PHYSX_DEBUG
				mp_pvd->disconnect();
				mp_pvd->release();
		#endif
				mp_cuda_context_manager->release();
				mp_physics->release();
				PxCloseExtensions();
				mp_foundation->release();*/
	}

	void Physics::I_Init() {
		mp_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_default_allocator_callback, g_default_error_callback);
		if (!mp_foundation) {
			ORNG_CORE_CRITICAL("PxCreateFoundation failed");
			BREAKPOINT;
		}


		bool record_memory_allocations = false;
#ifdef PHYSX_DEBUG
		record_memory_allocations = true;
		mp_pvd = PxCreatePvd(*mp_foundation);
		mp_transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

		mp_pvd->connect(*mp_transport, PxPvdInstrumentationFlag::eALL | PxPvdInstrumentationFlag::eDEBUG);
#endif
		mp_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *mp_foundation, PxTolerancesScale(), record_memory_allocations, mp_pvd);

		ASSERT(vehicle2::PxInitVehicleExtension(*mp_foundation));

		if (!mp_physics) {
			ORNG_CORE_CRITICAL("PxCreatePhysics failed");
			BREAKPOINT;
		}

		//mp_cooking = PxCreateCooking(PX_PHYSICS_VERSION, *mp_foundation, PxCookingParams(m_tolerances_scale));
		//if (!mp_cooking) {
			//ORNG_CORE_CRITICAL("PxCreateCooking failed");
			//BREAKPOINT;
		//}


		const PxU32 num_threads = 8;
		mp_dispatcher = PxDefaultCpuDispatcherCreate(num_threads);

		if (!PxInitExtensions(*mp_physics, mp_pvd)) {
			ORNG_CORE_CRITICAL("PxInitExtensions failed");
			BREAKPOINT;
		}

		if (PxGetSuggestedCudaDeviceOrdinal(mp_foundation->getErrorCallback()) >= 0)
		{
			PxCudaContextManagerDesc cudaContextManagerDesc;
			mp_cuda_context_manager = PxCreateCudaContextManager(*Physics::GetFoundation(), cudaContextManagerDesc, PxGetProfilerCallback());
			ASSERT(mp_cuda_context_manager && mp_cuda_context_manager->contextIsValid());
		}
	}
}