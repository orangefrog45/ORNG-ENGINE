#pragma once
#ifndef ENTITY_NODE_REF
#define ENTITY_NODE_REF

namespace ORNG {
	class SceneEntity;

	// Represents a relative "path" to traverse an entity hierarchy starting from entity "p_src" passed to constructor
	class EntityNodeRef {
		friend class SceneSerializer;
		friend class Scene;
	public:
		EntityNodeRef(SceneEntity* p_src, std::vector<std::string>& instructions) : mp_src(p_src), m_instructions(std::move(instructions)) {}
		EntityNodeRef(SceneEntity* p_src) : mp_src(p_src) {}

		// Path should work like a filepath where entity/entity_1 searches for src_entity -> entity (child of src_entity) -> entity_1 (child of entity_1)
		// ../ traverses up the hierarchy, e.g ../../ searches for src_entity -> src_entity_parent -> src_entity_parent_parent
		// If path starts with '::/' it is absolute
		void GenAndCacheInstructionsFromStringPath(const std::string& path) {
			std::string current_ent_name = "";

			for (size_t i = 0; i < path.size(); i++) {
				if (path[i] == '/') {
					m_instructions.push_back(current_ent_name);
					current_ent_name.clear();
				}
				else {
					current_ent_name += path[i];
				}
			}

			if (!current_ent_name.empty()) {
				if (current_ent_name.ends_with('/'))
					current_ent_name.pop_back();

				m_instructions.push_back(current_ent_name);
			}

		}

		std::string GenStringPathFromInstructions();

		const auto& GetInstructions() const noexcept { return m_instructions; }

		SceneEntity* GetSrc() const { return mp_src; }

	private:
		// The entity this reference is relative to
		SceneEntity* mp_src = nullptr;

		std::vector<std::string> m_instructions;
	};
}

#endif
