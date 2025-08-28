#include "pch/pch.h"

#include "scene/GridMesh.h"
#include "components/TransformComponent.h"

namespace ORNG {
	void GridMesh::CheckBoundary(glm::vec3 camera_pos) {
		auto& transforms = m_transform_ssbo.data;
		transforms.clear();

		if (abs(camera_pos.x - m_center.x) > grid_width / 5.f || abs(camera_pos.z - m_center.y) > grid_width / 5.f) {

			float rounded_z = glm::round(camera_pos.z / 100.f) * 100.f;
			float rounded_x = glm::round(camera_pos.x / 100.f) * 100.f;
			m_center.x = rounded_x;
			m_center.y = rounded_z;

			TransformComponent transform;
			for (float z = rounded_z; z <= rounded_z + grid_width; z += grid_step) {
				transform.SetPosition(rounded_x - grid_width / 2, 0.0f, z - grid_width / 2);
				transforms.push_back(transform.GetMatrix());

			}

			transform.SetOrientation(0.0f, 90.f, 0.f);
			for (float x = rounded_x; x <= rounded_x + grid_width; x += grid_step) {
				transform.SetPosition(x - grid_width / 2, 0.0f, rounded_z + grid_width / 2);
				transforms.push_back(transform.GetMatrix());
			}

			m_transform_ssbo.FillBuffer();
		}
	}

	void GridMesh::Init() {

		m_vao.vertex_data.positions = { 0.0f, 0.0f, 0.0f,
										grid_width, 0.0f, 0.0f };
		m_vao.FillBuffers();
		m_transform_ssbo.Init();
		m_transform_ssbo.data_type = GL_FLOAT;
		m_transform_ssbo.draw_type = GL_DYNAMIC_DRAW;

		// Trick just to get it to load
		CheckBoundary({ 1000, 0, 1000 });
		CheckBoundary({ 0, 0, 0 });
	}
}
