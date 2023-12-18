#pragma once
#include "rendering/VAO.h"
namespace ORNG {

	class GridMesh {
	public:
		friend class EditorLayer;
		void Init();
		void CheckBoundary(glm::vec3 camera_pos);

		float grid_step = 5.f;
		float grid_width = 400.0f;
	private:
		MeshVAO m_vao;
		SSBO<glm::mat4> m_transform_ssbo{ true, 0 };

		glm::vec2 m_center = glm::vec2(0); // center position on XZ plane
	};
}