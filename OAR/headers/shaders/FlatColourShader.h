#include <Shader.h>
class FlatColorShader : public Shader {
public:
	FlatColorShader() { paths.emplace_back("res/shaders/FlatColourVS.shader"); paths.emplace_back("res/shaders/FlatColourFS.shader"); }
	void SetColor(float r, float g, float b);
private:
	void InitUniforms() override;
	unsigned int m_color_loc;
};