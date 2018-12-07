#include "common.h"

#include <cage-core/hashString.h>

groupClass *entitiesToDestroy;

namespace
{
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
			r.color = sr.color;
			ENGINE_GET_COMPONENT(animatedTexture, at, e);
			at.startTime = currentControlTime();
			at.speed = randomRange(0.7, 1.5);
			GAME_GET_COMPONENT(physics, p, e);
			p.velocity = randomDirection3() * t.scale * 0.04 + sp.velocity;
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
