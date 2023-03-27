#include "TerrainGenerator.h"
#include "PerlinNoise.hpp"


TerrainGenerator::TerrainData TerrainGenerator::GenPerlinNoiseGrid(unsigned int seed, int width_x, int width_z, glm::fvec3 center, int resolution, float height_scale,
	float sampling_resolution) {
	TerrainGenerator::TerrainData terrain_data;

	const siv::PerlinNoise perlin{ seed };


	std::vector<glm::fvec3>& grid_verts = terrain_data.positions;
	std::vector<glm::fvec3>& grid_normals = terrain_data.normals;
	std::vector<glm::fvec2>& grid_tex_coords = terrain_data.tex_coords;

	grid_verts.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6); /* 6 vertices per quad (two triangles) of terrain */
	grid_normals.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6);
	grid_tex_coords.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6);

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

			const double noise_1 = perlin.noise2D_01(x_coord_1 / sampling_resolution, z_coord_1 / sampling_resolution)
				+ 0.5 * perlin.noise2D_01(x_coord_1 / (sampling_resolution * 0.5f), z_coord_1 / (sampling_resolution * 0.5f))
				+ 0.25 * perlin.noise2D_01(x_coord_1 / (sampling_resolution * 0.25f), z_coord_1 / (sampling_resolution * 0.25f));

			const double noise_2 = perlin.noise2D_01(x_coord_1 / sampling_resolution, z_coord_2 / sampling_resolution)
				+ 0.5 * perlin.noise2D_01(x_coord_1 / (sampling_resolution * 0.5f), z_coord_2 / (sampling_resolution * 0.5f))
				+ 0.25 * perlin.noise2D_01(x_coord_1 / (sampling_resolution * 0.25f), z_coord_2 / (sampling_resolution * 0.25f));

			const double noise_3 = perlin.noise2D_01(x_coord_2 / sampling_resolution, z_coord_1 / sampling_resolution)
				+ 0.5 * perlin.noise2D_01(x_coord_2 / (sampling_resolution * 0.5f), z_coord_1 / (sampling_resolution * 0.5f))
				+ 0.25 * perlin.noise2D_01(x_coord_2 / (sampling_resolution * 0.25f), z_coord_1 / (sampling_resolution * 0.25f));

			const double noise_4 = perlin.noise2D_01(x_coord_2 / sampling_resolution, z_coord_2 / sampling_resolution)
				+ 0.5 * perlin.noise2D_01(x_coord_2 / (sampling_resolution * 0.5f), z_coord_2 / (sampling_resolution * 0.5f))
				+ 0.25 * perlin.noise2D_01(x_coord_2 / (sampling_resolution * 0.25f), z_coord_2 / (sampling_resolution * 0.25f));

			//BL
			verts.vert_1.y += noise_1 * height_scale;
			//TL
			verts.vert_2.y += noise_2 * height_scale;
			//BR
			verts.vert_3.y += noise_3 * height_scale;
			//TR
			verts.vert_4.y += noise_4 * height_scale;

			//TRI 1
			grid_verts.push_back(verts.vert_1);
			grid_verts.push_back(verts.vert_2);
			grid_verts.push_back(verts.vert_3);
			grid_tex_coords.push_back(glm::fvec2(0, 0));
			grid_tex_coords.push_back(glm::fvec2(0, 1));
			grid_tex_coords.push_back(glm::fvec2(1, 0));
			glm::fvec3 normal_1 = glm::cross(verts.vert_2 - verts.vert_1, verts.vert_3 - verts.vert_1);
			grid_normals.push_back(normal_1);
			grid_normals.push_back(normal_1);
			grid_normals.push_back(normal_1);


			//TRI 2
			grid_verts.push_back(verts.vert_4);
			grid_verts.push_back(verts.vert_3);
			grid_verts.push_back(verts.vert_2);
			grid_tex_coords.push_back(glm::fvec2(1, 1));
			grid_tex_coords.push_back(glm::fvec2(1, 0));
			grid_tex_coords.push_back(glm::fvec2(0, 1));
			glm::fvec3 normal_2 = glm::cross(verts.vert_3 - verts.vert_2, verts.vert_2 - verts.vert_4);
			grid_normals.push_back(normal_2);
			grid_normals.push_back(normal_2);
			grid_normals.push_back(normal_2);

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
			grid_tex_coords.push_back(glm::fvec2(0, 0));
			grid_tex_coords.push_back(glm::fvec2(0, 1));
			grid_tex_coords.push_back(glm::fvec2(1, 0));
			glm::fvec3 normal_1 = glm::cross(verts.vert_2 - verts.vert_1, verts.vert_3 - verts.vert_1);
			grid_normals.push_back(normal_1);
			grid_normals.push_back(normal_1);
			grid_normals.push_back(normal_1);


			//TRI 2
			grid_verts.push_back(verts.vert_4);
			grid_verts.push_back(verts.vert_3);
			grid_verts.push_back(verts.vert_2);
			grid_tex_coords.push_back(glm::fvec2(1, 1));
			grid_tex_coords.push_back(glm::fvec2(1, 0));
			grid_tex_coords.push_back(glm::fvec2(0, 1));
			glm::fvec3 normal_2 = glm::cross(verts.vert_3 - verts.vert_2, verts.vert_2 - verts.vert_4);
			grid_normals.push_back(normal_2);
			grid_normals.push_back(normal_2);
			grid_normals.push_back(normal_2);

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

	return verts;
};