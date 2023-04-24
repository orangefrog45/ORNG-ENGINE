#include "pch/pch.h"
#include "ExtraMath.h"

glm::mat3 ExtraMath::Init2DScaleTransform(float x, float y) {
	glm::mat3 scale_matrix(
		x, 0.0f, 0.0f,
		0.0f, y, 0.0f,
		0.0f, 0.0f, 1.0f
	);

	return scale_matrix;
}

glm::mat3 ExtraMath::Init2DRotateTransform(float rot) {
	float rx = ToRadians(rot);

	glm::mat3 rot_matrix(
		cosf(rot), -sin(rot), 0.0f,
		sin(rot), cos(rot), 0.0f,
		0.0f, 0.0f, 1.0f
	);

	return rot_matrix;
}

glm::mat3 ExtraMath::Init2DTranslationTransform(float x, float y) {

	glm::mat3 translation_matrix(
		1.0f, 0.0f, x,
		0.0f, 1.0f, y,
		0.0f, 0.0f, 1.0f
	);

	return translation_matrix;
}

glm::mat4x4 ExtraMath::Init3DRotateTransform(float rotX, float rotY, float rotZ) {

	float rX = ToRadians(rotX);
	float rY = ToRadians(rotY);
	float rZ = ToRadians(rotZ);

	glm::mat4x4 rotationXMatrix(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, cosf(rX), -sinf(rX), 0.0f,
		0.0f, sinf(rX), cosf(rX), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	glm::mat4x4 rotationYMatrix(
		cosf(rY), 0.0f, sinf(rY), 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		-sinf(rY), 0.0f, cosf(rY), 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	glm::mat4x4 rotationZMatrix(
		cosf(rZ), -sinf(rZ), 0.0f, 0.0f,
		sinf(rZ), cosf(rZ), 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return rotationXMatrix * rotationYMatrix * rotationZMatrix;
}

glm::mat4x4 ExtraMath::Init3DScaleTransform(float scaleX, float scaleY, float scaleZ) {

	glm::mat4x4 scaleMatrix(
		scaleX, 0.0f, 0.0f, 0.0f,
		0.0f, scaleY, 0.0f, 0.0f,
		0.0f, 0.0f, scaleZ, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return scaleMatrix;

}

glm::mat4x4 ExtraMath::Init3DTranslationTransform(float tranX, float tranY, float tranZ) {

	glm::mat4x4 translationMatrix(
		1.0f, 0.0f, 0.0f, tranX,
		0.0f, 1.0f, 0.0f, tranY,
		0.0f, 0.0f, 1.0f, tranZ,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return translationMatrix;

}

glm::mat4 ExtraMath::GetCameraTransMatrix(glm::vec3 pos) {
	glm::mat4x4 cameraTransMatrix(
		1.0f, 0.0f, 0.0f, pos.x,
		0.0f, 1.0f, 0.0f, pos.y,
		0.0f, 0.0f, 1.0f, pos.z,
		0.0f, 0.0f, 0.0f, 1.0f
	);
	return cameraTransMatrix;
}

glm::mat4x4 ExtraMath::Init3DCameraTransform(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) {
	glm::vec3 n = target;
	glm::normalize(n);

	glm::vec3 u;
	u = glm::cross(up, n);

	glm::vec3 v = glm::cross(n, u);
	glm::normalize(v);

	glm::mat4x4 cameraRotMatrix(
		u.x, u.y, u.z, 0.0f,
		v.x, v.y, v.z, 0.0f,
		n.x, n.y, n.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	glm::mat4x4 cameraTransMatrix(
		1.0f, 0.0f, 0.0f, -pos.x,
		0.0f, 1.0f, 0.0f, -pos.y,
		0.0f, 0.0f, 1.0f, -pos.z,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return cameraRotMatrix * cameraTransMatrix;
}

glm::mat4x4 ExtraMath::InitPersProjTransform(float FOV, float WINDOW_WIDTH, float WINDOW_HEIGHT, float zNear, float zFar) {
	const float ar = WINDOW_WIDTH / WINDOW_HEIGHT;
	const float zRange = zNear - zFar;
	const float tanHalfFOV = tanf(ToRadians(FOV / 2.0f));
	const float f = 1.0f / tanHalfFOV;

	//normalize, due to precision keep z-values low anyway
	const float A = (-zFar - zNear) / zRange;
	const float B = 2.0f * zFar * zNear / zRange;

	glm::mat4x4 projectionMatrix(
		f / ar, 0.0f, 0.0f, 0.0f,
		0.0f, f, 0.0f, 0.0f,
		0.0f, 0.0f, A, B,
		0.0f, 0.0f, 1.0f, 0.0f
	);

	return projectionMatrix;
}


