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
		BufferBase(GLenum t_buffer_type, bool is_mutable, GLbitfield flags = 0);

		void Init();

		bool IsInitialized() {
			return m_is_initalized;
		}

		virtual ~BufferBase();

		virtual size_t GetSizeCPU() = 0;

		int GetGPU_BufferSize();

		void SetFlags(GLbitfield flags) {
			m_flags = flags;
			Resize(GetGPU_BufferSize());
		}

		uint32_t GetHandle() const {
			return m_ogl_handle;
		}

		GLbitfield GetFlags() {
			return m_flags;
		}

		// Allocates buffer on GPU with data
		virtual void FillBuffer();

		void BufferSubData(size_t start, size_t size, const std::byte* p_data);

		// Allocates new buffer with size size_bytes, old data is copied
		void Resize(size_t size_bytes);

		// Sets this buffer's data exactly equal to another, sets size of this buffer to size of other buffer + extra_allocated_size, extra memory is uninitialized
		void CopyDataFromBuffer(BufferBase& other, unsigned extra_allocated_size=0);

		// Allocates new buffer with elements from start to start + erase_size removed, old data is copied, start and erase_size should be in bytes
		void Erase(size_t start, size_t erase_size);

		void PushBack(std::byte* bytes, size_t size);

		// Only use on types that have no implicit padding, else manually convert to a byte array
		template<typename T>
		void PushBack(const T& data) {
			std::vector<std::byte> bytes;
			ConvertToBytes(data, &bytes[0]);

			PushBack(&bytes[0], sizeof(T));
		}

		GLenum draw_type = GL_STATIC_DRAW;
		GLenum data_type = GL_NONE;

	protected:
		virtual void* GetDataPtr() = 0;
		uint32_t m_ogl_handle = 0;
		GLenum buffer_type = GL_NONE;
		GLbitfield m_flags = 0;
	private:
		bool m_is_mutable = true;
		bool m_is_initalized = false;
	};

	template<typename DataType>
	class SSBO : public BufferBase {
	public:
		SSBO(bool is_mutable, GLbitfield flags) : BufferBase(GL_SHADER_STORAGE_BUFFER, is_mutable, flags) { };

		size_t GetSizeCPU() override { return data.size() * sizeof(DataType); }

		void* GetDataPtr() override { return data.data(); }

		std::vector<DataType> data;
	};

	class UBO : public BufferBase {
	public:
		UBO(bool is_mutable, GLbitfield flags) : BufferBase(GL_UNIFORM_BUFFER, is_mutable, flags) { };

		size_t GetSizeCPU() override { return data.size(); }

		void* GetDataPtr() override { return data.data(); }

		std::vector<std::byte> data;
	};



	class VertexBufferBase : public BufferBase {
		friend class VAO;
	public:
		VertexBufferBase() : BufferBase(GL_ARRAY_BUFFER, true, 0) {};

		size_t GetSizeCPU() override = 0;

		void* GetDataPtr() override = 0;

		void FillBuffer() override;

		uint8_t comps_per_attribute = 0;
		uint32_t stride = 0;
		uint8_t index = 0;
	};

	// Buffer will take ownership of data when set, when the buffer is destroyed the memory will be freed
	template<typename T>
	class VertexBufferGL : public VertexBufferBase {
		friend class VAO;
	public:
		size_t GetSizeCPU() override {
			return sizeof(T) * data.size();
		}

		std::vector<T> data;
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
		size_t GetSizeCPU() override {
			return size;
		}

		void* p_data = nullptr;
		size_t size = 0;
	private:
		void* GetDataPtr() override {
			return p_data;
		}
	};



	class ElementBufferGL : public BufferBase {
	public:
		ElementBufferGL() : BufferBase(GL_ELEMENT_ARRAY_BUFFER, false, GL_DYNAMIC_STORAGE_BIT) { };
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
				if (it == m_buffers.end())
					return;
			}
		}

		// Index = buffer position in vao, e.g index 0 can be accessed in shader as layout(location=0)
		template<std::derived_from<VertexBufferBase> T>
		T* AddBuffer(uint8_t index, GLenum data_type, uint8_t comps_per_attribute, GLenum draw_type) {
			auto* p_buf = new T();
			p_buf->Init();
			p_buf->data_type = data_type;
			p_buf->comps_per_attribute = comps_per_attribute;
			p_buf->draw_type = draw_type;
			p_buf->index = index;
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
			if (!m_buffers.contains(index)) {
				ORNG_CORE_ERROR("VAO::DeleteBuffer failed, buffer with index '{0}' not found", index);
			}
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