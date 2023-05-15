#pragma once

namespace ORNG {

	class Quad;
	class Texture2D;
	struct RenderPasses
	{
		static void InitPasses();
		static void DoLightingPass();
		static void DoGBufferPass();
		static void DoPickingPass();
		static void DoDepthPass();
		static void DoFogPass();
		static void DoDrawToQuadPass();


		inline static std::array<glm::mat4, 3> dir_light_space_mat_outputs;

		inline static Texture2D* blue_noise = nullptr;

		struct FogPassInputs {
			float intensity = 0.f;
			float hardness = 0.f;
			glm::vec3 color = glm::vec3(0.3);
		};

		inline static FogPassInputs fog_data;



		inline static unsigned int shadow_map_resolution = 4096;
		inline static bool depth_map_view_active = false;

		inline static unsigned int current_entity_id = 0;
		inline static glm::vec3 current_pos = glm::vec3(0);
		inline static bool gbuffer_positions_sample_flag = false;
	};

}