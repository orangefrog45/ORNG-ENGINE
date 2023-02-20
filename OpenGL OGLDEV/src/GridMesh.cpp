#include "GridMesh.h"
#include "util.h"
#include <glm/gtx/matrix_major_storage.hpp>
#include "WorldTransform.h"

static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;

void GridMesh::Init() {

	//might change to dynamically update with camera at some point

	for (float z = -500.0f; z <= 500.0f; z += 10.0f) {
		WorldTransform transform;
		transform.SetPosition(-250.0f, 0.0f, z);
		transform_array.push_back(glm::rowMajor4(transform.GetMatrix()));
		transform_array.push_back(transform.GetMatrix());

	}

	for (float x = 0.0f; x <= 500.0f; x += 10.0f) {
		WorldTransform transform;
		transform.SetPosition(x - 250.0f, 0.0f, 0.0f);
		transform.SetRotation(0.0f, 270.f, 0.f);
		transform_array.push_back(glm::rowMajor4(transform.GetMatrix()));
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

void GridMesh::Draw() {
	GLCall(glBindVertexArray(m_VAO));
	GLCall(glDrawArraysInstanced(GL_LINES, 0, 2, transform_array.size()));
	glBindVertexArray(0);
}