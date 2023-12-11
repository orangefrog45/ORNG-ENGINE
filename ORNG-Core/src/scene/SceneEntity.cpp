#include "pch/pch.h"
#include "scene/SceneEntity.h"

namespace ORNG {

	void SceneEntity::SetParent(SceneEntity& parent_entity) {
		RemoveParent();
		auto* p_comp = AddComponent<RelationshipComponent>();
		auto* p_parent_comp = parent_entity.GetComponent<RelationshipComponent>();

		entt::entity current_parent_child = p_parent_comp->first;

		// Make sure parent is not a child of this entity, if it is return - the parent will not be set
		entt::entity current_parent = p_parent_comp->parent;
		while (current_parent != entt::null) {
			if (current_parent == m_entt_handle) // Check if you're setting this as a parent of one of its children (not allowed)
				return;
			current_parent = mp_registry->get<RelationshipComponent>(current_parent).parent;
		}
		p_comp->parent = entt::entity{ parent_entity.GetEnttHandle() };

		// If parent has no children, link this to first
		if (current_parent_child == entt::null) {
			p_parent_comp->first = m_entt_handle;
			p_parent_comp->num_children++;
			// Update transform hierarchy
			auto* p_transform = GetComponent<TransformComponent>();
			p_transform->m_parent_handle = p_parent_comp->GetEnttHandle();
			p_transform->RebuildMatrix(TransformComponent::UpdateType::ALL);
			return;
		}

		while (mp_registry->get<RelationshipComponent>(current_parent_child).next != entt::null) {
			current_parent_child = mp_registry->get<RelationshipComponent>(current_parent_child).next;
		}

		// Link this entity to last entity found in parents linked list of children
		auto& prev_child_of_parent = mp_registry->get<RelationshipComponent>(current_parent_child);
		prev_child_of_parent.next = m_entt_handle;
		p_comp->prev = entt::entity{ prev_child_of_parent.GetEnttHandle() };
		p_parent_comp->num_children++;

		// Update transform hierarchy
		auto* p_transform = GetComponent<TransformComponent>();
		p_transform->m_parent_handle = p_parent_comp->GetEnttHandle();
		p_transform->RebuildMatrix(TransformComponent::UpdateType::ALL);
	}

	void SceneEntity::RemoveParent() {
		auto* p_comp = AddComponent<RelationshipComponent>();

		if (p_comp->parent == entt::null)
			return;

		auto& parent_comp = mp_registry->get<RelationshipComponent>(p_comp->parent);
		parent_comp.num_children--;

		if (parent_comp.first == m_entt_handle)
			parent_comp.first = p_comp->next;


		// Patch hole in linked list
		if (auto* prev = mp_registry->try_get<RelationshipComponent>(p_comp->prev))
			prev->next = p_comp->next;

		if (auto* next = mp_registry->try_get<RelationshipComponent>(p_comp->next))
			next->prev = p_comp->prev;

		p_comp->next = entt::null;
		p_comp->prev = entt::null;
		p_comp->parent = entt::null;

		GetComponent<TransformComponent>()->m_parent_handle = entt::null;
	}



	void SceneEntity::ForEachChildRecursive(std::function<void(entt::entity)> func_ptr) {
		auto& rel_comp = mp_registry->get<RelationshipComponent>(m_entt_handle);
		entt::entity current_entity = rel_comp.first;

		for (int i = 0; i < rel_comp.num_children; i++) {
			func_ptr(current_entity);
			ForEachChildRecursiveInternal(func_ptr, current_entity);
			current_entity = mp_registry->get<RelationshipComponent>(current_entity).next;
		}
	}

	void SceneEntity::ForEachLevelOneChild(std::function<void(entt::entity)> func_ptr) {
		auto& rel_comp = mp_registry->get<RelationshipComponent>(m_entt_handle);
		entt::entity current_entity = rel_comp.first;

		for (int i = 0; i < rel_comp.num_children; i++) {
			func_ptr(current_entity);
			current_entity = mp_registry->get<RelationshipComponent>(current_entity).next;
		}
	}


	void SceneEntity::ForEachChildRecursiveInternal(std::function<void(entt::entity)> func_ptr, entt::entity search_entity) {
		auto& rel_comp = mp_registry->get<RelationshipComponent>(search_entity);
		entt::entity current_entity = rel_comp.first;

		for (int i = 0; i < rel_comp.num_children; i++) {
			func_ptr(current_entity);
			ForEachChildRecursiveInternal(func_ptr, current_entity);
			current_entity = mp_registry->get<RelationshipComponent>(current_entity).next;
		}
	}


}