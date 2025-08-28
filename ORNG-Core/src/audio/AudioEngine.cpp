#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
#include <fmod.hpp>
#include <combaseapi.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "pch/pch.h"
#include "audio/AudioEngine.h"

namespace ORNG {

	void AudioEngine::I_Init() {
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		FMOD_RESULT result;
		result = FMOD::System_Create(&mp_system);

		// Create the main system object.
		if (result != FMOD_OK)
		{
			ORNG_CORE_ERROR("FMOD error! ({0}) {1}\n", result, FMOD_ErrorString(result));
			exit(-1);
		}

		result = mp_system->init(512, FMOD_INIT_3D_RIGHTHANDED, nullptr);    // Initialize FMOD.
		if (result != FMOD_OK)
		{
			ORNG_CORE_ERROR("FMOD error! ({0}) {1}\n", result, FMOD_ErrorString(result));
			exit(-1);
		}


	}

	AudioEngine::~AudioEngine() {
		mp_system->release();
	}
}
