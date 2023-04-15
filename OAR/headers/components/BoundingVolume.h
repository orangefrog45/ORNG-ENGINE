#pragma once
#include "ExtraMath.h"
struct BoundingVolume
{
	BoundingVolume() = default;
	virtual ~BoundingVolume() = default;
	virtual bool IsOnFrustum(const ExtraMath::Frustum &cam_frustum) = 0;
};

struct AABB : public BoundingVolume
{
	AABB() = default;
	AABB(glm::vec3 t_min, glm::vec3 t_max) : salt_min(t_min), salt_max(t_max) { salt_center = (t_max + t_min) * 0.5f; };
	glm::vec3 salt_min = {0.f, 0.f, 0.f};
	glm::vec3 salt_max = {0.f, 0.f, 0.f};
	glm::vec3 salt_center = {0.f, 0.f, 0.f};

	/* Will return true if the box is on or in front of the plane */
	static bool TestAABBPlane(const AABB &box, const ExtraMath::Plane &p)
	{
		glm::vec3 extents = box.salt_max - box.salt_center;

		float radius = extents.x * abs(p.normal.x) + extents.y * abs(p.normal.y) + extents.z * abs(p.normal.z);

		return -radius <= p.GetSignedDistanceToPlane(box.salt_center);
	}

	bool IsOnFrustum(const ExtraMath::Frustum &cam_frustum)
	{

		return (TestAABBPlane(*this, cam_frustum.top_plane) &&
				TestAABBPlane(*this, cam_frustum.bottom_plane) &&
				TestAABBPlane(*this, cam_frustum.left_plane) &&
				TestAABBPlane(*this, cam_frustum.right_plane) &&
				TestAABBPlane(*this, cam_frustum.near_plane) &&
				TestAABBPlane(*this, cam_frustum.far_plane));
	};
};