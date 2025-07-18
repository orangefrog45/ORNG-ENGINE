#pragma once
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"

namespace ORNG {
	class LightingPass : public Renderpass {
	public:
		LightingPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Lighting") {};

		void Init() override;

		void DoPass() override;

		void Destroy() override;
	private:
		class Scene* mp_scene = nullptr;

		// Accumulated cone tracing radiance
		FullscreenTexture2D m_cone_trace_accum_tex{ {0.5f, 0.5f} };

		// Applies direct lighting over image
		Shader m_shader;

		// Used for upscaling cone tracing radiance tex
		ShaderVariants m_depth_aware_upsample_sv;

		// Applies cone traced radiance over image if a VoxelPass is present
		Shader m_cone_trace_shader;
		// nullptr if VoxelPass not in render graph
		class VoxelPass* mp_voxel_pass = nullptr;

		// nullptr if DepthPass not in render graph
		class DepthPass* mp_depth_pass = nullptr;


		Texture2D* mp_gbf_albedo_tex = nullptr;
		Texture2D* mp_gbf_normal_tex = nullptr;
		Texture2D* mp_gbf_rma_tex = nullptr;
		Texture2D* mp_gbf_shader_id_tex = nullptr;
		Texture2D* mp_gbf_depth_tex = nullptr;

		TextureCubemapArray* mp_pointlight_depth_tex = nullptr;
		Texture2DArray* mp_spotlight_depth_tex = nullptr;

	};
}