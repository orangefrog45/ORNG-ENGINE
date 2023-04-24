#pragma once

class Quad;
struct RenderPasses
{
	static void InitPasses();
	static void DoLightingPass();
	static void DoGBufferPass();
	static void DoPickingPass();
	static void DoDrawToQuadPass(Quad* quad);

	inline static unsigned int current_entity_id = 0;
	inline static glm::vec3 current_pos = glm::vec3(0);
	inline static bool gbuffer_positions_sample_flag = false;
};