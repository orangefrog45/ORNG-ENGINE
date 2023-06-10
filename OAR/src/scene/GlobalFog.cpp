#include "pch/pch.h"
#include "scene/GlobalFog.h"
#include "../extern/fastsimd/FastNoiseSIMD-master/FastNoiseSIMD/FastNoiseSIMD.h"

namespace ORNG {

	void GlobalFog::SetNoise(FastNoiseSIMD* p_noise) {
		float* noise_set = p_noise->GetCellularSet(0, 0, 0, 64, 64, 64);

		Texture3DSpec fog_noise_spec;
		fog_noise_spec.format = GL_RED;
		fog_noise_spec.internal_format = GL_R16F;
		fog_noise_spec.storage_type = GL_FLOAT;
		fog_noise_spec.width = 64;
		fog_noise_spec.height = 64;
		fog_noise_spec.layer_count = 64;
		fog_noise_spec.min_filter = GL_NEAREST;
		fog_noise_spec.mag_filter = GL_NEAREST;
		fog_noise_spec.wrap_params = GL_MIRRORED_REPEAT;
		fog_noise.SetSpec(fog_noise_spec);
		glTexImage3D(GL_TEXTURE_3D, 0, fog_noise_spec.internal_format, fog_noise_spec.width, fog_noise_spec.height, fog_noise_spec.layer_count, 0, fog_noise_spec.format, fog_noise_spec.storage_type, noise_set);
	}
}