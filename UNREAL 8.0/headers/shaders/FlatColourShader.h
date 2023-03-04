#include <Shader.h>
#include <glm/glm.hpp>
class FlatColorShader : public Shader {
public:
	void Init() override;
	void ActivateProgram() override;
	void SetWVP(const glm::fmat4& mat) { glUniformMatrix4fv(wvp_loc, 1, GL_TRUE, &mat[0][0]); }
	void SetColor(float r, float g, float b) { glUniform3f(color_loc, r, g, b); }
private:
	void InitUniforms() override;
	unsigned int vert_shader_id;
	unsigned int frag_shader_id;
	unsigned int programID;
	GLint wvp_loc;
	GLint color_loc;
};