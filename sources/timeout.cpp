#include "common.h"

#include <cage-core/hashString.h>
#include <cage-core/color.h>

EntityGroup *entitiesToDestroy;

namespace
{
	Vec3 colorVariation(const Vec3 &c)
	{
		Vec3 hsv = colorRgbToHsv(c);
		hsv[0] += randomRange(0.8, 1.2);
		hsv[0] = hsv[0] % 1;
		hsv[1] += randomRange(-0.2, 0.2);
		hsv[2] += randomRange(-0.2, 0.2);
		hsv = clamp(hsv, 0, 1);
		return colorHsvToRgb(hsv);
	}

	void shipExplode(Entity *ship)
	{
		TransformComponent &st = ship->value<TransformComponent>();
		RenderComponent &sr = ship->value<RenderComponent>();
		::PhysicsComponent &sp = (ship)->value<::PhysicsComponent>(::PhysicsComponent::component);;
		uint32 cnt = randomRange(4, 7);
		for (uint32 i = 0; i < cnt; i++)
		{
			Entity *e = engineEntities()->createAnonymous();
			TransformComponent &t = e->value<TransformComponent>();
			t.scale = st.scale;
			t.position = st.position + randomDirection3() * st.scale;
			t.orientation = randomDirectionQuat();
			RenderComponent &r = e->value<RenderComponent>();
			r.object = HashString("ants/explosion/particle.blend");
			r.color = colorVariation(sr.color);
			r.intensity = 2;
			TextureAnimationComponent &at = e->value<TextureAnimationComponent>();
			at.startTime = engineControlTime();
			at.speed = randomRange(0.7, 1.5);
			::PhysicsComponent &p = (e)->value<::PhysicsComponent>(::PhysicsComponent::component);;
			p.velocity = randomDirection3() * t.scale * 0.07 + sp.velocity;
			::TimeoutComponent &ttl = (e)->value<::TimeoutComponent>(::TimeoutComponent::component);;
			ttl.ttl = numeric_cast<sint32>(Real(30) / at.speed);
		}
	}

	void engineUpdate()
	{
		{
			for (Entity *e : TimeoutComponent::component->entities())
			{
				::TimeoutComponent &t = (e)->value<::TimeoutComponent>(::TimeoutComponent::component);;
				if (t.ttl-- <= 0)
					e->add(entitiesToDestroy);
			}
		}
		{
			for (Entity *e : LifeComponent::component->entities())
			{
				::LifeComponent &l = (e)->value<::LifeComponent>(::LifeComponent::component);;
				if (l.life <= 0)
				{
					e->add(entitiesToDestroy);
					if (e->has(ShipComponent::component))
						shipExplode(e);
				}
			}
		}
		{
			entitiesToDestroy->destroy();
		}
	}

	void engineInitialize()
	{
		entitiesToDestroy = engineEntities()->defineGroup();
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize, -90);
			engineInitListener.bind<&engineInitialize>();
			engineUpdateListener.attach(controlThread().update, 90);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInstance;
}
