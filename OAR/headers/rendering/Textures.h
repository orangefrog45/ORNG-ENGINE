#pragma once
#include "util/util.h"

class TextureBase {
public:
	TextureBase() = delete;
	virtual ~TextureBase() { Unload(); };

	void Unload() { GLCall(glDeleteTextures(1, &m_texture_obj)); };

	unsigned int GetTextureRef() const { return m_texture_obj; }

protected:
	explicit TextureBase(unsigned int texture_target) : m_texture_target(texture_target) {};
	std::string m_filename;
	unsigned int m_texture_target = 0;
	unsigned int m_texture_obj = 0;

};

struct TextureBaseSpec {

	GLenum internal_format = GL_NONE;
	GLenum format = GL_NONE;
	GLenum min_filter = GL_NONE;
	GLenum mag_filter = GL_NONE;

	GLenum wrap_params = GL_REPEAT;
	bool generate_mipmaps = false;

};

struct Texture2DSpec : public TextureBaseSpec {
	std::string filename = "";
};

struct Texture2DArraySpec : public TextureBaseSpec {
	int tex_width = -1;
	int tex_height = -1;
	std::vector<std::string> filepaths;
};

struct TextureCubemapSpec : public TextureBaseSpec {
	std::array<std::string, 6> filepaths;
};

class Texture2D : public TextureBase {
public:
	friend class EditorLayer;
	friend class Scene;
	explicit Texture2D(const std::string& filename);

	bool SetSpec(const Texture2DSpec& spec);
	bool ValidateSpec() const;
	virtual bool Load();
	const std::string& GetFilename() const { return m_spec.filename; };

private:
	Texture2DSpec m_spec;
};




class Texture2DArray : public TextureBase {
public:
	Texture2DArray();
	bool Load();
	bool ValidateSpec() const;

	bool SetSpec(const Texture2DArraySpec& spec);

private:
	Texture2DArraySpec m_spec;
};


class TextureCubemap : public TextureBase {
	friend class Renderer;
public:
	TextureCubemap();
	bool ValidateSpec() const;
	bool SetSpec(const TextureCubemapSpec& spec);
	bool Load();
private:
	TextureCubemapSpec m_spec;
};
