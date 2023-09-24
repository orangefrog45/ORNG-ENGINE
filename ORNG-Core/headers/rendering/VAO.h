#pragma once
#include "util/util.h"


namespace ORNG {

	struct VertexData3D {
		std::vector<float> positions;
		std::vector<float> normals;
		std::vector<float> tangents;
		std::vector<float> tex_coords;
		std::vector<unsigned int> indices;
	};


	class BufferBase {
		friend class VAO;
	public:
		BufferBase(GLenum t_buffer_type);
		virtual ~BufferBase() { glDeleteBuffers(1, &m_ogl_handle); };
		virtual uint32_t GetSize() = 0;
		// Number of components per vertex attribute, e.g 3D position (x,y,z) = 3
		uint32_t GetHandle() const {
			return m_ogl_handle;
		}

		GLenum draw_type = GL_STATIC_DRAW;
		GLenum data_type = GL_NONE;
	private:
		uint32_t m_ogl_handle = 0;
	protected:
		GLenum buffer_type = GL_NONE;
		virtual void* GetDataPtr() = 0;
	};



	class VertexBufferBase : public BufferBase {
		friend class VAO;
	public:
		VertexBufferBase() : BufferBase(GL_ARRAY_BUFFER) {};
		uint32_t GetSize() override = 0;
		void* GetDataPtr() override = 0;
		uint8_t comps_per_attribute = 0;
		uint32_t stride = 0;
		uint8_t index = 0;
	};

	// Buffer will take ownership of data when set, when the buffer is destroyed the memory will be freed
	template<typename T>
	class VertexBufferGL : public VertexBufferBase {
		friend class VAO;
	public:
		std::vector<T> data;
		uint32_t GetSize() override {
			return sizeof(T) * data.size();
		}
	private:
		virtual void* GetDataPtr() {
			return static_cast<void*>(data.data());
		}
	};



	// Used to interface with other api's where p_data can point to external data without requiring a copy into the vector, less safe but more memory efficient
	// Use owning buffer over this when possible
	// p_data will not be freed upon buffer destruction
	class ExternVertexBufferGL : public VertexBufferBase {
		friend class VAO;
	public:
		uint32_t GetSize() override {
			return size;
		}

		void* p_data = nullptr;
		uint32_t size = 0;
	private:
		virtual void* GetDataPtr() {
			return p_data;
		}
	};



	class ElementBufferGL : public BufferBase {
	public:
		ElementBufferGL() : BufferBase(GL_ELEMENT_ARRAY_BUFFER) { };
		std::vector<uint32_t> indices;
	};




	class VAO_Base {
	public:
		VAO_Base() = default;
		virtual ~VAO_Base();
		virtual void FillBuffers() = 0;
		void Init() {
			glGenVertexArrays(1, &m_vao_handle);
		}

		uint32_t GetHandle() const {
			return m_vao_handle;
		}
	protected:
		uint32_t m_vao_handle;
	};

	class VAO : public VAO_Base {
	public:
		void FillBuffers() override;
		~VAO() {
			for (auto it = m_buffers.begin(); it != m_buffers.end(); it++) {
				delete it->second;
				it = m_buffers.erase(it);
			}
		}

		// Index = buffer position in vao, e.g index 0 can be accessed in shader as layout(location=0)
		template<std::derived_from<VertexBufferBase> T>
		T* AddBuffer(uint8_t index, GLenum data_type, uint8_t comps_per_attribute, GLenum draw_type) {
			auto* p_buf = new T();
			p_buf->data_type = data_type;
			p_buf->comps_per_attribute = comps_per_attribute;
			p_buf->draw_type = draw_type;
			m_buffers[index] = static_cast<VertexBufferBase*>(p_buf);
			return p_buf;
		}

		template<std::derived_from<VertexBufferBase> T>
		T* GetBuffer(uint8_t index) {
			ASSERT(m_buffers.contains(index));
			auto* p_buf = dynamic_cast<T*>(m_buffers[index]);
			ASSERT(p_buf);
			return p_buf;
		}

		void DeleteBuffer(uint8_t index) {
			if (!m_buffers.contains(index))
				ORNG_CORE_ERROR("VAO::DeleteBuffer failed, buffer with index '{0}' not found", index);
			else {
				delete m_buffers[index];
				m_buffers.erase(index);
			}
		}

		// Leave as nullptr if not using indices
		std::unique_ptr<ElementBufferGL> p_element_buffer = nullptr;
	private:
		std::unordered_map<uint8_t, VertexBufferBase*> m_buffers;
	};

	class MeshVAO : public VAO_Base {
		friend class GL_StateManager;
	public:
		MeshVAO();

		~MeshVAO();

		// Fill all buffers with data provided to vectors in class
		void FillBuffers();



		// Generate SSBO to hold world transforms, call each time a new MeshInstanceGroup is created as transforms between them need to be seperated
		[[nodiscard]] unsigned int GenTransformSSBO();

		/* Fully re - allocate all memory for the transform SSBO, only use when shrinking / growing the buffer
		* Custom buffer size can be specified, if this is too small it will throw an error
		* */
		void FullUpdateTransformSSBO(unsigned int ssbo_handle, const std::vector<glm::mat4>* transforms, long long buffer_size = -1);

		//Update section of transform buffer, prefer this for performance
		void SubUpdateTransformSSBO(unsigned int ssbo_handle, unsigned int index_offset, std::vector<glm::mat4>& transforms);

		void DeleteTransformSSBO(unsigned int ssbo_handle);


		VertexData3D vertex_data;

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

		// Index location : Buffer object
		std::array<unsigned int, NUM_BUFFERS> m_buffers;
		std::vector<unsigned int> m_transform_ssbo_handles;
	};

}