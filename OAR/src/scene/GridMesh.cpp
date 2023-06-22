#include "pch/pch.h"

#include "scene/GridMesh.h"
#include "util/util.h"
#include "components/TransformComponent.h"

static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;


namespace ORNG {

	void GridMesh::CheckBoundary(glm::vec3 camera_pos) {
		if (abs(camera_pos.x - m_center.x) > grid_width / 5.f || abs(camera_pos.z - m_center.y) > grid_width / 5.f) {
			std::vector<glm::mat4> transforms;

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

			m_vao.FullUpdateTransformSSBO(m_ssbo_handle, &transforms, sizeof(glm::mat4) * transforms.size());


		}
	}

	void GridMesh::Init() {

		m_vao.vertex_data.positions = { glm::vec3(0.0f, 0.0f, 0.0f),
										glm::vec3(grid_width, 0.0f, 0.0f) };
		m_vao.FillBuffers();

		m_ssbo_handle = m_vao.GenTransformSSBO();
		m_vao.FullUpdateTransformSSBO(m_ssbo_handle, nullptr, sizeof(glm::mat4) * ceil(grid_width * grid_step) * 2);

	}


}