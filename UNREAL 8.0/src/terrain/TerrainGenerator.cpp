#include "TerrainGenerator.h"

TerrainGenerator::TerrainData TerrainGenerator::GenWavyGrid(int width_x, int width_z, glm::fvec3 center, int resolution) {
	TerrainGenerator::TerrainData terrain_data;
	float angle = 0;
	float angle_h = 0;
	static float angle_offset = 0.0f;
	static float angle_offset_h = 0.0f;
	std::vector<glm::fvec3>& grid_verts = terrain_data.positions;
	std::vector<glm::fvec3>& grid_normals = terrain_data.normals;
	std::vector<glm::fvec2>& grid_tex_coords = terrain_data.tex_coords;

	grid_verts.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6); /* 6 vertices per quad (two triangles) of terrain */
	grid_normals.reserve(((width_x) / resolution) * ((width_x) / resolution) * 6);

	float x_start = -(width_x / 2.0f) + center.x;
	float z_start = -(width_z / 2.0f) + center.z;

	float x_end = (width_x / 2.0f) + center.x;
	float z_end = (width_z / 2.0f) + center.z;


	for (int x = x_start; x <= x_end; x += resolution) {
		for (int z = z_start; z <= z_end; z += resolution) {
			TerrainGenerator::QuadVertices verts = GenQuad(resolution, glm::fvec3(x, center.y, z));

			verts.bot_left_triangle.vert_1.y += (sinf(glm::radians(angle + angle_offset)) + cosf(glm::radians(angle_h + angle_offset_h))) * 15.0f;
			verts.bot_left_triangle.vert_2.y += (sinf(glm::radians(angle + angle_offset)) + cosf(glm::radians(angle_h + angle_offset_h + resolution))) * 15.0f;
			verts.bot_left_triangle.vert_3.y += (sinf(glm::radians(angle + angle_offset + resolution)) + cosf(glm::radians(angle_h + angle_offset_h))) * 15.0f;

			verts.top_right_triangle.vert_1.y += (sinf(glm::radians(angle + angle_offset + resolution)) + cosf(glm::radians(angle_h + angle_offset_h + resolution))) * 15.0f;
			verts.top_right_triangle.vert_2.y += (sinf(glm::radians(angle + angle_offset + resolution)) + cosf(glm::radians(angle_h + angle_offset_h))) * 15.0f;
			verts.top_right_triangle.vert_3.y += (sinf(glm::radians(angle + angle_offset)) + cosf(glm::radians(angle_h + angle_offset_h + resolution))) * 15.0f;

			grid_verts.push_back(verts.bot_left_triangle.vert_1);
			grid_verts.push_back(verts.bot_left_triangle.vert_2);
			grid_verts.push_back(verts.bot_left_triangle.vert_3);
			grid_tex_coords.push_back(glm::fvec2(0, 0));
			grid_tex_coords.push_back(glm::fvec2(0, 1));
			grid_tex_coords.push_back(glm::fvec2(1, 0));
			//triangles share vertex 2 and 3
			//every triangle shares two vertices
			//should generate vertices i know will be unique to this triangle at the current moment first, store those as indices, link up other vertices using math

			glm::fvec3 normal_1 = glm::cross(verts.bot_left_triangle.vert_2 - verts.bot_left_triangle.vert_1, verts.bot_left_triangle.vert_3 - verts.bot_left_triangle.vert_1);
			grid_normals.push_back(normal_1);
			grid_normals.push_back(normal_1);
			grid_normals.push_back(normal_1);

			grid_verts.push_back(verts.top_right_triangle.vert_1);
			grid_verts.push_back(verts.top_right_triangle.vert_2);
			grid_verts.push_back(verts.top_right_triangle.vert_3);
			grid_tex_coords.push_back(glm::fvec2(1, 1));
			grid_tex_coords.push_back(glm::fvec2(1, 0));
			grid_tex_coords.push_back(glm::fvec2(0, 1));



			glm::fvec3 normal_2 = glm::cross(verts.top_right_triangle.vert_2 - verts.top_right_triangle.vert_1, verts.top_right_triangle.vert_3 - verts.top_right_triangle.vert_1);
			grid_normals.push_back(normal_2);
			grid_normals.push_back(normal_2);
			grid_normals.push_back(normal_2);

			angle_h += resolution;
		}
		angle_h = 0.0f;
		angle += resolution;
	}

	angle_offset += 0.7f;
	angle_offset_h += 0.7f;

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

	verts.bot_left_triangle = GenTriangle(false, size, bot_left_vert_pos);
	verts.top_right_triangle = GenTriangle(true, size, glm::fvec3(bot_left_vert_pos.x + size, bot_left_vert_pos.y, bot_left_vert_pos.z + size));

	return verts;
};