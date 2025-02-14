#pragma once

class FastNoiseSIMD;

namespace ORNG {
	class Texture3D;

	struct GlobalFog {
		glm::vec3 colour{ 1 };
		float scattering_coef = 0.04f;
		float absorption_coef = 0.003f;
		float density_coef = 0.0f;
		int step_count = 32;
		float emissive_factor = 0.5f;
		float scattering_anisotropy = 0.85f;
	};

	struct Bloom {
		float threshold = 1.0f;
		float knee = 0.1f;
		float intensity = 1.0f;
	};


	struct PostProcessingSettings {
		Bloom bloom;
		GlobalFog global_fog;
	};
}