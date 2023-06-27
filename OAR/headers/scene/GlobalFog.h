#pragma once
#include "rendering/Textures.h"


class FastNoiseSIMD;
namespace ORNG {

	struct GlobalFog {
		void SetNoise(FastNoiseSIMD* p_noise);

		Texture3D fog_noise{ "Global fog noise" };
		glm::vec3 color{ 1 };
		float scattering_coef = 0.04f;
		float absorption_coef = 0.003f;
		float density_coef = 0.1f;
		int step_count = 32;
		float emissive_factor = 0.5f;
		float scattering_anistropy = 0.85f;
	};
}