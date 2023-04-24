#pragma once

class Framebuffer {
public:
	Framebuffer() = default;
	Framebuffer(unsigned int id, const char* name) : m_name(name), m_framebuffer_id(id) { };

	struct FB_TextureObject {
		FB_TextureObject() = default;
		FB_TextureObject(unsigned int t_type, unsigned int t_tex_ref, const char* t_name) : type(t_type), tex_ref(t_tex_ref), name(t_name) {};
		unsigned int type;
		unsigned int tex_ref;
		const char* name;
	};

	FB_TextureObject& Add2DTexture(const std::string& name, unsigned int width, unsigned int height, unsigned int attachment_point, unsigned int internal_format,
		unsigned int format, unsigned int gl_type);

	FB_TextureObject& Add2DTextureArray(const std::string& name, unsigned int width, unsigned int height, unsigned int layers, unsigned int internal_format,
		unsigned int format, unsigned int gl_type);


	void AddRenderbuffer();

	void BindTextureLayerToFBAttachment(unsigned int tex_ref, unsigned int attachment, unsigned int layer);

	void EnableReadBuffer(unsigned int buffer);

	void EnableDrawBuffers(unsigned int amount, unsigned int buffers[]);

	FB_TextureObject& GetTexture(const std::string& name);

	virtual ~Framebuffer();
	void Init();
	void Bind() const;
protected:
	unsigned int m_framebuffer_id;
	const char* m_name = "Unnamed framebuffer";
	std::unordered_map <std::string, FB_TextureObject> m_textures;
	unsigned int m_fbo = 0;
	unsigned int m_rbo = 0;
};