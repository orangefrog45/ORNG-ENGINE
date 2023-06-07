#pragma once
#include "rendering/Textures.h"


namespace ORNG {
	struct GlobalFog {
		glm::vec3 color{ 1 };
		float scattering_coef = 0.04f;
		float absorption_coef = 0.003f;
		float density_coef = 0.f;
		int step_count = 32;
		float scattering_anistropy;
		Texture3D fog_noise{ "Global fog noise" };
	};
}