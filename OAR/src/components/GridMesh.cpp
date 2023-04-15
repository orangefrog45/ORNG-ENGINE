#include "GridMesh.h"
#include "util/util.h"
#include <glm/gtx/matrix_major_storage.hpp>
#include "WorldTransform.h"
#include <iostream>

static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;

void GridMesh::CheckBoundary(glm::fvec3 camera_pos)
{
	if (abs(camera_pos.x - salt_center_x) > salt_grid_width / 5 || abs(camera_pos.z - salt_center_z) > salt_grid_width / 5)
	{
		transform_array.clear();

		float rounded_z = glm::round(camera_pos.z / 100.f) * 100.f;
		float rounded_x = glm::round(camera_pos.x / 100.f) * 100.f;
		salt_center_x = rounded_x;
		salt_center_z = rounded_z;
		for (float z = rounded_z; z <= rounded_z + salt_grid_width; z += 10.0f)
		{
			WorldTransform transform;
			transform.SetPosition(rounded_x - salt_grid_width / 2, 0.0f, z - salt_grid_width / 2);
			transform_array.push_back(glm::rowMajor4(transform.GetMatrix()));
			transform_array.push_back(transform.GetMatrix());
		}

		for (float x = rounded_x; x <= rounded_x + salt_grid_width; x += 10.0f)
		{
			WorldTransform transform;
			transform.SetPosition(x - salt_grid_width / 2, 0.0f, rounded_z + salt_grid_width / 2);
			transform.SetRotation(0.0f, 90.f, 0.f);
			transform_array.push_back(glm::rowMajor4(transform.GetMatrix()));
		}

		GLCall(glGenVertexArrays(1, &salt_m_VAO));
		GLCall(glBindVertexArray(salt_m_VAO));

		glGenBuffers(1, &salt_m_vertex_buffer);
		glGenBuffers(1, &salt_m_world_mat_buffer);

		glBindBuffer(GL_ARRAY_BUFFER, salt_m_vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::fvec3) * salt_vertexArray.size(), &salt_vertexArray[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		GLCall(glBindBuffer(GL_ARRAY_BUFFER, salt_m_world_mat_buffer));
		GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(transform_array[0]) * transform_array.size(), &transform_array[0], GL_STATIC_DRAW));

		GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_1));
		GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)0));
		GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_1, 1));

		GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_2));
		GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(glm::vec4))));
		GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_2, 1));

		GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_3));
		GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(glm::vec4) * 2)));
		GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_3, 1));

		GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_4));
		GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(sizeof(glm::vec4) * 3)));
		GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_4, 1));

		GLCall(glBindVertexArray(0));
	}
}

void GridMesh::Init()
{

	salt_center_x = 0.f;
	salt_center_z = 0.f;

	CheckBoundary(glm::fvec3(1000.f, 1000.f, 1000.f));
}

void GridMesh::Draw()
{
	GLCall(glBindVertexArray(salt_m_VAO));
	GLCall(glDrawArraysInstanced(GL_LINES, 0, 2, transform_array.size()));
	glBindVertexArray(0);
}