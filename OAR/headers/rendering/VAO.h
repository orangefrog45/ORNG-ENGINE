#pragma once
#include "util/util.h"


namespace ORNG {


	class VAO {
		friend class GL_StateManager;
	public:
		VAO() {
			glGenVertexArrays(1, &m_vao_handle);
			glGenBuffers((GLsizei)m_buffers.size(), &m_buffers[0]);
		};

		~VAO() {
			glDeleteBuffers((GLsizei)m_buffers.size(), &m_buffers[0]);
			glDeleteVertexArrays(1, &m_vao_handle);
		};

		/* Fill all buffers with data provided to vectors in class
		* glcomponentsize is the number of components per generic vertex attribute (3 = triangles, 4 = quads etc)
		*/
		void FillBuffers();

		/* Fully re-allocate all memory for the transform buffer, only use when shrinking/growing the buffer */
		//void UpdateTransformMatrixBuffer(const std::vector<glm::mat4>& transforms);

		/* Update section of transform buffer, prefer this for performance */
		//void SubUpdateTransformMatrixBuffer(unsigned int index_offset, std::vector<glm::mat4>& transforms);


		// Generate SSBO to hold world transforms, call each time a new MeshInstanceGroup is created as transforms between them need to be seperated
		[[nodiscard]] unsigned int GenTransformSSBO();

		/* Fully re - allocate all memory for the transform SSBO, only use when shrinking / growing the buffer
		* Custom buffer size can be specified, if this is too small it will throw an error
		* */
		void FullUpdateTransformSSBO(unsigned int ssbo_handle, const std::vector<glm::mat4>* transforms, long long buffer_size = -1);

		//Update section of transform buffer, prefer this for performance
		void SubUpdateTransformSSBO(unsigned int ssbo_handle, unsigned int index_offset, std::vector<glm::mat4>& transforms);

		struct VertexData {
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec3> tangents;
			std::vector<glm::vec2> tex_coords;
			std::vector<unsigned int> indices;
		};

		VertexData vertex_data;

	private:
		enum BUFFER_TYPE {
			INDEX_BUFFER = 0,
			POS_VB = 1,
			TEXCOORD_VB = 2,
			NORMAL_VB = 3,
			WORLD_MAT_VB = 4,
			TANGENT_VB = 5,
			NUM_BUFFERS = 6
		};

		std::array<unsigned int, NUM_BUFFERS> m_buffers;
		std::vector<unsigned int> m_transform_ssbo_handles;
		unsigned int m_vao_handle;
	};

}