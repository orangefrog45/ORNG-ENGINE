#include "entt/EnttSingleInclude.h"
namespace ORNG {

	class Scene;
	class AssetManager;


	class SceneManager {
	public:
		Scene* p_active_scene;
		std::vector<Scene*> scenes;

	private:

	};
}