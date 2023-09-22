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
		glm::vec2 m_center = glm::vec2(0); // center position on XZ plane
		MeshVAO m_vao;
		uint32_t m_ssbo_handle;
	};
}