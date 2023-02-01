#include "ExtraMath.h"
#include <glm/glm.hpp>


const double ExtraMath::pi = atan(1) * 4;

glm::fmat4x4 ExtraMath::Init3DRotateTransform(float rotX, float rotY, float rotZ) {

	float rX = ToRadians(rotX);
	float rY = ToRadians(rotY);
	float rZ = ToRadians(rotZ);

	glm::fmat4x4 rotationXMatrix(
		1.0f, 0.0f,		0.0f,	   0.0f,
		0.0f, cosf(rX), -sinf(rX), 0.0f,
		0.0f, sinf(rX), cosf(rX),  0.0f,
		0.0f, 0.0f,		0.0f,	   1.0f
	);

	glm::fmat4x4 rotationYMatrix(
		cosf(rY),  0.0f,  sinf(rY), 0.0f,
		0.0f,      1.0f,  0.0f,     0.0f,
		-sinf(rY), 0.0f,  cosf(rY), 0.0f,
		0.0f,      0.0f,  0.0f,	    1.0f
	);

	glm::fmat4x4 rotationZMatrix(
		cosf(rZ), -sinf(rZ), 0.0f, 0.0f,
		sinf(rZ), cosf(rZ),  0.0f, 0.0f,
		0.0f,	  0.0f,		 1.0f, 0.0f,
		0.0f,	  0.0f,		 0.0f, 1.0f
	);

	return rotationXMatrix * rotationYMatrix * rotationZMatrix;
}

glm::fmat4x4 ExtraMath::Init3DScaleTransform(float scaleX, float scaleY, float scaleZ) {

	glm::fmat4x4 scaleMatrix(
		scaleX, 0.0f, 0.0f, 0.0f,
		0.0f, scaleY, 0.0f, 0.0f,
		0.0f, 0.0f, scaleZ, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return scaleMatrix;

}

glm::fmat4x4 ExtraMath::Init3DTranslationTransform(float tranX, float tranY, float tranZ) {

	glm::fmat4x4 translationMatrix(
		1.0f, 0.0f, 0.0f, tranX,
		0.0f, 1.0f, 0.0f, tranY,
		0.0f, 0.0f, 1.0f, tranZ,
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return translationMatrix;

}

glm::fmat4x4 ExtraMath::Init3DCameraTransform(const glm::fvec3& pos, const glm::fvec3& target, const glm::fvec3& up) {
	glm::fvec3 n = target;
	glm::normalize(n);

	glm::fvec3 u;
	u = glm::cross(up, n);

	glm::fvec3 v = glm::cross(n, u);

	glm::fmat4x4 cameraMatrix(
		u.x, u.y, u.z, -pos.x,
		v.x, v.y, v.z, -pos.y,
		n.x, n.y, n.z, -pos.z,
		0.0f, 0.0f, 0.0f, 1.0f
	); 

	return cameraMatrix;
}

