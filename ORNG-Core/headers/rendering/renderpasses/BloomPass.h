#pragma once
#include "events/Events.h"
#include "rendering/Textures.h"

namespace ORNG {
	class Shader;

	class BloomPass {
	public:
		void Init();

		// p_input - The input texture that the bloom effect will be created from
		// p_output - The texture that will have the bloom effect composited over it
		// p_input can be p_output, resolutions must match
		void DoPass(Texture2D* p_input, Texture2D* p_output, float intensity, float threshold, float knee);


		void ResizeTexture(unsigned new_width, unsigned new_height);
	private:
		Events::EventListener<Events::WindowEvent> m_window_listener;

		Texture2D m_bloom_tex{ "Bloom tex" };
		Shader* mp_bloom_downsample_shader = nullptr;
		Shader* mp_bloom_upsample_shader = nullptr;
		Shader* mp_composition_shader = nullptr;
		Shader* mp_bloom_threshold_shader = nullptr;
	};
}