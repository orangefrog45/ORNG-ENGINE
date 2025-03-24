#include "pch/pch.h"
#include "assets/AssetManager.h"
#include "core/FrameTiming.h"
#include "components/systems/ScriptSystem.h"

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

void ScriptSystem::OnUpdate() {
	const float dt = FrameTiming::GetTimeStep() * 0.001f;
	for (auto [entity, script] : mp_scene->GetRegistry().view<ScriptComponent>().each()) {
		script.p_instance->OnUpdate(dt); 
	}
}


