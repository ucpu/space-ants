#include "common.h"

#include <cage-core/hashString.h>
#include <cage-core/color.h>

groupClass *entitiesToDestroy;

namespace
{
	vec3 colorVariation(const vec3 &c)
	{
		vec3 hsv = convertRgbToHsv(c);
		hsv[0] += randomRange(0.8, 1.2);
		hsv[0] = hsv[0] % 1;
		hsv[1] += randomRange(-0.2, 0.2);
		hsv[2] += randomRange(-0.2, 0.2);
		hsv = clamp(hsv, 0, 1);
		return convertHsvToRgb(hsv);
	}

	void shipExplode(entityClass *ship)
	{
		ENGINE_GET_COMPONENT(transform, st, ship);
		ENGINE_GET_COMPONENT(render, sr, ship);
		GAME_GET_COMPONENT(physics, sp, ship);
		uint32 cnt = randomRange(4, 7);
		for (uint32 i = 0; i < cnt; i++)
		{
			entityClass *e = entities()->createAnonymous();
			ENGINE_GET_COMPONENT(transform, t, e);
			t.scale = st.scale;
			t.position = st.position + randomDirection3() * st.scale;
			t.orientation = randomDirectionQuat();
			ENGINE_GET_COMPONENT(render, r, e);
			r.object = hashString("ants/explosion/particle.blend");
			r.color = colorVariation(sr.color) * 2;
			ENGINE_GET_COMPONENT(animatedTexture, at, e);
			at.startTime = currentControlTime();
			at.speed = randomRange(0.7, 1.5);
			GAME_GET_COMPONENT(physics, p, e);
			p.velocity = randomDirection3() * t.scale * 0.07 + sp.velocity;
			GAME_GET_COMPONENT(timeout, ttl, e);
			ttl.ttl = numeric_cast<sint32>(real(30) / at.speed);
		}
	}

	void engineUpdate()
	{
		for (entityClass *e : timeoutComponent::component->entities())
		{
			GAME_GET_COMPONENT(timeout, t, e);
			if (t.ttl-- <= 0)
				e->add(entitiesToDestroy);
		}
		for (entityClass *e : lifeComponent::component->entities())
		{
			GAME_GET_COMPONENT(life, l, e);
			if (l.life <= 0)
			{
				e->add(entitiesToDestroy);
				if (e->has(shipComponent::component))
					shipExplode(e);
			}
		}
		entitiesToDestroy->destroy();
	}

	void engineInitialize()
	{
		entitiesToDestroy = entities()->defineGroup();
	}

	class callbacksInitClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
	public:
		callbacksInitClass()
		{
			engineInitListener.attach(controlThread().initialize, -90);
			engineInitListener.bind<&engineInitialize>();
			engineUpdateListener.attach(controlThread().update, 90);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
