#include "core/AssetManager.h"
#include "entt/EnttSingleInclude.h"
namespace ORNG {

	class Scene;
	class AssetManager;


	class SceneManager {
	public:
		Scene* p_active_scene;
		std::vector<Scene*> scenes;
		AssetManager asset_manager;

	private:

	};
}