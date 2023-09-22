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

		static void RenderDebug(const physx::PxRenderBuffer& buf, Texture2D* p_output_tex, Texture2D* p_input_depth) {
			Get().IRenderDebug(buf, p_output_tex, p_input_depth);
		}

		static glm::vec3 GetPhysxDebugCol(physx::PxDebugColor::Enum col_id) {
			ASSERT(Get().m_phys_debug_cols.contains(col_id));
			return Get().m_phys_debug_cols[col_id];
		}

		static glm::vec3 GetPhysxDebugCol(physx::PxU32 col_id) {
			return GetPhysxDebugCol((physx::PxDebugColor::Enum)col_id);
		}
	private:
		void I_Init();

		void IShutdown();

		void InitDebugRenderPass();

		void IRenderDebug(const physx::PxRenderBuffer& buf, Texture2D* p_output_tex, Texture2D* p_input_depth);

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

		Shader* p_debug_shader = nullptr;
		Framebuffer* p_debug_render_fb = nullptr;
		VAO m_physx_data_vao;

		std::map<physx::PxDebugColor::Enum, glm::vec3> m_phys_debug_cols;
		float m_tolerances_scale = 1.f;

	};
}
