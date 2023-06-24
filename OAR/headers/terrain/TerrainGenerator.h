#pragma once
#include "rendering/VAO.h"
#include "components/BoundingVolume.h"

namespace ORNG {

	class TerrainGenerator {
	public:

		struct TerrainData {
			AABB bounding_box;
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec3> tangents;
			std::vector<glm::vec2> tex_coords;
			std::vector<int> indices;
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

		// Resolution = width/height of grid, smaller = more detailed
		static void GenNoiseChunk(unsigned int seed, int width, unsigned int resolution, float height_scale, glm::vec3 bot_left_coord, VAO::VertexData& output_data, AABB& output_bounding_box);

	private:
		static QuadVertices GenQuad(float size, glm::vec3 bot_left_vert_pos);
	};
}