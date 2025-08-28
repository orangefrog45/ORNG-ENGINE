#pragma once
#include "Asset.h"

namespace FMOD {
	class Sound;
}

namespace ORNG {
	class SoundAsset final : public Asset {
	public:
		explicit SoundAsset(const std::string& t_filepath) : Asset(t_filepath) {}
		~SoundAsset() override;

		FMOD::Sound* p_sound = nullptr;

		std::string source_filepath;

		void CreateSoundFromFile();
		void CreateSoundFromBinary(const std::vector<std::byte>& data);

	};
}
