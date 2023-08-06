#pragma once
#include "util/ExtraMath.h"

namespace ORNG {
	struct BoundingVolume {
		BoundingVolume() = default;
		virtual ~BoundingVolume() = default;
		virtual bool IsOnFrustum(const ExtraMath::Frustum& cam_frustum) const = 0;
	};

	struct AABB : public BoundingVolume {
		AABB() = default;
		AABB(glm::vec3 t_min, glm::vec3 t_max) : min(t_min), max(t_max) { center = (t_max + t_min) * 0.5f; };
		glm::vec3 min = { 0.f, 0.f, 0.f };
		glm::vec3 max = { 0.f, 0.f, 0.f };
		glm::vec3 center = { 0.f, 0.f, 0.f };

		/* Will return true if the box is on or in front of the plane */
		static bool TestAABBPlane(const AABB& box, const ExtraMath::Plane& p) {
			glm::vec3 extents = box.max - box.center;

			float radius = extents.x * abs(p.normal.x) + extents.y * abs(p.normal.y) + extents.z * abs(p.normal.z);

			return -radius <= p.GetSignedDistanceToPlane(box.center);
		}

		static bool AABBPointIntersectionTest(const AABB& box, glm::vec3 point) {
			return (
				point.x >= box.min.x &&
				point.x <= box.max.x &&
				point.y >= box.min.y &&
				point.y <= box.max.y &&
				point.z >= box.min.z &&
				point.z <= box.max.z
				);
		}

		static bool AABBIntersectionTest(const AABB& a, const AABB& b) {
			return (
				a.min.x <= b.max.x &&
				a.max.x >= b.min.x &&
				a.min.y <= b.max.y &&
				a.max.y >= b.min.y &&
				a.min.z <= b.max.z &&
				a.max.z >= b.min.z
				);
		}

		bool IsOnFrustum(const ExtraMath::Frustum& cam_frustum) const {

			return (TestAABBPlane(*this, cam_frustum.top_plane) &&
				TestAABBPlane(*this, cam_frustum.bottom_plane) &&
				TestAABBPlane(*this, cam_frustum.left_plane) &&
				TestAABBPlane(*this, cam_frustum.right_plane) &&
				TestAABBPlane(*this, cam_frustum.near_plane) &&
				TestAABBPlane(*this, cam_frustum.far_plane));
		};
	};

}