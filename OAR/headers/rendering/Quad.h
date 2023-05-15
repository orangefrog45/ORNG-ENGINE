#pragma once
#include "components/WorldTransform.h"
#include "VAO.h"

namespace ORNG {

	class Quad {
	public:
		friend class Renderer;
		void Load();
		inline void SetScale(float x, float y) { m_world_transform.SetScale(x, y); };
		inline void SetPosition(float x, float y) { m_world_transform.SetPosition(x, y); }
		inline void SetRotation(float rot) { m_world_transform.SetOrientation(rot); }
		const WorldTransform2D& GetTransform() const { return m_world_transform; }


	private:
		WorldTransform2D m_world_transform;
		VAO m_vao;
	};
}