#include "common.h"

namespace
{
	const auto engineUpdateListener = controlThread().update.listen([]() {
		ProfilingScope profiling("physics");
		for (Entity *e : engineEntities()->component<PhysicsComponent>()->entities())
		{
			TransformComponent &t = e->value<TransformComponent>();
			PhysicsComponent &p = e->value<PhysicsComponent>();
			CAGE_ASSERT(p.acceleration.valid() && p.velocity.valid() && t.position.valid());
			CAGE_ASSERT(p.rotation.valid() && t.orientation.valid());
			p.velocity += p.acceleration;
			if (lengthSquared(p.velocity) > sqr(p.maxSpeed))
				p.velocity = normalize(p.velocity) * p.maxSpeed;
			p.acceleration = Vec3();
			t.position += p.velocity;
			t.orientation = p.rotation * t.orientation;
		}
	}, 100);
}
