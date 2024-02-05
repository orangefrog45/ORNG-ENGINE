#include "pch/pch.h"

#include "terrain/TerrainGenerator.h"
#include "../extern/fastnoiselite/FastNoiseLite.h"

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



		terrain_data.positions.reserve(terrain_data.positions.size() + ((width)) * ((width) / resolution) * 4);
		terrain_data.normals.reserve(terrain_data.normals.size() + ((width) / resolution) * ((width) / resolution) * 4);
		terrain_data.tangents.reserve(terrain_data.tangents.size() + ((width) / resolution) * ((width) / resolution) * 4);
		terrain_data.tex_coords.reserve(terrain_data.tex_coords.size() + ((width) / resolution) * ((width) / resolution) * 4);
		terrain_data.indices.reserve(terrain_data.indices.size() + ((width) / resolution) * ((width) / resolution) * 4);

		bounding_box.max = bot_left_coord;
		bounding_box.min = bot_left_coord;
		for (float x = bot_left_coord.x; x < bot_left_coord.x + width; x += resolution) {
			for (float z = bot_left_coord.z; z < bot_left_coord.z + width; z += resolution) {
				const float noise1 = noise2.GetNoise(x, z) * 0.15f
					+ 3.5f * noise.GetNoise(x, z);

				const float height_exponent = 5.f;

				/*Rounding will cause shelves*/
				const float height_factor = glm::pow(height_scale, height_exponent);
				glm::vec3 vert = { x, noise1 * height_factor, z };

				// Form bounding box for chunk
				bounding_box.max.x = bounding_box.max.x < x ? x : bounding_box.max.x;
				bounding_box.max.y = bounding_box.max.y < vert.y ? vert.y : bounding_box.max.y;
				bounding_box.max.z = bounding_box.max.z < z ? z : bounding_box.max.z;

				bounding_box.min.x = bounding_box.min.x > x ? x : bounding_box.min.x;
				bounding_box.min.y = bounding_box.min.y > vert.y ? vert.y : bounding_box.min.y;
				bounding_box.min.z = bounding_box.min.z > z ? z : bounding_box.min.z;

				VEC_PUSH_VEC3(terrain_data.positions, vert);
			}
		}

		ASSERT(width % resolution == 0);

		const int side_length_steps = width / resolution;
		for (int lx = 0; lx < side_length_steps - 1; lx++) {
			int x_offset = lx * side_length_steps;
			for (int lz = 0; lz < side_length_steps - 1; lz++) {
				TerrainGenerator::QuadVertices verts{};
				verts.vert_1 = { terrain_data.positions[(x_offset + lz) * 3], terrain_data.positions[(x_offset + lz) * 3 + 1], terrain_data.positions[(x_offset + lz) * 3 + 2] };
				verts.vert_2 = { terrain_data.positions[(x_offset + lz + 1) * 3], terrain_data.positions[(x_offset + lz + 1) * 3 + 1], terrain_data.positions[(x_offset + lz + 1) * 3 + 2] };
				verts.vert_3 = { terrain_data.positions[(x_offset + lz + side_length_steps) * 3], terrain_data.positions[(x_offset + lz + side_length_steps) * 3 + 1], terrain_data.positions[(x_offset + lz + side_length_steps) * 3 + 2] };
				verts.vert_4 = { terrain_data.positions[(x_offset + lz + side_length_steps + 1) * 3], terrain_data.positions[(x_offset + lz + side_length_steps + 1) * 3 + 1], terrain_data.positions[(x_offset + lz + side_length_steps + 1) * 3 + 2] };
				verts.tex_coord_1 = glm::vec2{ (float)lx / (float)side_length_steps, (float)lz / (float)side_length_steps };
				verts.tex_coord_2 = glm::vec2{ (float)lx / (float)side_length_steps, (float)(lz + 1) / (float)side_length_steps };
				verts.tex_coord_3 = glm::vec2{ (float)(lx + 1) / (float)side_length_steps, (float)lz / (float)side_length_steps };
				verts.tex_coord_4 = glm::vec2{ (float)(lx + 1) / (float)side_length_steps, (float)(lz + 1) / (float)side_length_steps };


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

				VEC_PUSH_VEC3(terrain_data.normals, glm::normalize(glm::mix(verts.normal_1, verts.normal_2, 0.5f)));
				VEC_PUSH_VEC3(terrain_data.tangents, glm::normalize(glm::mix(verts.tangent_1, verts.tangent_2, 0.5f)));
				if (lz == side_length_steps - 2 || lx == side_length_steps - 2) {
					VEC_PUSH_VEC3(terrain_data.normals, glm::normalize(glm::mix(verts.normal_1, verts.normal_2, 0.5f)));
					VEC_PUSH_VEC3(terrain_data.tangents, glm::normalize(glm::mix(verts.tangent_1, verts.tangent_2, 0.5f)));
				}
				glm::vec2 tex_coord = verts.tex_coord_1;
				VEC_PUSH_VEC2(terrain_data.tex_coords, tex_coord);
				int bl_index = x_offset + lz;
				// Tri 1
				terrain_data.indices.push_back(bl_index);
				terrain_data.indices.push_back(bl_index + 1);
				terrain_data.indices.push_back(bl_index + side_length_steps + 1);

				terrain_data.indices.push_back(bl_index + side_length_steps);
				terrain_data.indices.push_back(bl_index);
				terrain_data.indices.push_back(bl_index + side_length_steps + 1);
			}
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