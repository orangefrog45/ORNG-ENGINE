#include "pch/pch.h"
#include "components/ScriptComponent.h"
#include "scene/SceneEntity.h"

using namespace ORNG;

void ScriptComponent::SetSymbols(const ScriptSymbols* t_symbols) {
	if (p_instance)
		p_symbols->DestroyInstance(p_instance);

	p_symbols = t_symbols;
	p_instance = t_symbols->CreateInstance();
	p_instance->p_entity = GetEntity();
	p_instance->p_scene = GetEntity()->GetScene();
}