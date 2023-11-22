#include "util/util.h"
#include "assets/PhysXMaterialAsset.h"
#include "assets/SoundAsset.h"
#include "audio/AudioEngine.h"
#include <fmod.hpp>
#include <fmod_errors.h>


namespace ORNG {

	void SoundAsset::CreateSound() {
		if (auto result = AudioEngine::GetSystem()->createSound(source_filepath.c_str(), FMOD_DEFAULT | FMOD_3D | FMOD_LOOP_OFF, nullptr, &p_sound); result != FMOD_OK) {
			ORNG_CORE_ERROR("Error loading sound: '{0}', '{1}'", source_filepath, FMOD_ErrorString(result));
		}
	}

	SoundAsset::~SoundAsset() {
		if (p_sound)
			p_sound->release();
	}

}