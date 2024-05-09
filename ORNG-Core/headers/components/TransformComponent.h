#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include "Component.h"
#include "util/ExtraMath.h"

namespace ORNG {

	class TransformComponent2D {
	public:

		void SetScale(float x, float y);
		void SetOrientation(float rot);
		void SetPosition(float x, float y);

		glm::mat3 GetMatrix() const;
		glm::vec2 GetPosition() const;

	private:
		glm::vec2 m_scale = glm::vec2(1.0f, 1.0f);
		float m_rotation = 0.0f;
		glm::vec2 m_pos = glm::vec2(0.0f, 0.0f);
	};

	class TransformComponent : public Component
	{
	public:
		friend class SceneSerializer;
		friend class EditorLayer;
		friend class SceneEntity;
		friend class TransformHierarchySystem;
		friend class PhysicsSystem;

		TransformComponent(SceneEntity* p_entity = nullptr) : Component(p_entity) {};


		void SetScale(float scaleX, float scaleY, float scaleZ) {
			SetScale({ scaleX, scaleY, scaleZ });
		}

		void SetAbsoluteScale(glm::vec3 scale) {
			SetScale(scale / (m_abs_scale / m_scale));
		}

		inline void SetAbsolutePosition(glm::vec3 pos) {
			glm::vec3 final_pos = pos;
			if (GetParent() && !m_is_absolute) {
				final_pos = glm::inverse(GetParent()->GetMatrix()) * glm::vec4(pos, 1.0);
			}

			SetPosition(final_pos);
		}

		inline void SetAbsoluteOrientation(glm::vec3 orientation) {
			SetOrientation(orientation - (m_abs_orientation - m_orientation));
		}

		inline void SetOrientation(float x, float y, float z) {
			glm::vec3 orientation{x, y, z};
			SetOrientation(orientation);
		}
		inline void SetPosition(float x, float y, float z) {
			glm::vec3 pos{x, y, z};
			SetPosition(pos);
		}

		bool IsAbsolute() {
			return m_is_absolute;
		}

		void LookAt(glm::vec3 t_pos, glm::vec3 t_up = { 0.0, 1.0,0.0 });

		inline void SetPosition(const glm::vec3 pos) {
			m_pos = pos;
			RebuildMatrix(UpdateType::TRANSLATION);
		};

		inline void SetScale(const glm::vec3 scale) {
			m_scale = glm::max(scale, glm::vec3(0.001f));
			RebuildMatrix(UpdateType::SCALE);
		};

		inline void SetOrientation(const glm::vec3 rot) {
			m_orientation = rot;
			RebuildMatrix(UpdateType::ORIENTATION);
		};

		inline void SetAbsoluteMode(bool mode) {
			m_is_absolute = mode;
			RebuildMatrix(UpdateType::ALL);
		}

		inline glm::vec3 GetAbsPosition() {
			return m_abs_pos;
		}

		inline glm::vec3 GetAbsOrientation() {
			return m_abs_orientation;
		}

		inline glm::vec3 GetAbsScale() {
			return m_abs_scale;
		}

		// Returns inherited position([0]), scale([1]), rotation ([2]) including this components transforms.
		std::tuple<glm::vec3, glm::vec3, glm::vec3> GetAbsoluteTransforms() { return std::make_tuple(m_abs_pos, m_abs_scale, m_abs_orientation); }

		TransformComponent* GetParent();

		const glm::mat4x4& GetMatrix() const { return m_transform; };


		glm::vec3 GetPosition() const { return m_pos; };
		glm::vec3 GetScale() const { return m_scale; };
		glm::vec3 GetOrientation() const { return m_orientation; };


		enum UpdateType : uint8_t {
			TRANSLATION = 0,
			SCALE = 1,
			ORIENTATION = 2,
			ALL = 3
		};


		glm::vec3 forward = { 0.0, 0.0, -1.0 };
		glm::vec3 up = { 0.0, 1.0, 0.0 };
		glm::vec3 right = { 1.0, 0.0, 0.0 };

		void RebuildMatrix(UpdateType type);
	private:
		void UpdateAbsTransforms();

		entt::entity m_parent_handle = entt::null;
		// If true, transform will not take parent transforms into account when building matrix.
		bool m_is_absolute = false;


		glm::mat4 m_transform = glm::mat4(1);

		glm::vec3 m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 m_orientation = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 m_pos = glm::vec3(0.0f, 0.0f, 0.0f);

		glm::vec3 m_abs_scale = glm::vec3(1.0f, 1.0f, 1.0f);
		glm::vec3 m_abs_orientation = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 m_abs_pos = glm::vec3(0.0f, 0.0f, 0.0f);

		glm::vec3 g_up = { 0.0, 1.0, 0.0 };

	};

}

#endif