#include "pch/pch.h"
#include "physics/Physics.h"
#include "pvd/PxPvd.h"
#include "rendering/Renderer.h"
#include "core/CodedAssets.h"

namespace ORNG {
	using namespace physx;
	void Physics::IShutdown() {
		/*#ifndef NDEBUG
				mp_pvd->disconnect();
				mp_pvd->release();
		#endif
				mp_cuda_context_manager->release();
				PxCloseExtensions();
				mp_physics->release();
				mp_foundation->release();*/
	}

	void Physics::I_Init() {
		InitDebugRenderPass();
		mp_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, g_default_allocator_callback, g_default_error_callback);
		if (!mp_foundation) {
			ORNG_CORE_CRITICAL("PxCreateFoundation failed");
			BREAKPOINT;
		}


		bool record_memory_allocations = true;

#ifndef NDEBUG
		mp_pvd = PxCreatePvd(*mp_foundation);
		mp_transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

		mp_pvd->connect(*mp_transport, PxPvdInstrumentationFlag::eALL | PxPvdInstrumentationFlag::eDEBUG);
#endif
		mp_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *mp_foundation, PxTolerancesScale(), record_memory_allocations, mp_pvd);



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

		PxCudaContextManagerDesc cudaContextManagerDesc;
		mp_cuda_context_manager = PxCreateCudaContextManager(*Physics::GetFoundation(), cudaContextManagerDesc, PxGetProfilerCallback());

		using enum PxDebugColor::Enum;
		m_phys_debug_cols[eARGB_BLACK] = { 0, 0, 0 };
		m_phys_debug_cols[eARGB_RED] = { 1, 0, 0 };
		m_phys_debug_cols[eARGB_GREEN] = { 0, 1, 0 };
		m_phys_debug_cols[eARGB_BLUE] = { 0, 0, 1 };
		m_phys_debug_cols[eARGB_YELLOW] = { 1, 1, 0 };
		m_phys_debug_cols[eARGB_MAGENTA] = { 1, 0, 1 };
		m_phys_debug_cols[eARGB_CYAN] = { 0, 0.5, 1 };
		m_phys_debug_cols[eARGB_WHITE] = { 1, 1, 1 };
		m_phys_debug_cols[eARGB_GREY] = { 0.5, 0.5, 0.5 };
		m_phys_debug_cols[eARGB_DARKRED] = { 0.5, 0, 0 };
		m_phys_debug_cols[eARGB_DARKGREEN] = { 0, 0.5, 0 };
		m_phys_debug_cols[eARGB_DARKBLUE] = { 0, 0, 0.5 };
	}


	void Physics::IRenderDebug(const physx::PxRenderBuffer& buf, Texture2D* p_output_tex, Texture2D* p_input_depth) {
		p_debug_shader->ActivateProgram();
		p_debug_render_fb->Bind();
		p_debug_render_fb->BindTexture2D(p_output_tex->GetTextureHandle(), GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
		p_debug_render_fb->BindTexture2D(p_input_depth->GetTextureHandle(), GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
		p_debug_shader->SetUniform("transform", glm::mat4(1));



		auto* p_position_buf = m_physx_data_vao.GetBuffer<VertexBufferGL<float>>(0);
		auto* p_col_buf = m_physx_data_vao.GetBuffer<VertexBufferGL<float>>(1);
		p_position_buf->data.clear();
		p_col_buf->data.clear();

		for (int i = 0; i < buf.getNbPoints(); i++) {
			auto& point = buf.getPoints()[i];
			VEC_PUSH_VEC3(p_position_buf->data, point.pos);
			VEC_PUSH_VEC3(p_col_buf->data, GetPhysxDebugCol(point.color));
		}
		m_physx_data_vao.FillBuffers();
		Renderer::DrawVAOArrays(m_physx_data_vao, buf.getNbPoints(), GL_POINTS);


		p_position_buf->data.clear();
		p_col_buf->data.clear();
		for (int i = 0; i < buf.getNbLines(); i++) {
			auto& phys_line = buf.getLines()[i];
			VEC_PUSH_VEC3(p_position_buf->data, phys_line.pos0);
			VEC_PUSH_VEC3(p_position_buf->data, phys_line.pos1);
			VEC_PUSH_VEC3(p_col_buf->data, GetPhysxDebugCol(phys_line.color0));
			VEC_PUSH_VEC3(p_col_buf->data, GetPhysxDebugCol(phys_line.color1));
		}
		m_physx_data_vao.FillBuffers();
		Renderer::DrawVAOArrays(m_physx_data_vao, buf.getNbLines(), GL_LINES);


		p_position_buf->data.clear();
		p_col_buf->data.clear();
		for (int i = 0; i < buf.getNbTriangles(); i++) {
			auto& phys_tri = buf.getTriangles()[i];
			VEC_PUSH_VEC3(p_position_buf->data, phys_tri.pos0);
			VEC_PUSH_VEC3(p_position_buf->data, phys_tri.pos1);
			VEC_PUSH_VEC3(p_position_buf->data, phys_tri.pos2);
			VEC_PUSH_VEC3(p_col_buf->data, GetPhysxDebugCol(phys_tri.color0));
			VEC_PUSH_VEC3(p_col_buf->data, GetPhysxDebugCol(phys_tri.color1));
			VEC_PUSH_VEC3(p_col_buf->data, GetPhysxDebugCol(phys_tri.color2));
		}
		m_physx_data_vao.FillBuffers();
		Renderer::DrawVAOArrays(m_physx_data_vao, buf.getNbTriangles(), GL_TRIANGLES);
	}

	void Physics::InitDebugRenderPass() {
		p_debug_shader = &Renderer::GetShaderLibrary().CreateShader("physx-debug");

		p_debug_shader->AddStage(GL_VERTEX_SHADER, "res/shaders/TransformVS.glsl", { "COLOR" });
		p_debug_shader->AddStage(GL_FRAGMENT_SHADER, "res/shaders/ColorFS.glsl");
		p_debug_shader->Init();
		p_debug_shader->AddUniform("transform");
		p_debug_render_fb = &Renderer::GetFramebufferLibrary().CreateFramebuffer("physx-debug", false);
		p_debug_render_fb->Init();

		m_physx_data_vao.Init();
		auto* p_position_buf = m_physx_data_vao.AddBuffer<VertexBufferGL<float>>(0, GL_FLOAT, 3, GL_STREAM_DRAW);
		auto* p_color_buf = m_physx_data_vao.AddBuffer<VertexBufferGL<float>>(1, GL_FLOAT, 3, GL_STREAM_DRAW);
	}
}