#include <glew.h>
#include <glfw3.h>
#include "util/util.h"
#include "Terrain.h"
#include "WorldTransform.h"

static constexpr unsigned int WORLD_MAT_LOCATION_1 = 3;
static constexpr unsigned int WORLD_MAT_LOCATION_2 = 4;
static constexpr unsigned int WORLD_MAT_LOCATION_3 = 5;
static constexpr unsigned int WORLD_MAT_LOCATION_4 = 6;

Terrain::Terrain() : m_x_width(1000), m_z_width(1000), m_resolution(3.f) {};

void Terrain::UpdateTerrain(unsigned int seed, int width_x, int width_z, glm::fvec3 pos, int resolution, float height_scale, float amplitude) {
	m_x_width = width_x;
	m_z_width = width_z;
	m_center_pos = pos;
	m_resolution = resolution;
	m_height_scale = height_scale;
	m_amplitude = amplitude;

	m_terrain_data = TerrainGenerator::GenPerlinNoiseGrid(seed, m_x_width, m_z_width, pos, m_resolution, m_height_scale, m_amplitude);

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

	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_tangent_buffer));
	GLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(m_terrain_data.tangents[0]) * m_terrain_data.tangents.size(), &m_terrain_data.tangents[0], GL_STATIC_DRAW));
	GLCall(glEnableVertexAttribArray(7));
	GLCall(glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, 0, 0));

};

void Terrain::Init() {
	m_terrain_top_mat.diffuse_texture = std::make_unique<Texture2D>("./res/textures/missing_texture.jpeg");
	m_terrain_top_mat.normal_map_texture = std::make_unique<Texture2D>("./res/textures/terrain/forest-floor/forest_floor_Normal-ogl.png");
	m_terrain_top_mat.diffuse_texture->Load();
	m_terrain_top_mat.normal_map_texture->Load();
	m_terrain_top_mat.specular_color = glm::fvec3(0);
	m_terrain_top_mat.diffuse_color = glm::fvec3(1);

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_position_buffer);
	glGenBuffers(1, &m_tex_coord_buffer);
	glGenBuffers(1, &m_transform_buffer);
	glGenBuffers(1, &m_normal_buffer);
	glGenBuffers(1, &m_tangent_buffer);
	glBindVertexArray(m_vao);

	WorldTransform transform;
	m_transforms.emplace_back(transform.GetMatrix());

	UpdateTerrain(123, m_x_width, m_z_width, glm::fvec3(0, 0.0f, 0), m_resolution, 10.0f, 10.0f);

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

	glBindVertexArray(0);

}
