#pragma once
namespace FMOD {
	class System;
}

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