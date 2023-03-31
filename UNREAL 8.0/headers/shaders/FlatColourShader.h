#include <Shader.h>
#include <glm/glm.hpp>
class FlatColorShader : public Shader {
public:
	FlatColorShader() { paths.emplace_back("res/shaders/FlatColourVS.shader"); paths.emplace_back("res/shaders/FlatColourFS.shader"); }
	void SetColor(float r, float g, float b) { glUniform3f(m_color_loc, r, g, b); }
private:
	void InitUniforms() override;
	GLint m_color_loc;
};