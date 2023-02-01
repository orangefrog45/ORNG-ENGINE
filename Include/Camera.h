#include <glm/glm.hpp>

class Camera {
public:
	Camera();

	void SetPosition(float x, float y, float z);

	void OnKeyboard(unsigned char key);

	glm::fmat4x4 GetMatrix();


private:

	glm::fvec3 m_pos;
	glm::fvec3 m_target;
	glm::fvec3 m_up;
	float m_speed = 1.0f;

};