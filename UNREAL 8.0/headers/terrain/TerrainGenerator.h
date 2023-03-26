#pragma once
#include <glm/glm.hpp>
#include <vector>

class TerrainGenerator {
public:
	struct TriangleVertices {
		glm::fvec3 vert_1 = glm::fvec3(0, 0, 0);
		glm::fvec3 vert_2 = glm::fvec3(0, 0, 0);
		glm::fvec3 vert_3 = glm::fvec3(0, 0, 0);
	};

	/*Vertices are viewed top-down on a cartesian coordinate system, x=xaxis, z=yaxis, positive right and up*/
	struct QuadVertices {
		TriangleVertices bot_left_triangle;
		TriangleVertices top_right_triangle;
	};

	struct TerrainData {
		std::vector<glm::fvec3> positions;
		std::vector<glm::fvec3> normals;
		std::vector<glm::fvec2> tex_coords;
	};

	/* Grid centered at 3d point, resolution = quad size */
	static TerrainData GenWavyGrid(int width_x, int width_z, glm::fvec3 center, int resolution);
	/* Size = width and height */

private:
	static TriangleVertices GenTriangle(bool flipped, float size, glm::fvec3 vert_pos);
	static QuadVertices GenQuad(float size, glm::fvec3 bot_left_vert_pos);
};