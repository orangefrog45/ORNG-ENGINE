#include "util/util.h"
#include "assets/SoundAsset.h"
#include "audio/AudioEngine.h"
#include <fmod.hpp>
#include <fmod_errors.h>


namespace ORNG {

	void SoundAsset::CreateSoundFromFile() {
		if (auto result = AudioEngine::GetSystem()->createSound(source_filepath.c_str(),  FMOD_3D | FMOD_LOOP_OFF, nullptr, &p_sound); result != FMOD_OK) {
			ORNG_CORE_ERROR("Error loading sound: '{0}', '{1}'", source_filepath, FMOD_ErrorString(result));
		}
	}

	void SoundAsset::CreateSoundFromBinary(const std::vector<std::byte>& data) {
		FMOD_CREATESOUNDEXINFO info{};
		info.length = data.size();
		info.filebuffersize = data.size();
		info.fileoffset = 0;
		info.cbsize = sizeof(info);

		if (auto result = AudioEngine::GetSystem()->createSound(reinterpret_cast<const char*>(data.data()), FMOD_OPENMEMORY | FMOD_3D | FMOD_LOOP_OFF, &info, &p_sound); result != FMOD_OK) {
			ORNG_CORE_ERROR("Error loading sound from binary: '{0}'", FMOD_ErrorString(result));
		}
	}

	SoundAsset::~SoundAsset() {
		if (p_sound)
			p_sound->release();
	}

}