#include "common.h"

namespace
{
	void engineUpdate()
	{
		OPTICK_EVENT("physics");
		for (entity *e : physicsComponent::component->entities())
		{
			CAGE_COMPONENT_ENGINE(transform, t, e);
			ANTS_COMPONENT(physics, p, e);
			CAGE_ASSERT(p.acceleration.valid() && p.velocity.valid() && t.position.valid(), p.acceleration, p.velocity, t.position);
			CAGE_ASSERT(p.rotation.valid() && t.orientation.valid(), p.rotation, t.orientation);
			p.velocity += p.acceleration;
			if (squaredLength(p.velocity) > sqr(p.maxSpeed))
				p.velocity = normalize(p.velocity) * p.maxSpeed;
			p.acceleration = vec3();
			t.position += p.velocity;
			t.orientation = p.rotation * t.orientation;
		}
	}

	class callbacksInitClass
	{
		eventListener<void()> engineUpdateListener;
	public:
		callbacksInitClass()
		{
			engineUpdateListener.attach(controlThread().update, 100);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
