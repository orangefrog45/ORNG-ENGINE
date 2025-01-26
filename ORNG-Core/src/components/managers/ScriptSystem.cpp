#include "pch/pch.h"
#include "components/ComponentSystems.h"
#include "assets/AssetManager.h"
#include "core/Window.h"
#include "rendering/SceneRenderer.h"
#include "rendering/Renderer.h"

using namespace ORNG;

void ScriptSystem::OnLoad() {
	auto& reg = mp_scene->GetRegistry();

	m_script_destroy_connection = reg.on_destroy<ScriptComponent>().connect<&ScriptSystem::OnScriptDestroy>();

	for (auto* p_script_asset : AssetManager::GetView<ScriptAsset>()) {
		if (p_script_asset->symbols.loaded) {
			auto* p_instance = p_script_asset->symbols.CreateInstance();
			p_script_asset->symbols.DestroyInstance(p_instance);
		}
	}
}

