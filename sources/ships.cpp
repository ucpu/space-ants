#include <vector>
#include <algorithm>
#include <random>

#include "common.h"

#include <cage-core/random.h>
#include <cage-core/spatial.h>
#include <cage-core/geometry.h>

uint32 pickTargetPlanet(uint32 shipOwner)
{
	auto range = planetComponent::component->entities();
	std::vector<entityClass*> planets(range.begin(), range.end());
	std::shuffle(planets.begin(), planets.end(), std::default_random_engine((unsigned)currentRandomGenerator().next()));
	for (entityClass *e : planets)
	{
		GAME_GET_COMPONENT(owner, owner, e);
		if (owner.owner != shipOwner)
			return e->name();
	}
	return 0;
}

namespace
{
	holder<spatialDataClass> data = newSpatialData(spatialDataCreateConfig());
	holder<spatialQueryClass> query = newSpatialQuery(data.get());

	void engineUpdate()
	{
		entityManagerClass *ents = entities();

		// add all physics objects into spatial data
		data->clear();
		for (entityClass *e : physicsComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			data->update(e->name(), sphere(t.position, t.scale));
		}
		data->rebuild();

		for (entityClass *e : shipComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			GAME_GET_COMPONENT(ship, s, e);
			GAME_GET_COMPONENT(owner, owner, e);
			GAME_GET_COMPONENT(physics, phys, e);

			if (ents->has(s.currentTarget))
			{
				// accelerate towards target
				ENGINE_GET_COMPONENT(transform, target, ents->get(s.currentTarget));
				vec3 f = target.position - t.position;
				if (f.squaredLength() > 1e-7)
					phys.acceleration += f.normalize() * 0.002;
			}
			else
			{
				// update target
				if (!ents->has(s.longtermTarget))
					s.longtermTarget = pickTargetPlanet(owner.owner);
				s.currentTarget = s.longtermTarget;
			}

			// find all nearby objects
			query->intersection(sphere(t.position, t.scale + 5));
			uint32 myName = e->name();
			vec3 avgPos;
			vec3 avgDir;
			uint32 avgCnt = 0;
			for (uint32 nearbyName : query->result())
			{
				if (nearbyName == myName)
					continue;
				entityClass *n = ents->get(nearbyName);
				ENGINE_GET_COMPONENT(transform, nt, n);
				vec3 d = nt.position - t.position;
				real l = d.length();
				vec3 dn = d / l;
				phys.acceleration -= dn / sqr(max(l - t.scale - nt.scale, 0)) * 0.02; // separation
				if (n->has(ownerComponent::component) && n->has(shipComponent::component))
				{
					GAME_GET_COMPONENT(owner, no, n);
					if (no.owner == owner.owner)
					{
						// same owner
						GAME_GET_COMPONENT(physics, np, n);
						avgPos += nt.position;
						if (np.velocity.squaredLength() > 1e-7)
							avgDir += np.velocity.normalize();
						avgCnt++;
					}
				}
			}
			if (avgCnt > 0)
			{
				avgPos /= avgCnt;
				phys.acceleration += (avgPos - t.position).normalize() * 0.005; // cohesion
				if (avgDir.squaredLength() > 1e-7)
					phys.acceleration += avgDir.normalize() * 0.002; // alignment
			}

			{
				// update ship orientation
				if (phys.acceleration.squaredLength() > 1e-7)
					t.orientation = quat(phys.acceleration.normalize(), t.orientation * vec3(0, 1, 0));
			}
		}
	}

	class callbacksInitClass
	{
		eventListener<void()> engineUpdateListener;
	public:
		callbacksInitClass()
		{
			engineUpdateListener.attach(controlThread().update, 30);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
