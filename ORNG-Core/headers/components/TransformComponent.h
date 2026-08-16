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

		lml::mat3 GetMatrix() const;
		lml::vec2 GetPosition() const;

	private:
		lml::vec2 m_scale = lml::vec2(1.0f, 1.0f);
		float m_rotation = 0.0f;
		lml::vec2 m_pos = lml::vec2(0.0f, 0.0f);
	};

	class TransformComponent final : public Component {
	public:
		friend class SceneSerializer;
		friend class EditorLayer;
		friend class SceneEntity;
		friend class TransformHierarchySystem;
		friend class PhysicsSystem;

		explicit TransformComponent(SceneEntity* p_entity = nullptr) : Component(p_entity) {}
		TransformComponent& operator=(const TransformComponent&) = default;
		TransformComponent(const TransformComponent&) = default;
		~TransformComponent() override = default;

		void SetScale(float scaleX, float scaleY, float scaleZ) {
			SetScale({ scaleX, scaleY, scaleZ });
		}

		void SetAbsoluteScale(lml::vec3 scale) {
			SetScale(scale / (m_abs_scale / m_scale));
		}

		inline void SetAbsolutePosition(lml::vec3 pos) {
			lml::vec3 final_pos = pos;
			if (GetParent() && !m_is_absolute) {
				final_pos = lml::inverse(GetParent()->GetMatrix()) * lml::vec4(pos, 1.0);
			}

			SetPosition(final_pos);
		}

		inline void SetAbsoluteOrientation(lml::vec3 orientation_degrees) {
			SetAbsOrientationQuat(lml::quat{lml::radians(orientation_degrees)});
		}

		inline void SetOrientation(float x, float y, float z) {
			lml::vec3 orientation{x, y, z};
			SetOrientation(orientation);
		}

		void SetOrientationQuat(lml::quat q) {
			m_orientation = q;
			RebuildMatrix(UpdateType::ORIENTATION);
		}

		void SetAbsOrientationQuat(lml::quat q) {
			if (m_parent_handle == entt::null) {
				SetOrientationQuat(q);
			} else {
				lml::quat parent_accumulated = m_abs_orientation * lml::inverse(m_orientation);
				lml::quat new_local = lml::inverse(parent_accumulated) * q;
				SetOrientationQuat(new_local);
			}
		}

		[[nodiscard]] lml::quat GetAbsOrientationQuat() const noexcept {
			return m_abs_orientation;
		}

		[[nodiscard]] lml::quat GetOrientationQuat() const noexcept {
			return m_orientation;
		}

		inline void SetPosition(float x, float y, float z) {
			lml::vec3 pos{x, y, z};
			SetPosition(pos);
		}

		bool IsAbsolute() {
			return m_is_absolute;
		}

		void LookAt(lml::vec3 t_pos, lml::vec3 t_up = { 0.0, 1.0,0.0 });

		inline void SetPosition(const lml::vec3 pos) {
			m_pos = pos;
			RebuildMatrix(UpdateType::TRANSLATION);
		}

		inline void SetScale(const lml::vec3 scale) {
			m_scale = lml::max(scale, lml::vec3(0.001f));
			RebuildMatrix(UpdateType::SCALE);
		}

		inline void SetOrientation(const lml::vec3 rot) {
			m_orientation = lml::quat{lml::radians(rot)};
			RebuildMatrix(UpdateType::ORIENTATION);
		}

		inline void SetAbsoluteMode(bool mode) {
			m_is_absolute = mode;
			RebuildMatrix(UpdateType::ALL);
		}

		inline lml::vec3 GetAbsPosition() {
			return m_abs_pos;
		}

		inline lml::vec3 GetAbsOrientation() {
			return lml::degrees(lml::eulerAngles(m_abs_orientation));
		}

		inline lml::vec3 GetAbsScale() {
			return m_abs_scale;
		}

		TransformComponent* GetParent();

		const lml::mat4& GetMatrix() const { return m_transform; }

		lml::vec3 GetPosition() const { return m_pos; }
		lml::vec3 GetScale() const { return m_scale; }
		lml::vec3 GetOrientation() const { return lml::degrees(lml::eulerAngles(m_orientation)); }


		enum UpdateType : uint8_t {
			TRANSLATION = 0,
			SCALE = 1,
			ORIENTATION = 2,
			ALL = 3
		};

		lml::vec3 forward = { 0.0, 0.0, -1.0 };
		lml::vec3 up = { 0.0, 1.0, 0.0 };
		lml::vec3 right = { 1.0, 0.0, 0.0 };

		void RebuildMatrix(UpdateType type);
	private:
		void UpdateAbsTransforms();

		entt::entity m_parent_handle = entt::null;
		// If true, transform will not take parent transforms into account when building matrix.
		bool m_is_absolute = false;

		lml::mat4 m_transform = lml::mat4(1);

		lml::vec3 m_scale = lml::vec3(1.0f, 1.0f, 1.0f);
		lml::quat m_orientation = {1.f, 0.f, 0.f, 0.f};
		lml::vec3 m_pos = lml::vec3(0.0f, 0.0f, 0.0f);

		lml::vec3 m_abs_scale = lml::vec3(1.0f, 1.0f, 1.0f);
		lml::quat m_abs_orientation = {1.f, 0.f, 0.f, 0.f};
		lml::vec3 m_abs_pos = lml::vec3(0.0f, 0.0f, 0.0f);

		lml::vec3 g_up = { 0.0, 1.0, 0.0 };

	};

}

#endif
