#include "pch/pch.h"
#include "scene/ScenePostProcessing.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"
#include "rendering/Textures.h"


namespace ORNG {

	void GlobalFog::SetNoise(FastNoiseSIMD* p_noise) {
		float* noise_set = p_noise->GetPerlinFractalSet(0, 0, 0, 128, 128, 128);

		fog_noise = std::make_unique<Texture3D>("global fog noise");
		Texture3DSpec fog_noise_spec;
		fog_noise_spec.format = GL_RED;
		fog_noise_spec.internal_format = GL_R8;
		fog_noise_spec.storage_type = GL_FLOAT;
		fog_noise_spec.width = 128;
		fog_noise_spec.height = 128;
		fog_noise_spec.layer_count = 128;
		fog_noise_spec.min_filter = GL_LINEAR;
		fog_noise_spec.mag_filter = GL_LINEAR;
		fog_noise_spec.wrap_params = GL_MIRRORED_REPEAT;
		fog_noise->SetSpec(fog_noise_spec);
		glTexImage3D(GL_TEXTURE_3D, 0, fog_noise_spec.internal_format, fog_noise_spec.width, fog_noise_spec.height, fog_noise_spec.layer_count, 0, fog_noise_spec.format, fog_noise_spec.storage_type, noise_set);
	}

}