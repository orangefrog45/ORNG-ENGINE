#include "pch/pch.h"

#include "scene/GridMesh.h"
#include "util/util.h"
#include "components/WorldTransform.h"

static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;


namespace ORNG {

	void GridMesh::CheckBoundary(glm::vec3 camera_pos) {
		if (abs(camera_pos.x - center_x) > grid_width / 5 || abs(camera_pos.z - center_z) > grid_width / 5) {
			std::vector<glm::mat4> transforms;

			float rounded_z = glm::round(camera_pos.z / 100.f) * 100.f;
			float rounded_x = glm::round(camera_pos.x / 100.f) * 100.f;
			center_x = rounded_x;
			center_z = rounded_z;

			WorldTransform transform;
			for (float z = rounded_z; z <= rounded_z + grid_width; z += 10.0f) {
				transform.SetPosition(rounded_x - grid_width / 2, 0.0f, z - grid_width / 2);
				transforms.push_back(glm::rowMajor4(transform.GetMatrix()));
				transforms.push_back(transform.GetMatrix());

			}

			transform.SetOrientation(0.0f, 90.f, 0.f);
			for (float x = rounded_x; x <= rounded_x + grid_width; x += 10.0f) {
				transform.SetPosition(x - grid_width / 2, 0.0f, rounded_z + grid_width / 2);
				transforms.push_back(glm::rowMajor4(transform.GetMatrix()));
			}

			m_vao.SubUpdateTransformSSBO(m_ssbo_handle, 0, transforms);
		}
	}

	void GridMesh::Init() {

		center_x = 0.f;
		center_z = 0.f;

		CheckBoundary(glm::vec3(1000.f, 1000.f, 1000.f));

		m_vao.vertex_data.positions = { glm::vec3(0.0f, 0.0f, 0.0f),
										glm::vec3(grid_width, 0.0f, 0.0f) };
		m_vao.FillBuffers();

		m_ssbo_handle = m_vao.GenTransformSSBO();
		m_vao.FullUpdateTransformSSBO(m_ssbo_handle, nullptr, sizeof(glm::mat4) * grid_width * 2);

	}


}