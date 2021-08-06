#include "common.h"

namespace
{
	void engineUpdate()
	{
		for (Entity *e : PhysicsComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(Transform, t, e);
			ANTS_COMPONENT(Physics, p, e);
			CAGE_ASSERT(p.acceleration.valid() && p.velocity.valid() && t.position.valid());
			CAGE_ASSERT(p.rotation.valid() && t.orientation.valid());
			p.velocity += p.acceleration;
			if (lengthSquared(p.velocity) > sqr(p.maxSpeed))
				p.velocity = normalize(p.velocity) * p.maxSpeed;
			p.acceleration = vec3();
			t.position += p.velocity;
			t.orientation = p.rotation * t.orientation;
		}
	}

	class Callbacks
	{
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineUpdateListener.attach(controlThread().update, 100);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}
