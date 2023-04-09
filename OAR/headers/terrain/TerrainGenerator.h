#pragma once
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include <vector>
#include "BoundingVolume.h"


class TerrainGenerator {
public:
	struct TriangleVertices {
		glm::vec3 vert_1 = glm::vec3(0, 0, 0);
		glm::vec3 vert_2 = glm::vec3(0, 0, 0);
		glm::vec3 vert_3 = glm::vec3(0, 0, 0);
	};

	/*Vertices are viewed top-down on a cartesian coordinate system, x=xaxis, z=yaxis, positive right and up*/
	struct QuadVertices {
		glm::vec3 vert_1 = glm::vec3(0, 0, 0); //BL
		glm::vec3 vert_2 = glm::vec3(0, 0, 0); //TL
		glm::vec3 vert_3 = glm::vec3(0, 0, 0); // BR
		glm::vec3 vert_4 = glm::vec3(0, 0, 0); //TR

		glm::vec2 tex_coord_1 = glm::vec2(0, 0);
		glm::vec2 tex_coord_2 = glm::vec2(0, 1);
		glm::vec2 tex_coord_3 = glm::vec2(1, 0);
		glm::vec2 tex_coord_4 = glm::vec2(1, 1);

		glm::vec3 normal_1 = glm::vec3(0);
		glm::vec3 normal_2 = glm::vec3(0);
		glm::vec3 tangent_1 = glm::vec3(0);
		glm::vec3 tangent_2 = glm::vec3(0);
	};

	struct TerrainData {
		AABB bounding_box;
		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec3> tangents;
		std::vector<glm::vec2> tex_coords;
		std::vector<int> indices;
	};

	/* Grid centered at 3d point, resolution = quad size */
	static void GenWavyGrid(int width_x, int width_z, glm::vec3 center, int resolution, float height_range, float amplitude);
	/*Resolution = triangle size, lower values = more detail
	* Chunk generated inside "data" argument to stop copies being made
	*/
	static void GenNoiseChunk(unsigned int seed, int width, float resolution, float height_scale, float frequency, glm::vec3 bot_left_coord, TerrainData& data, unsigned int master_width);

private:
	static TriangleVertices GenTriangle(bool flipped, float size, glm::vec3 vert_pos);
	static QuadVertices GenQuad(float size, glm::vec3 bot_left_vert_pos);
};