#include <vector>

#include "common.h"

#include <cage-core/hashString.h>
#include <cage-core/color.h>

namespace
{
	const uint32 modelNames[] = {
		hashString("ants/planets/farcodev/0.object"),
		hashString("ants/planets/farcodev/1.object"),
		hashString("ants/planets/farcodev/2.object"),
		hashString("ants/planets/farcodev/3.object"),
		hashString("ants/planets/farcodev/4.object"),
		hashString("ants/planets/farcodev/5.object"),
		hashString("ants/planets/farcodev/6.object"),
		hashString("ants/planets/farcodev/7.object"),
	};

#ifdef CAGE_DEBUG
	const uint32 shipsLimit = 200;
	const uint32 batchScale = 1;
#else
	const uint32 shipsLimit = 5000;
	const uint32 batchScale = 10;
#endif // CAGE_DEBUG

	void engineInitialize()
	{
		uint32 playersCount = randomRange(2u, 5u);
		std::vector<vec3> playerColors;
		playerColors.resize(playersCount);
		real hueOff = randomChance();
		for (uint32 p = 0; p < playersCount; p++)
			playerColors[p] = colorHsvToRgb(vec3((real(p) / playersCount + hueOff) % 1, randomRange(0.5, 1.0), randomRange(0.5, 1.0)));
		uint32 planetsCount = randomRange(3u, 12u);
		for (uint32 p = 0; p < planetsCount; p++)
		{
			entity *e = entities()->createUnique();
			CAGE_COMPONENT_ENGINE(transform, t, e);
			t.position = randomDirection3() * randomRange(80, 150);
			t.orientation = randomDirectionQuat();
			t.scale = 5;
			ANTS_COMPONENT(owner, owner, e);
			owner.owner = p % playersCount;
			CAGE_COMPONENT_ENGINE(render, r, e);
			r.object = modelNames[randomRange(0u, (uint32)(sizeof(modelNames) / sizeof(modelNames[0])))];
			r.color = playerColors[owner.owner];
			ANTS_COMPONENT(physics, physics, e);
			physics.rotation = interpolate(quat(), randomDirectionQuat(), 0.0003);
			ANTS_COMPONENT(life, life, e);
			life.life = randomRange(1000000, 2000000);
			ANTS_COMPONENT(planet, planet, e);
			planet.batch = randomRange(3 * batchScale, 5 * batchScale);
		}
	}

	void createShip(entity *planet, uint32 target)
	{
		entity *e = entities()->createUnique();
		CAGE_COMPONENT_ENGINE(transform, planetTransform, planet);
		CAGE_COMPONENT_ENGINE(transform, t, e);
		t.scale = 0.3;
		t.position = planetTransform.position + randomDirection3() * (t.scale + planetTransform.scale + 1e-5);
		t.orientation = randomDirectionQuat();
		ANTS_COMPONENT(owner, planetOwner, planet);
		ANTS_COMPONENT(owner, owner, e);
		owner.owner = planetOwner.owner;
		CAGE_COMPONENT_ENGINE(render, planetRender, planet);
		CAGE_COMPONENT_ENGINE(render, r, e);
		r.color = planetRender.color;
		r.object = hashString("ants/ships/1/1.object");
		ANTS_COMPONENT(physics, physics, e);
		ANTS_COMPONENT(life, life, e);
		life.life = randomRange(200, 300);
		ANTS_COMPONENT(ship, ship, e);
		ship.longtermTarget = target;
	}

	uint32 planetIndex = 0;

	void engineUpdate()
	{
		OPTICK_EVENT("planets");
		uint32 shipsCount = shipComponent::component->group()->count();
		uint32 planetsCount = planetComponent::component->group()->count();
		uint32 currentIndex = 0;
		for (entity *e : planetComponent::component->entities())
		{
			ANTS_COMPONENT(planet, p, e);
			if (currentIndex++ != (planetIndex % planetsCount))
				continue;
			if (shipsCount + p.batch > shipsLimit)
				continue;
			ANTS_COMPONENT(owner, owner, e);
			uint32 target = pickTargetPlanet(owner.owner);
			for (uint32 s = 0; s < p.batch; s++)
				createShip(e, target);
			p.batch = randomRange(3 * batchScale, 5 * batchScale);
			planetIndex++;
		}
	}

	class callbacksInitClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
	public:
		callbacksInitClass()
		{
			engineInitListener.attach(controlThread().initialize, -50);
			engineInitListener.bind<&engineInitialize>();
			engineUpdateListener.attach(controlThread().update, 60);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
