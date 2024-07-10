#include "Asset.h"

namespace FMOD {
	class Sound;
}

namespace ORNG {
	struct SoundAsset : public Asset {
		SoundAsset(const std::string& t_filepath) : Asset(t_filepath) {};
		~SoundAsset();

		FMOD::Sound* p_sound = nullptr;

		std::string source_filepath;

		void CreateSoundFromFile();
		void CreateSoundFromBinary(const std::vector<std::byte>& data);

	};
}