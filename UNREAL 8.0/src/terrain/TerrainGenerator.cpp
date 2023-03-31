#include "TerrainGenerator.h"
#include "PerlinNoise.hpp"
#include "glm/gtc/round.hpp"


TerrainGenerator::TerrainData TerrainGenerator::GenPerlinNoiseGrid(unsigned int seed, int width_x, int width_z, glm::fvec3 center, int resolution, float height_scale,
	float sampling_density) {
	TerrainGenerator::TerrainData terrain_data;

	const siv::PerlinNoise perlin{ seed };


	terrain_data.positions.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6); /* 6 vertices per quad (two triangles) of terrain */
	terrain_data.normals.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6);
	terrain_data.tangents.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6);
	terrain_data.tex_coords.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6);

	const float x_start = -(width_x / 2.0f) + center.x;
	const float z_start = -(width_z / 2.0f) + center.z;

	const float x_end = (width_x / 2.0f) + center.x;
	const float z_end = (width_z / 2.0f) + center.z;

	for (int x = x_start; x <= x_end; x += resolution) {
		for (int z = z_start; z <= z_end; z += resolution) {
			TerrainGenerator::QuadVertices verts = GenQuad(resolution, glm::fvec3(x, center.y, z));

			int adjusted_x = x + width_x / 2.0f - center.x;
			int adjusted_z = z + width_x / 2.0f - center.z;

			double x_coord_1 = (adjusted_x / resolution) * resolution;
			double x_coord_2 = (adjusted_x / resolution + 1) * resolution;
			double z_coord_1 = (adjusted_z / resolution) * resolution;
			double z_coord_2 = (adjusted_z / resolution + 1) * resolution;

			const double noise_1 = perlin.noise2D_01(x_coord_1 / sampling_density, z_coord_1 / sampling_density)
				+ 0.5 * perlin.noise2D_01(x_coord_1 / (sampling_density * 0.5f), z_coord_1 / (sampling_density * 0.5f))
				+ 0.25 * perlin.noise2D_01(x_coord_1 / (sampling_density * 0.25f), z_coord_1 / (sampling_density * 0.25f));

			const double noise_2 = perlin.noise2D_01(x_coord_1 / sampling_density, z_coord_2 / sampling_density)
				+ 0.5 * perlin.noise2D_01(x_coord_1 / (sampling_density * 0.5f), z_coord_2 / (sampling_density * 0.5f))
				+ 0.25 * perlin.noise2D_01(x_coord_1 / (sampling_density * 0.25f), z_coord_2 / (sampling_density * 0.25f));

			const double noise_3 = perlin.noise2D_01(x_coord_2 / sampling_density, z_coord_1 / sampling_density)
				+ 0.5 * perlin.noise2D_01(x_coord_2 / (sampling_density * 0.5f), z_coord_1 / (sampling_density * 0.5f))
				+ 0.25 * perlin.noise2D_01(x_coord_2 / (sampling_density * 0.25f), z_coord_1 / (sampling_density * 0.25f));

			const double noise_4 = perlin.noise2D_01(x_coord_2 / sampling_density, z_coord_2 / sampling_density)
				+ 0.5 * perlin.noise2D_01(x_coord_2 / (sampling_density * 0.5f), z_coord_2 / (sampling_density * 0.5f))
				+ 0.25 * perlin.noise2D_01(x_coord_2 / (sampling_density * 0.25f), z_coord_2 / (sampling_density * 0.25f));

			const float height_exponent = 1.0f;

			/*Rounding will cause shelves*/
			//BL
			verts.vert_1.y += glm::ceilMultiple((noise_1 / 1.75) * glm::pow(height_scale, height_exponent), 1.0);
			//TL
			verts.vert_2.y += glm::ceilMultiple((noise_2 / 1.75) * glm::pow(height_scale, height_exponent), 1.0);
			//BR
			verts.vert_3.y += glm::ceilMultiple((noise_3 / 1.75) * glm::pow(height_scale, height_exponent), 1.0);
			//TR
			verts.vert_4.y += glm::ceilMultiple((noise_4 / 1.75) * glm::pow(height_scale, height_exponent), 1.0);

			//TRI 1
			terrain_data.positions.push_back(verts.vert_1);
			terrain_data.positions.push_back(verts.vert_2);
			terrain_data.positions.push_back(verts.vert_3);
			terrain_data.tex_coords.push_back(verts.tex_coord_1);
			terrain_data.tex_coords.push_back(verts.tex_coord_2);
			terrain_data.tex_coords.push_back(verts.tex_coord_3);

			glm::vec3 edge1 = verts.vert_2 - verts.vert_1;
			glm::vec3 edge2 = verts.vert_3 - verts.vert_1;
			glm::vec2 delta_uv1 = verts.tex_coord_2 - verts.tex_coord_1;
			glm::vec2 delta_uv2 = verts.tex_coord_3 - verts.tex_coord_1;

			float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);

			verts.tangent_1.x = f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
			verts.tangent_1.y = f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
			verts.tangent_1.z = f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);

			verts.normal_1 = glm::cross(verts.vert_2 - verts.vert_1, verts.vert_3 - verts.vert_1);

			terrain_data.normals.push_back(verts.normal_1);
			terrain_data.normals.push_back(verts.normal_1);
			terrain_data.normals.push_back(verts.normal_1);
			terrain_data.tangents.push_back(verts.tangent_1);
			terrain_data.tangents.push_back(verts.tangent_1);
			terrain_data.tangents.push_back(verts.tangent_1);

			//TRI 2
			terrain_data.positions.push_back(verts.vert_4);
			terrain_data.positions.push_back(verts.vert_3);
			terrain_data.positions.push_back(verts.vert_2);
			terrain_data.tex_coords.push_back(verts.tex_coord_4);
			terrain_data.tex_coords.push_back(verts.tex_coord_3);
			terrain_data.tex_coords.push_back(verts.tex_coord_2);

			glm::vec3 edge3 = verts.vert_3 - verts.vert_2;
			glm::vec3 edge4 = verts.vert_4 - verts.vert_2;
			glm::vec2 delta_uv3 = verts.tex_coord_3 - verts.tex_coord_2;
			glm::vec2 delta_uv4 = verts.tex_coord_4 - verts.tex_coord_2;

			float f2 = 1.0f / (delta_uv3.x * delta_uv4.y - delta_uv4.x * delta_uv3.y);

			verts.tangent_2.x = f2 * (delta_uv4.y * edge3.x - delta_uv3.y * edge4.x);
			verts.tangent_2.y = f2 * (delta_uv4.y * edge3.y - delta_uv3.y * edge4.y);
			verts.tangent_2.z = f2 * (delta_uv4.y * edge3.z - delta_uv3.y * edge4.z);

			verts.normal_2 = glm::cross(verts.vert_3 - verts.vert_2, verts.vert_2 - verts.vert_4);
			terrain_data.normals.push_back(verts.normal_2);
			terrain_data.normals.push_back(verts.normal_2);
			terrain_data.normals.push_back(verts.normal_2);
			terrain_data.tangents.push_back(verts.tangent_2);
			terrain_data.tangents.push_back(verts.tangent_2);
			terrain_data.tangents.push_back(verts.tangent_2);

		}

	}

	return terrain_data;
};

TerrainGenerator::TerrainData TerrainGenerator::GenWavyGrid(int width_x, int width_z, glm::fvec3 center, int resolution, float height_range, float amplitude) {
	TerrainGenerator::TerrainData terrain_data;
	float angle = 0;
	float angle_h = 0;
	/*static float amplitude_offset = 0.0f;
	static float coefficient = 1.0f;
	if (amplitude_offset > 10.0f) coefficient = -1.0f;
	if (amplitude_offset < 10.0f) coefficient = 1.0f;
	amplitude_offset += 0.0005f * coefficient;
	amplitude = amplitude_offset;*/
	std::vector<glm::fvec3>& grid_verts = terrain_data.positions;
	std::vector<glm::fvec3>& grid_normals = terrain_data.normals;
	std::vector<glm::fvec2>& grid_tex_coords = terrain_data.tex_coords;

	grid_verts.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6); /* 6 vertices per quad (two triangles) of terrain */
	grid_normals.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6);
	grid_tex_coords.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6);

	float x_start = -(width_x / 2.0f) + center.x;
	float z_start = -(width_z / 2.0f) + center.z;

	float x_end = (width_x / 2.0f) + center.x;
	float z_end = (width_z / 2.0f) + center.z;


	for (int x = x_start; x <= x_end; x += resolution) {
		for (int z = z_start; z <= z_end; z += resolution) {
			TerrainGenerator::QuadVertices verts = GenQuad(resolution, glm::fvec3(x, center.y, z));

			//BL
			verts.vert_1.y += (sinf(glm::radians(angle)) + cosf(glm::radians(angle_h))) * height_range;
			//TL
			verts.vert_2.y += (sinf(glm::radians(angle)) + cosf(glm::radians(angle_h + resolution * amplitude))) * height_range;
			//BR
			verts.vert_3.y += (sinf(glm::radians(angle + resolution * amplitude)) + cosf(glm::radians(angle_h))) * height_range;
			//TR
			verts.vert_4.y += (sinf(glm::radians(angle + resolution * amplitude)) + cosf(glm::radians(angle_h + resolution * amplitude))) * height_range;

			//TRI 1
			grid_verts.push_back(verts.vert_1);
			grid_verts.push_back(verts.vert_2);
			grid_verts.push_back(verts.vert_3);
			grid_tex_coords.push_back(verts.tex_coord_1);
			grid_tex_coords.push_back(verts.tex_coord_2);
			grid_tex_coords.push_back(verts.tex_coord_3);
			grid_normals.push_back(verts.normal_1);
			grid_normals.push_back(verts.normal_1);
			grid_normals.push_back(verts.normal_1);


			//TRI 2
			grid_verts.push_back(verts.vert_4);
			grid_verts.push_back(verts.vert_3);
			grid_verts.push_back(verts.vert_2);
			grid_tex_coords.push_back(verts.tex_coord_4);
			grid_tex_coords.push_back(verts.tex_coord_3);
			grid_tex_coords.push_back(verts.tex_coord_2);
			grid_normals.push_back(verts.normal_2);
			grid_normals.push_back(verts.normal_2);
			grid_normals.push_back(verts.normal_2);

			angle_h += resolution * amplitude;
		}
		angle_h = 0.0f;
		angle += resolution * amplitude;
	}

	return terrain_data;
}

TerrainGenerator::TriangleVertices TerrainGenerator::GenTriangle(bool flipped, float size, glm::fvec3 pos) {
	TerrainGenerator::TriangleVertices verts;

	/* Triangle created around top right vertex facing left & down if flipped, else created around bottom left vertex facing up & right*/
	if (flipped) {
		verts.vert_1 = glm::fvec3(pos.x, pos.y, pos.z);
		verts.vert_2 = glm::fvec3(pos.x, pos.y, pos.z - size);
		verts.vert_3 = glm::fvec3(pos.x - size, pos.y, pos.z);
	}
	else {
		verts.vert_1 = glm::fvec3(pos.x, pos.y, pos.z);
		verts.vert_2 = glm::fvec3(pos.x, pos.y, pos.z + size);
		verts.vert_3 = glm::fvec3(pos.x + size, pos.y, pos.z);
	}

	return verts;
}

TerrainGenerator::QuadVertices TerrainGenerator::GenQuad(float size, glm::fvec3 bot_left_vert_pos) {
	TerrainGenerator::QuadVertices verts;

	verts.vert_1 = glm::fvec3(bot_left_vert_pos.x, bot_left_vert_pos.y, bot_left_vert_pos.z);
	verts.vert_2 = glm::fvec3(bot_left_vert_pos.x, bot_left_vert_pos.y, bot_left_vert_pos.z + size);
	verts.vert_3 = glm::fvec3(bot_left_vert_pos.x + size, bot_left_vert_pos.y, bot_left_vert_pos.z);
	verts.vert_4 = glm::fvec3(bot_left_vert_pos.x + size, bot_left_vert_pos.y, bot_left_vert_pos.z + size);

	//TRI 1
	glm::vec3 edge1 = verts.vert_2 - verts.vert_1;
	glm::vec3 edge2 = verts.vert_3 - verts.vert_1;
	glm::vec2 delta_uv1 = verts.tex_coord_2 - verts.tex_coord_1;
	glm::vec2 delta_uv2 = verts.tex_coord_3 - verts.tex_coord_1;

	float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);

	verts.tangent_1.x = f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
	verts.tangent_1.y = f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
	verts.tangent_1.z = f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);

	verts.normal_1 = glm::cross(verts.vert_2 - verts.vert_1, verts.vert_3 - verts.vert_1);

	//TRI 2
	glm::vec3 edge3 = verts.vert_3 - verts.vert_2;
	glm::vec3 edge4 = verts.vert_4 - verts.vert_2;
	glm::vec2 delta_uv3 = verts.tex_coord_3 - verts.tex_coord_2;
	glm::vec2 delta_uv4 = verts.tex_coord_4 - verts.tex_coord_2;

	float f2 = 1.0f / (delta_uv3.x * delta_uv4.y - delta_uv4.x * delta_uv3.y);

	verts.tangent_2.x = f2 * (delta_uv4.y * edge3.x - delta_uv3.y * edge4.x);
	verts.tangent_2.y = f2 * (delta_uv4.y * edge3.y - delta_uv3.y * edge4.y);
	verts.tangent_2.z = f2 * (delta_uv4.y * edge3.z - delta_uv3.y * edge4.z);

	verts.normal_2 = glm::cross(verts.vert_3 - verts.vert_2, verts.vert_4 - verts.vert_2);

	return verts;
};