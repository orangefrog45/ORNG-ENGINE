#pragma once
#include "events/Events.h"
#include "rendering/renderpasses/Renderpass.h"
#include "rendering/Textures.h"

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

		void Destroy() override;

	private:
		Texture2D m_bloom_tex{ "Bloom tex" };
		Shader* mp_bloom_downsample_shader = nullptr;
		Shader* mp_bloom_upsample_shader = nullptr;
		Shader* mp_composition_shader = nullptr;
		Shader* mp_bloom_threshold_shader = nullptr;
	};
}