#include "common.h"

namespace
{
	void engineUpdate()
	{
		for (Entity *e : PhysicsComponent::component->entities())
		{
			TransformComponent &t = e->value<TransformComponent>();
			::PhysicsComponent &p = (e)->value<::PhysicsComponent>(::PhysicsComponent::component);;
			CAGE_ASSERT(p.acceleration.valid() && p.velocity.valid() && t.position.valid());
			CAGE_ASSERT(p.rotation.valid() && t.orientation.valid());
			p.velocity += p.acceleration;
			if (lengthSquared(p.velocity) > sqr(p.maxSpeed))
				p.velocity = normalize(p.velocity) * p.maxSpeed;
			p.acceleration = Vec3();
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
