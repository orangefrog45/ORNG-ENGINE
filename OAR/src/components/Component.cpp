#include "pch/pch.h"

#include "components/Component.h"
#include "rendering/Renderer.h"


SceneEntity* Component::GetEntity() {
	return Renderer::GetScene()->m_entities[m_entity_handle];
}