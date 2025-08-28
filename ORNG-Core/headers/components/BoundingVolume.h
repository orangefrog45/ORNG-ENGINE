#pragma once
#include "util/ExtraMath.h"

namespace ORNG {
	struct BoundingVolume {
		BoundingVolume() = default;
		BoundingVolume(const BoundingVolume&) = default;
		BoundingVolume& operator=(const BoundingVolume&) = default;
		virtual ~BoundingVolume() = default;
		virtual bool IsOnFrustum(const ExtraMath::Frustum& cam_frustum) const = 0;
	};

	struct AABB : public BoundingVolume {
		AABB() = default;
		AABB(glm::vec3 _extents) : extents(_extents) { }
		glm::vec3 extents = { 0.f, 0.f, 0.f };
		glm::vec3 center = { 0.f, 0.f, 0.f };

		/* Will return true if the box is on or in front of the plane */
		static bool TestAABBPlane(const AABB& box, const ExtraMath::Plane& p) {
			float radius = box.extents.x * abs(p.normal.x) + box.extents.y * abs(p.normal.y) + box.extents.z * abs(p.normal.z);

			return -radius <= p.GetSignedDistanceToPlane(box.center);
		}

		static bool AABBPointIntersectionTest(const AABB& box, glm::vec3 point) {
			auto min = box.center - box.extents;
			auto max = box.center + box.extents;
			return (
				point.x >= min.x &&
				point.x <= max.x &&
				point.y >= min.y &&
				point.y <= max.y &&
				point.z >= min.z &&
				point.z <= max.z
				);
		}

		static bool AABBIntersectionTest(const AABB& a, const AABB& b) {
			auto a_min = a.center - a.extents;
			auto a_max = a.center + a.extents;

			auto b_min = b.center - b.extents;
			auto b_max = b.center + b.extents;
			return (
				a_min.x <= b_max.x &&
				a_max.x >= b_min.x &&
				a_min.y <= b_max.y &&
				a_max.y >= b_min.y &&
				a_min.z <= b_max.z &&
				a_max.z >= b_min.z
				);
		}

		bool IsOnFrustum(const ExtraMath::Frustum& cam_frustum) const override {
			return (TestAABBPlane(*this, cam_frustum.top_plane) &&
				TestAABBPlane(*this, cam_frustum.bottom_plane) &&
				TestAABBPlane(*this, cam_frustum.left_plane) &&
				TestAABBPlane(*this, cam_frustum.right_plane) &&
				TestAABBPlane(*this, cam_frustum.near_plane) &&
				TestAABBPlane(*this, cam_frustum.far_plane));
		}
	};
}
