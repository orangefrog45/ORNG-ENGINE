#include "pch/pch.h"
#include "components/Component.h"
#include "scene/SceneEntity.h"

namespace ORNG {
	Component::Component(SceneEntity* p_entity) : mp_entity(p_entity) {};

	unsigned long Component::GetEntityHandle() const { return mp_entity->GetID(); }
}