#include "GridMesh.h"
#include "util.h"
#include <glm/gtx/matrix_major_storage.hpp>
#include "WorldTransform.h"
#include <iostream>

static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;



void GridMesh::CheckBoundary(glm::fvec3 camera_pos) {
	if (abs(camera_pos.x - center_x) > 100.0f || abs(camera_pos.z - center_z) > 100.0f) {
		transform_array.clear();

		float rounded_z = glm::round(camera_pos.z / 100.f) * 100.f;
		float rounded_x = glm::round(camera_pos.x / 100.f) * 100.f;
		center_x = rounded_x;
		center_z = rounded_z;
		std::cout << "Z: " << rounded_z << std::endl;
		std::cout << "X: " << rounded_x << std::endl;
		std::cout << "CAMERA Z: " << camera_pos.z << std::endl;
		for (float z = rounded_z; z <= rounded_z + 500.0f; z += 10.0f) {
			WorldTransform transform;
			transform.SetPosition(rounded_x - 250.0f, 0.0f, z - 250.f);
			transform_array.push_back(glm::rowMajor4(transform.GetMatrix()));
			transform_array.push_back(transform.GetMatrix());

		}

		for (float x = rounded_x; x <= rounded_x + 500.0f; x += 10.0f) {
			WorldTransform transform;
			transform.SetPosition(x - 250.0f, 0.0f, rounded_z + 250.0f);
			transform.SetRotation(0.0f, 90.f, 0.f);
			transform_array.push_back(glm::rowMajor4(transform.GetMatrix()));
		}


		GLCall(glGenVertexArrays(1, &m_VAO));
		GLCall(glBindVertexArray(m_VAO));

		glGenBuffers(1, &m_vertex_buffer);
		glGenBuffers(1, &m_world_mat_buffer);

		glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::fvec3) * vertexArray.size(), &vertexArray[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_world_mat_buffer));
		GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(transform_array[0]) * transform_array.size(), &transform_array[0], GL_STATIC_DRAW));

		GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_1));
		GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0));
		GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_1, 1));

		GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_2));
		GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4))));
		GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_2, 1));

		GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_3));
		GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 2)));
		GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_3, 1));

		GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_4));
		GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 3)));
		GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_4, 1));


		GLCall(glBindVertexArray(0));

	}
}

void GridMesh::Init() {

	center_x = 0.f;
	center_z = 0.f;

	CheckBoundary(glm::fvec3(1000.f, 1000.f, 1000.f));

}

void GridMesh::Draw() {
	GLCall(glBindVertexArray(m_VAO));
	GLCall(glDrawArraysInstanced(GL_LINES, 0, 2, transform_array.size()));
	glBindVertexArray(0);
}