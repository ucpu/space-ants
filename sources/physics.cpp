#include "common.h"

namespace
{
	const auto engineUpdateListener = controlThread().update.listen(
		[]()
		{
			const ProfilingScope profiling("physics");
			entitiesVisitor(
				[](TransformComponent &t, PhysicsComponent &p)
				{
					CAGE_ASSERT(p.acceleration.valid() && p.velocity.valid() && t.position.valid());
					CAGE_ASSERT(p.rotation.valid() && t.orientation.valid());
					p.velocity += p.acceleration;
					if (lengthSquared(p.velocity) > sqr(p.maxSpeed))
						p.velocity = normalize(p.velocity) * p.maxSpeed;
					p.acceleration = Vec3();
					t.position += p.velocity;
					t.orientation = p.rotation * t.orientation;
				},
				engineEntities(), false);
		},
		100);
}
