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

		class Scene* p_scene = nullptr;

		// Accumulated cone tracing radiance
		FullscreenTexture2D cone_trace_accum_tex{ {0.5f, 0.5f} };

		// Applies direct lighting over image
		Shader shader;

		// Used for upscaling cone tracing radiance tex
		ShaderVariants depth_aware_upsample_sv;

		// Applies cone traced radiance over image if a VoxelPass is present
		Shader cone_trace_shader;
		// nullptr if VoxelPass not in render graph
		class VoxelPass* p_voxel_pass = nullptr;

		// nullptr if DepthPass not in render graph
		class DepthPass* p_depth_pass = nullptr;


		Texture2D* p_gbf_albedo_tex = nullptr;
		Texture2D* p_gbf_normal_tex = nullptr;
		Texture2D* p_gbf_rma_tex = nullptr;
		Texture2D* p_gbf_shader_id_tex = nullptr;
		Texture2D* p_gbf_depth_tex = nullptr;

		TextureCubemapArray* p_pointlight_depth_tex = nullptr;
		Texture2DArray* p_spotlight_depth_tex = nullptr;
	};
}