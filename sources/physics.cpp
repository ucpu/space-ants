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
			CAGE_ASSERT_RUNTIME(p.acceleration.valid() && p.velocity.valid() && t.position.valid(), p.acceleration, p.velocity, t.position);
			CAGE_ASSERT_RUNTIME(p.rotation.valid() && t.orientation.valid(), p.rotation, t.orientation);
			p.velocity += p.acceleration;
			if (p.velocity.squaredLength() > p.maxSpeed.sqr())
				p.velocity = p.velocity.normalize() * p.maxSpeed;
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
