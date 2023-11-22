#include "Asset.h"

namespace FMOD {
	class Sound;
}

namespace ORNG {
	struct SoundAsset : public Asset {
		SoundAsset(const std::string& t_filepath) : Asset(t_filepath) {};
		~SoundAsset();

		FMOD::Sound* p_sound = nullptr;
		// Filepath of audio data, separate from the filepath of this asset (.osound file)
		std::string source_filepath;

		template<typename S>
		void serialize(S& s) {
			s.object(uuid);
			s.text1b(source_filepath, ORNG_MAX_FILEPATH_SIZE);
		}

		void CreateSound();
	};
}