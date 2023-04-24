#include "pch/pch.h"

#include "terrain/TerrainGenerator.h"
#include "../extern/fastnoiselite/FastNoiseLite.h"
#include "glm/gtc/round.hpp"
#include "util/Log.h"


void TerrainGenerator::GenNoiseChunk(unsigned int seed, int width, float resolution,
	float height_scale, float frequency, glm::vec3 bot_left_coord, TerrainData& data, unsigned int master_width)
{
	TerrainGenerator::TerrainData& terrain_data = data;

	static FastNoiseLite noise;
	static FastNoiseLite noise2;
	static FastNoiseLite noise3;
	static bool noise_is_loaded = false;

	if (!noise_is_loaded) {
		noise3.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
		noise3.SetFractalType(FastNoiseLite::FractalType_FBm);
		noise3.SetFrequency(0.0005);
		noise3.SetFractalGain(0.f);
		noise3.SetFractalOctaves(10.f);
		noise3.SetCellularDistanceFunction(FastNoiseLite::CellularDistanceFunction_Euclidean);
		noise3.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add);
		noise3.SetFractalPingPongStrength(1.0f);

		noise2.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
		noise2.SetCellularReturnType(FastNoiseLite::CellularReturnType_CellValue);
		noise2.SetFractalType(FastNoiseLite::FractalType_PingPong);
		noise2.SetFractalOctaves(4.f);
		noise2.SetFractalGain(1.f);
		noise2.SetFrequency(0.001f);
		noise2.SetCellularJitter(2.0);
		noise2.SetFractalPingPongStrength(10.0f);

		noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		noise.SetFrequency(0.00075f);
		noise.SetFractalType(FastNoiseLite::FractalType_FBm);
		noise.SetFractalOctaves(5.0);

		noise_is_loaded = true;
	}

	int ceiled_resolution = glm::ceil(resolution);
	std::vector<glm::vec3> normals_to_average;
	std::vector<glm::vec3> tangents_to_average;
	normals_to_average.reserve(((width) / ceiled_resolution) * ((width) / ceiled_resolution));
	tangents_to_average.reserve(((width) / ceiled_resolution) * ((width) / ceiled_resolution));

	terrain_data.positions.reserve(terrain_data.positions.size() + ((width)) * ((width) / ceiled_resolution) * 4);
	terrain_data.normals.reserve(terrain_data.normals.size() + ((width) / ceiled_resolution) * ((width) / ceiled_resolution) * 4);
	terrain_data.tangents.reserve(terrain_data.tangents.size() + ((width) / ceiled_resolution) * ((width) / ceiled_resolution) * 4);
	terrain_data.tex_coords.reserve(terrain_data.tex_coords.size() + ((width) / ceiled_resolution) * ((width) / ceiled_resolution) * 4);
	terrain_data.indices.reserve(terrain_data.indices.size() + ((width) / ceiled_resolution) * ((width) / ceiled_resolution) * 6);

	terrain_data.bounding_box.max = bot_left_coord;
	terrain_data.bounding_box.min = bot_left_coord;

	for (float x = bot_left_coord.x; x < bot_left_coord.x + width; x += resolution) {
		for (float z = bot_left_coord.z; z < bot_left_coord.z + width; z += resolution) {
			TerrainGenerator::QuadVertices verts = GenQuad(resolution, glm::vec3(x, bot_left_coord.y, z));

			const double x_coord_1 = x;
			const double x_coord_2 = (x + resolution);
			const double z_coord_1 = z;
			const double z_coord_2 = (z + resolution);

			const double noise_1 = noise2.GetNoise(x_coord_1, z_coord_1) * 0.05 + noise3.GetNoise(x_coord_1, z_coord_1) * 1
				+ 0.03 * noise.GetNoise(x_coord_1, z_coord_1);

			const double noise_2 = noise2.GetNoise(x_coord_1, z_coord_2) * 0.05 + noise3.GetNoise(x_coord_1, z_coord_2) * 1
				+ 0.03 * noise.GetNoise(x_coord_1, z_coord_2);

			const double noise_3 = noise2.GetNoise(x_coord_2, z_coord_1) * 0.05 + noise3.GetNoise(x_coord_2, z_coord_1) * 1
				+ 0.03 * noise.GetNoise(x_coord_2, z_coord_1);

			const double noise_4 = noise2.GetNoise(x_coord_2, z_coord_2) * 0.05 + noise3.GetNoise(x_coord_2, z_coord_2) * 1
				+ 0.03 * noise.GetNoise(x_coord_2, z_coord_2);

			const float height_exponent = 7.5f;

			/*Rounding will cause shelves*/
			const double height_factor = glm::pow(height_scale, height_exponent);
			//BL
			verts.vert_1.y += (noise_1)*height_factor;
			//TL
			verts.vert_2.y += (noise_2)*height_factor;
			//BR
			verts.vert_3.y += (noise_3)*height_factor;
			//TR
			verts.vert_4.y += (noise_4)*height_factor;

			/* Form bounding box for chunk*/
			terrain_data.bounding_box.max.x = terrain_data.bounding_box.max.x < x ? x : terrain_data.bounding_box.max.x;
			terrain_data.bounding_box.max.y = terrain_data.bounding_box.max.y < verts.vert_1.y ? verts.vert_1.y : terrain_data.bounding_box.max.y;
			terrain_data.bounding_box.max.z = terrain_data.bounding_box.max.z < z ? z : terrain_data.bounding_box.max.z;

			terrain_data.bounding_box.min.x = terrain_data.bounding_box.min.x > x ? x : terrain_data.bounding_box.min.x;
			terrain_data.bounding_box.min.y = terrain_data.bounding_box.min.y > verts.vert_1.y ? verts.vert_1.y : terrain_data.bounding_box.min.y;
			terrain_data.bounding_box.min.z = terrain_data.bounding_box.min.z > z ? z : terrain_data.bounding_box.min.z;

			terrain_data.positions.emplace_back(verts.vert_1);
			int index_1 = terrain_data.positions.size() - 1;
			terrain_data.positions.emplace_back(verts.vert_2);
			terrain_data.positions.emplace_back(verts.vert_3);
			terrain_data.positions.emplace_back(verts.vert_4);

			terrain_data.indices.emplace_back(index_1 + 1);
			terrain_data.indices.emplace_back(index_1 + 3);
			terrain_data.indices.emplace_back(index_1 + 2);
			terrain_data.indices.emplace_back(index_1);


			const glm::vec3 adj_bl_coord = bot_left_coord;
			const float adj_z = (z + master_width * 0.5f) * 0.5f;
			const float adj_z_2 = (z + resolution + master_width * 0.5f) * 0.5f;
			const float adj_x = (x + master_width * 0.5f) * 0.5f;
			const float adj_x_2 = (x + resolution + master_width * 0.5f) * 0.5f;

			const float adj_z_coord = ((adj_z) * 0.03);
			const float adj_x_coord = ((adj_x) * 0.03);

			const float adj_z_coord_2 = (adj_z_2 * 0.03);
			const float adj_x_coord_2 = (adj_x_2 * 0.03);

			glm::vec2 bl_tex_coord = { adj_x_coord, adj_z_coord };
			glm::vec2 tl_tex_coord = { adj_x_coord, adj_z_coord_2 };
			glm::vec2 br_tex_coord = { adj_x_coord_2, adj_z_coord };
			glm::vec2 tr_tex_coord = { adj_x_coord_2, adj_z_coord_2 };

			terrain_data.tex_coords.emplace_back(bl_tex_coord);
			terrain_data.tex_coords.emplace_back(tl_tex_coord);
			terrain_data.tex_coords.emplace_back(br_tex_coord);
			terrain_data.tex_coords.emplace_back(tr_tex_coord);

			/* Calculate normals and tangents */
			const glm::vec3 edge1 = verts.vert_2 - verts.vert_1;
			const glm::vec3 edge2 = verts.vert_3 - verts.vert_1;
			const glm::vec2 delta_uv1 = verts.tex_coord_2 - verts.tex_coord_1;
			const glm::vec2 delta_uv2 = verts.tex_coord_3 - verts.tex_coord_1;

			const float f = 1.0f / (delta_uv1.x * delta_uv2.y - delta_uv2.x * delta_uv1.y);

			verts.tangent_1.x = f * (delta_uv2.y * edge1.x - delta_uv1.y * edge2.x);
			verts.tangent_1.y = f * (delta_uv2.y * edge1.y - delta_uv1.y * edge2.y);
			verts.tangent_1.z = f * (delta_uv2.y * edge1.z - delta_uv1.y * edge2.z);

			verts.normal_1 = glm::cross(verts.vert_2 - verts.vert_1, verts.vert_3 - verts.vert_1);

			const glm::vec3 edge3 = verts.vert_3 - verts.vert_2;
			const glm::vec3 edge4 = verts.vert_4 - verts.vert_2;
			const glm::vec2 delta_uv3 = verts.tex_coord_3 - verts.tex_coord_2;
			const glm::vec2 delta_uv4 = verts.tex_coord_4 - verts.tex_coord_2;

			const float f2 = 1.0f / (delta_uv3.x * delta_uv4.y - delta_uv4.x * delta_uv3.y);

			verts.tangent_2.x = f2 * (delta_uv4.y * edge3.x - delta_uv3.y * edge4.x);
			verts.tangent_2.y = f2 * (delta_uv4.y * edge3.y - delta_uv3.y * edge4.y);
			verts.tangent_2.z = f2 * (delta_uv4.y * edge3.z - delta_uv3.y * edge4.z);

			verts.normal_2 = glm::cross(verts.vert_3 - verts.vert_2, verts.vert_2 - verts.vert_4);

			normals_to_average.emplace_back(glm::normalize((verts.normal_1 + verts.normal_2) * 0.5f));
			tangents_to_average.emplace_back(glm::normalize((verts.tangent_1 + verts.tangent_2) * 0.5f));

		}
	}

	/* Average normals/tangents for smoothing */
	const int column_length_normals = width / resolution;

	for (int i = 0; i < normals_to_average.size(); i++) {
		glm::vec3 left_normal = glm::vec3(0);
		glm::vec3 right_normal = glm::vec3(0);
		glm::vec3 top_normal = glm::vec3(0);
		glm::vec3 bottom_normal = glm::vec3(0);
		glm::vec3 original_normal = normals_to_average[i];

		glm::vec3 left_tangent = glm::vec3(0);
		glm::vec3 right_tangent = glm::vec3(0);
		glm::vec3 top_tangent = glm::vec3(0);
		glm::vec3 bottom_tangent = glm::vec3(0);
		glm::vec3 original_tangent = tangents_to_average[i];

		float division_num = 1.f;

		if (i - column_length_normals >= 0) {
			left_normal = normals_to_average[i - column_length_normals];
			left_tangent = tangents_to_average[i - column_length_normals];
			division_num++;
		}

		if (i + column_length_normals < normals_to_average.size()) {
			right_normal = normals_to_average[i + column_length_normals];
			right_tangent = tangents_to_average[i + column_length_normals];
			division_num++;
		}
		if (i - 1 >= 0) {
			top_normal = normals_to_average[i - 1];
			top_tangent = tangents_to_average[i - 1];
			division_num++;
		}
		if (i + 1 < normals_to_average.size()) {
			bottom_normal = normals_to_average[i + 1];
			bottom_tangent = tangents_to_average[i + 1];
			division_num++;
		}

		glm::vec3 averaged_normal = glm::normalize(glm::vec3(left_normal + right_normal + top_normal + bottom_normal + original_normal) / division_num);
		glm::vec3 averaged_tangent = glm::normalize(glm::vec3(left_tangent + right_tangent + top_tangent + bottom_tangent + original_tangent) / division_num);

		terrain_data.normals.emplace_back(original_normal);
		terrain_data.normals.emplace_back(original_normal);
		terrain_data.normals.emplace_back(original_normal);
		terrain_data.normals.emplace_back(original_normal);
		terrain_data.tangents.emplace_back(averaged_tangent);
		terrain_data.tangents.emplace_back(averaged_tangent);
		terrain_data.tangents.emplace_back(averaged_tangent);
		terrain_data.tangents.emplace_back(averaged_tangent);

	}
	terrain_data.bounding_box.center = (terrain_data.bounding_box.max + terrain_data.bounding_box.min) * 0.5f;
}


TerrainGenerator::TriangleVertices TerrainGenerator::GenTriangle(bool flipped, float size, glm::vec3 pos) {
	TerrainGenerator::TriangleVertices verts;

	/* Triangle created around top right vertex facing left & down if flipped, else created around bottom left vertex facing up & right*/
	if (flipped) {
		verts.vert_1 = glm::vec3(pos.x, pos.y, pos.z);
		verts.vert_2 = glm::vec3(pos.x, pos.y, pos.z - size);
		verts.vert_3 = glm::vec3(pos.x - size, pos.y, pos.z);
	}
	else {
		verts.vert_1 = glm::vec3(pos.x, pos.y, pos.z);
		verts.vert_2 = glm::vec3(pos.x, pos.y, pos.z + size);
		verts.vert_3 = glm::vec3(pos.x + size, pos.y, pos.z);
	}

	return verts;
}

TerrainGenerator::QuadVertices TerrainGenerator::GenQuad(float size, glm::vec3 bot_left_vert_pos) {
	TerrainGenerator::QuadVertices verts;

	verts.vert_1 = glm::vec3(bot_left_vert_pos.x, bot_left_vert_pos.y, bot_left_vert_pos.z);
	verts.vert_2 = glm::vec3(bot_left_vert_pos.x, bot_left_vert_pos.y, bot_left_vert_pos.z + size);
	verts.vert_3 = glm::vec3(bot_left_vert_pos.x + size, bot_left_vert_pos.y, bot_left_vert_pos.z);
	verts.vert_4 = glm::vec3(bot_left_vert_pos.x + size, bot_left_vert_pos.y, bot_left_vert_pos.z + size);


	return verts;
};