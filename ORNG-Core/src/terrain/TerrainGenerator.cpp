#include "pch/pch.h"

#include "terrain/TerrainGenerator.h"
#include "../extern/fastnoiselite/FastNoiseLite.h"
#include "glm/glm/gtc/round.hpp"
#include "util/Log.h"
#include "util/TimeStep.h"
#include "../extern/glm/glm/gtc/quaternion.hpp"

namespace ORNG {

	void TerrainGenerator::GenNoiseChunk(unsigned int seed, int width, unsigned int resolution,
		float height_scale, glm::vec3 bot_left_coord, VertexData3D& output_data, AABB& bounding_box)
	{

		VertexData3D& terrain_data = output_data;

		static FastNoiseLite noise;
		static FastNoiseLite noise2;


		noise2.SetSeed(seed);
		noise2.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
		noise2.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Div);
		noise2.SetFractalType(FastNoiseLite::FractalType_FBm);
		noise2.SetFrequency(0.00125f);

		noise.SetSeed(seed);
		noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
		noise.SetFrequency(0.001f);
		noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2);
		noise.SetFractalType(FastNoiseLite::FractalType_FBm);
		noise.SetFractalOctaves(5.0);


		int ceiled_resolution = glm::ceil(resolution);
		std::vector<glm::vec3> tangents_to_average;
		std::vector<glm::vec3> normals_to_average;
		tangents_to_average.reserve(((width) / ceiled_resolution) * ((width) / ceiled_resolution));
		normals_to_average.reserve(((width) / ceiled_resolution) * ((width) / ceiled_resolution));

		terrain_data.positions.reserve(terrain_data.positions.size() + ((width)) * ((width) / ceiled_resolution) * 4);
		terrain_data.normals.reserve(terrain_data.normals.size() + ((width) / ceiled_resolution) * ((width) / ceiled_resolution) * 4);
		terrain_data.tangents.reserve(terrain_data.tangents.size() + ((width) / ceiled_resolution) * ((width) / ceiled_resolution) * 4);
		terrain_data.tex_coords.reserve(terrain_data.tex_coords.size() + ((width) / ceiled_resolution) * ((width) / ceiled_resolution) * 4);
		terrain_data.indices.reserve(terrain_data.indices.size() + ((width) / ceiled_resolution) * ((width) / ceiled_resolution) * 4);

		bounding_box.max = bot_left_coord;
		bounding_box.min = bot_left_coord;

		for (float x = bot_left_coord.x; x < bot_left_coord.x + width; x += resolution) {
			for (float z = bot_left_coord.z; z < bot_left_coord.z + width; z += resolution) {
				TerrainGenerator::QuadVertices verts = GenQuad(resolution, glm::vec3(x, bot_left_coord.y, z));

				const double x_coord_1 = x;
				const double x_coord_2 = (x + resolution);
				const double z_coord_1 = z;
				const double z_coord_2 = (z + resolution);

				const float noise_1 = noise2.GetNoise(x_coord_1, z_coord_1) * 0.15f
					+ 3.5f * noise.GetNoise(x_coord_1, z_coord_1);

				const float noise_2 = noise2.GetNoise(x_coord_1, z_coord_2) * 0.15f
					+ 3.5f * noise.GetNoise(x_coord_1, z_coord_2);

				const float noise_3 = noise2.GetNoise(x_coord_2, z_coord_1) * 0.15f
					+ 3.5f * noise.GetNoise(x_coord_2, z_coord_1);

				const float noise_4 = noise2.GetNoise(x_coord_2, z_coord_2) * 0.15f
					+ 3.5f * noise.GetNoise(x_coord_2, z_coord_2);

				const float height_exponent = 5.f;

				/*Rounding will cause shelves*/
				const float height_factor = glm::pow(height_scale, height_exponent);
				//BL
				verts.vert_1.y += noise_1 * height_factor;
				//TL
				verts.vert_2.y += noise_2 * height_factor;
				//BR
				verts.vert_3.y += noise_3 * height_factor;
				//TR
				verts.vert_4.y += noise_4 * height_factor;

				// Form bounding box for chunk
				bounding_box.max.x = bounding_box.max.x < x ? x : bounding_box.max.x;
				bounding_box.max.y = bounding_box.max.y < verts.vert_1.y ? verts.vert_1.y : bounding_box.max.y;
				bounding_box.max.z = bounding_box.max.z < z ? z : bounding_box.max.z;

				bounding_box.min.x = bounding_box.min.x > x ? x : bounding_box.min.x;
				bounding_box.min.y = bounding_box.min.y > verts.vert_1.y ? verts.vert_1.y : bounding_box.min.y;
				bounding_box.min.z = bounding_box.min.z > z ? z : bounding_box.min.z;

				VEC_PUSH_VEC3(terrain_data.positions, verts.vert_1);
				int index_1 = (terrain_data.positions.size() - 3) / 3;
				VEC_PUSH_VEC3(terrain_data.positions, verts.vert_2);
				VEC_PUSH_VEC3(terrain_data.positions, verts.vert_3);
				VEC_PUSH_VEC3(terrain_data.positions, verts.vert_4);

				terrain_data.indices.emplace_back(index_1 + 1);
				terrain_data.indices.emplace_back(index_1 + 3);
				terrain_data.indices.emplace_back(index_1 + 2);
				terrain_data.indices.emplace_back(index_1);


				const float adj_z_coord = ((z) * 0.03f);
				const float adj_z_coord_2 = ((z + resolution) * 0.03f);
				const float adj_x_coord = ((x) * 0.03f);
				const float adj_x_coord_2 = ((x + resolution) * 0.03f);

				glm::vec2 bl_tex_coord = { adj_x_coord, adj_z_coord };
				glm::vec2 tl_tex_coord = { adj_x_coord, adj_z_coord_2 };
				glm::vec2 br_tex_coord = { adj_x_coord_2, adj_z_coord };
				glm::vec2 tr_tex_coord = { adj_x_coord_2, adj_z_coord_2 };

				VEC_PUSH_VEC2(terrain_data.tex_coords, bl_tex_coord);
				VEC_PUSH_VEC2(terrain_data.tex_coords, tl_tex_coord);
				VEC_PUSH_VEC2(terrain_data.tex_coords, br_tex_coord);
				VEC_PUSH_VEC2(terrain_data.tex_coords, tr_tex_coord);

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

				//Take average of the normals of both triangles that form the quad
				glm::vec3 normal = glm::normalize(glm::mix(verts.normal_1, verts.normal_2, 0.5f));
				normals_to_average.emplace_back(normal);
				tangents_to_average.emplace_back(glm::mix(verts.tangent_1, verts.tangent_2, 0.5f));

			}
		}


		/* Average normals/tangents for smoothing */
		const int column_length_tangents = width / resolution;

		for (int i = 0; i < tangents_to_average.size(); i++) {

			glm::vec3 left_tangent = glm::vec3(0);
			glm::vec3 right_tangent = glm::vec3(0);
			glm::vec3 top_tangent = glm::vec3(0);
			glm::vec3 bottom_tangent = glm::vec3(0);
			glm::vec3 original_tangent = tangents_to_average[i];

			glm::vec3 original_normal = normals_to_average[i];
			glm::vec3 left_normal = glm::vec3(0);
			glm::vec3 right_normal = glm::vec3(0);
			glm::vec3 top_normal = glm::vec3(0);
			glm::vec3 bottom_normal = glm::vec3(0);

			float division_num = 1.f;

			if (i - column_length_tangents >= 0) {
				left_tangent = tangents_to_average[i - column_length_tangents];
				left_normal = normals_to_average[i - column_length_tangents];
				division_num++;
			}

			if (i + column_length_tangents < tangents_to_average.size()) {
				right_tangent = tangents_to_average[i + column_length_tangents];
				right_normal = normals_to_average[i + column_length_tangents];
				division_num++;
			}
			if (i - 1 >= 0) {
				top_tangent = tangents_to_average[i - 1];
				top_normal = normals_to_average[i - 1];
				division_num++;
			}
			if (i + 1 < tangents_to_average.size()) {
				bottom_tangent = tangents_to_average[i + 1];
				bottom_normal = normals_to_average[i + 1];
				division_num++;
			}

			glm::vec3 averaged_tangent = glm::normalize(glm::vec3(left_tangent + right_tangent + top_tangent + bottom_tangent + original_tangent) / division_num);
			glm::vec3 averaged_normal = glm::normalize(glm::vec3(left_normal + right_normal + top_normal + bottom_normal + original_normal) / division_num);

			VEC_PUSH_VEC3(terrain_data.tangents, averaged_tangent);
			VEC_PUSH_VEC3(terrain_data.tangents, averaged_tangent);
			VEC_PUSH_VEC3(terrain_data.tangents, averaged_tangent);
			VEC_PUSH_VEC3(terrain_data.tangents, averaged_tangent);

			VEC_PUSH_VEC3(terrain_data.normals, averaged_normal);
			VEC_PUSH_VEC3(terrain_data.normals, averaged_normal);
			VEC_PUSH_VEC3(terrain_data.normals, averaged_normal);
			VEC_PUSH_VEC3(terrain_data.normals, averaged_normal);

		}

		bounding_box.center = (bounding_box.max + bounding_box.min) * 0.5f;
	}


	TerrainGenerator::QuadVertices TerrainGenerator::GenQuad(float size, glm::vec3 bot_left_vert_pos) {
		TerrainGenerator::QuadVertices verts;

		verts.vert_1 = glm::vec3(bot_left_vert_pos.x, bot_left_vert_pos.y, bot_left_vert_pos.z);
		verts.vert_2 = glm::vec3(bot_left_vert_pos.x, bot_left_vert_pos.y, bot_left_vert_pos.z + size);
		verts.vert_3 = glm::vec3(bot_left_vert_pos.x + size, bot_left_vert_pos.y, bot_left_vert_pos.z);
		verts.vert_4 = glm::vec3(bot_left_vert_pos.x + size, bot_left_vert_pos.y, bot_left_vert_pos.z + size);


		return verts;
	};
}