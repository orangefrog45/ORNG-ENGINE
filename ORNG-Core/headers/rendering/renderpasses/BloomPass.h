#pragma once
#include "events/Events.h"
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"
#include "shaders/Shader.h"

namespace ORNG {
	class Shader;

	class BloomPass : public Renderpass {
	public:
		BloomPass(class RenderGraph* p_graph) : Renderpass(p_graph, "Bloom") {};

		void Init() override;

		// p_input - The input texture that the bloom effect will be created from
		// p_output - The texture that will have the bloom effect composited over it
		// p_input can be p_output, resolutions must match
		void DoPass() override;

		void Destroy() override {};

	private:
		Texture2D bloom_tex{ "Bloom tex" };
		Shader bloom_downsample_shader;
		Shader bloom_upsample_shader;
		Shader composition_shader;
		Shader bloom_threshold_shader;
	};
}