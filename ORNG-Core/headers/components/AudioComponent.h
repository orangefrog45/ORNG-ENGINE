#include "Component.h"

namespace FMOD {
	class Channel;
}
struct FMOD_VECTOR;

namespace ORNG {
	class SoundAsset;

	class AudioComponent : public Component {
		friend class AudioSystem;
	public:
		explicit AudioComponent(SceneEntity* p_entity) : Component(p_entity) {};
		void SetSound(const SoundAsset* p_asset);

		void Play();
		void Pause();

		enum class AudioEventType {
			PLAY,
			PAUSE
		};

	private:
		// Memory managed by AudioSystem
		FMOD_VECTOR* mp_fmod_pos;
		FMOD_VECTOR* mp_fmod_vel;

		FMOD::Channel* mp_channel;
		const SoundAsset* mp_asset = nullptr;
	};
}