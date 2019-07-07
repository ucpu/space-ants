#include "common.h"

#include <cage-core/hashString.h>
#include <cage-core/color.h>

entityGroup *entitiesToDestroy;

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

	void shipExplode(entity *ship)
	{
		CAGE_COMPONENT_ENGINE(transform, st, ship);
		CAGE_COMPONENT_ENGINE(render, sr, ship);
		ANTS_COMPONENT(physics, sp, ship);
		uint32 cnt = randomRange(4, 7);
		for (uint32 i = 0; i < cnt; i++)
		{
			entity *e = entities()->createAnonymous();
			CAGE_COMPONENT_ENGINE(transform, t, e);
			t.scale = st.scale;
			t.position = st.position + randomDirection3() * st.scale;
			t.orientation = randomDirectionQuat();
			CAGE_COMPONENT_ENGINE(render, r, e);
			r.object = hashString("ants/explosion/particle.blend");
			r.color = colorVariation(sr.color) * 2;
			CAGE_COMPONENT_ENGINE(textureAnimation, at, e);
			at.startTime = currentControlTime();
			at.speed = randomRange(0.7, 1.5);
			ANTS_COMPONENT(physics, p, e);
			p.velocity = randomDirection3() * t.scale * 0.07 + sp.velocity;
			ANTS_COMPONENT(timeout, ttl, e);
			ttl.ttl = numeric_cast<sint32>(real(30) / at.speed);
		}
	}

	void engineUpdate()
	{
		OPTICK_EVENT("timeout");
		{
			OPTICK_EVENT("timeout");
			for (entity *e : timeoutComponent::component->entities())
			{
				ANTS_COMPONENT(timeout, t, e);
				if (t.ttl-- <= 0)
					e->add(entitiesToDestroy);
			}
		}
		{
			OPTICK_EVENT("life");
			for (entity *e : lifeComponent::component->entities())
			{
				ANTS_COMPONENT(life, l, e);
				if (l.life <= 0)
				{
					e->add(entitiesToDestroy);
					if (e->has(shipComponent::component))
						shipExplode(e);
				}
			}
		}
		{
			OPTICK_EVENT("entities destroy");
			entitiesToDestroy->destroy();
		}
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
