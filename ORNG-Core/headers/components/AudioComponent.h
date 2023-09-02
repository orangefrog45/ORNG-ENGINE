#include "Component.h"

namespace FMOD {
	class Channel;
}

namespace ORNG {
	class SoundAsset;

	enum class AudioEventType {
		PLAY,
		PAUSE
	};

	class AudioComponent : public Component {
		friend class AudioSystem;
	public:
		explicit AudioComponent(SceneEntity* p_entity) : Component(p_entity) {};
		void SetSound(const SoundAsset* p_asset);

		void Play();
		void Pause();

	private:
		FMOD::Channel* mp_channel = nullptr;
		SoundAsset* mp_asset = nullptr;
	};
}