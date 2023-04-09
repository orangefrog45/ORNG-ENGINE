#include "TerrainChunk.h"
#include <glew.h>
#include <glfw3.h>
#include "util/util.h"

static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;

void TerrainChunk::Init() {

	GLCall(glGenVertexArrays(1, &m_vao));
	GLCall(glGenBuffers(1, &m_position_buffer));
	GLCall(glGenBuffers(1, &m_tex_coord_buffer));
	GLCall(glGenBuffers(1, &m_normal_buffer));
	GLCall(glGenBuffers(1, &m_tangent_buffer));
	GLCall(glGenBuffers(1, &m_transform_buffer));
	GLCall(glGenBuffers(1, &m_index_buffer));

	GLCall(glBindVertexArray(m_vao));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_position_buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_terrain_data.positions[0]) * m_terrain_data.positions.size(), &m_terrain_data.positions[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(0));
	GLCall(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_tex_coord_buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_terrain_data.tex_coords[0]) * m_terrain_data.tex_coords.size(), &m_terrain_data.tex_coords[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(1));
	GLCall(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_normal_buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_terrain_data.normals[0]) * m_terrain_data.normals.size(), &m_terrain_data.normals[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(2));
	GLCall(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0));

	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_index_buffer));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_terrain_data.indices[0]) * m_terrain_data.indices.size(), &m_terrain_data.indices[0], GL_STATIC_DRAW));

	m_transforms.emplace_back(1.0f);

	/* Attrib divisor is int max as the grid does not need to be transformed to world coordinates as it is already created in them, placeholder identity matrix
inserted for lighting shader compatibility */
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_transform_buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_transforms[0]) * m_transforms.size(), &m_transforms[0], GL_STATIC_DRAW));

	GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_1));
	GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0));
	GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_1, INT_MAX));

	GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_2));
	GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4))));
	GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_2, INT_MAX));

	GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_3));
	GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 2)));
	GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_3, INT_MAX));

	GLCall(glEnableVertexAttribArray(WORLD_MAT_LOCATION_4));
	GLCall(glVertexAttribPointer(WORLD_MAT_LOCATION_4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * 3)));
	GLCall(glVertexAttribDivisor(WORLD_MAT_LOCATION_4, INT_MAX));


	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_tangent_buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_terrain_data.tangents[0]) * m_terrain_data.tangents.size(), &m_terrain_data.tangents[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(7));
	GLCall(glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 0, 0));



	GLCall(glBindVertexArray(0));

	m_is_initialized = true;
}

TerrainChunk::~TerrainChunk() {

	glDeleteBuffers(1, &m_position_buffer);
	glDeleteBuffers(1, &m_tex_coord_buffer);
	glDeleteBuffers(1, &m_normal_buffer);
	glDeleteBuffers(1, &m_tangent_buffer);
	glDeleteBuffers(1, &m_transform_buffer);
	glDeleteBuffers(1, &m_index_buffer);
	glDeleteVertexArrays(1, &m_vao);
}