#pragma once
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#elif defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4505 ) // 'FMOD_ErrorString': unreferenced function with internal linkage has been removed
#endif
#include <fmod_errors.h>
#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning( pop )
#endif


#include "util/Log.h"
#include "util/util.h"


namespace FMOD {
	class System;
}

//#define ORNG_CALL_FMOD(x) if (x != FMOD_OK){ ORNG_CORE_ERROR("FMOD Error at '{0}', line '{1}' : '{2}", FUNC_NAME, __LINE__, FMOD_ErrorString(x));}
#define ORNG_CALL_FMOD(x) x

namespace ORNG {
	class AudioEngine {
	public:

		static FMOD::System* GetSystem() {
			return Get().mp_system;
		}

		static void Init() { Get().I_Init(); }

		~AudioEngine();

	private:
		static AudioEngine& Get() {
			static AudioEngine s_instance;
			return s_instance;
		}

		FMOD::System* mp_system = nullptr;
		void I_Init();
	};
}
