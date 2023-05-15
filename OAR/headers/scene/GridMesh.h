#pragma once
#include "rendering/VAO.h"
namespace ORNG {

	class GridMesh {
	public:
		void Init();
		void CheckBoundary(glm::vec3 camera_pos);
	private:
		float grid_width = 400.0f;
		float center_x;
		float center_z;
		VAO m_vao;
		unsigned int m_ssbo_handle;
		std::vector<float> vertexArray = { 0.0f, 0.0f, 0.0f,
										   grid_width, 0.0f, 0.0f };

	};
}