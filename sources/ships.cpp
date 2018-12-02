#include <vector>
#include <algorithm>
#include <random>

#include "common.h"

#include <cage-core/random.h>
#include <cage-core/spatial.h>
#include <cage-core/geometry.h>
#include <cage-core/threadPool.h>
#include <cage-core/concurrent.h>
#include <cage-core/hashString.h>

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

real shipSeparation = 0.005;
real shipSteadfast = 0.002;
real shipCohesion = 0.005;
real shipAlignment = 0.002;
real shipDetectRadius = 4;

namespace
{
	holder<spatialDataClass> spatialData = newSpatialData(spatialDataCreateConfig());
	holder<threadPoolClass> threads = newThreadPool("ships_");
	holder<mutexClass> mutex = newMutex();

	void shipsUpdateEntry(uint32 thrIndex, uint32 thrCount)
	{
		holder<spatialQueryClass> spatialQuery = newSpatialQuery(spatialData.get());
		entityManagerClass *ents = entities();

		entityClass *const *entsArr = shipComponent::component->group()->array();
		uint32 entsTotal = shipComponent::component->group()->count();
		uint32 entsPerThr = entsTotal / thrCount;
		uint32 myStart = entsPerThr * thrIndex;
		uint32 myEnd = thrIndex + 1 == thrCount ? entsTotal : myStart + entsPerThr;

		std::vector<std::pair<entityClass*, entityClass*>> shots;
		shots.reserve(myEnd - myStart);

		for (uint32 entIndex = myStart; entIndex != myEnd; entIndex++)
		{
			entityClass *e = entsArr[entIndex];

			ENGINE_GET_COMPONENT(transform, t, e);
			GAME_GET_COMPONENT(ship, s, e);
			GAME_GET_COMPONENT(owner, owner, e);
			GAME_GET_COMPONENT(physics, phys, e);

			// find all nearby objects
			spatialQuery->intersection(sphere(t.position, t.scale + shipDetectRadius));
			uint32 myName = e->name();
			vec3 avgPos;
			vec3 avgDir;
			uint32 avgCnt = 0;
			uint32 closestTargetName = 0;
			real closestTargetDistance = real::PositiveInfinity;
			bool currentTargetInVicinity = false;
			for (uint32 nearbyName : spatialQuery->result())
			{
				if (nearbyName == myName)
					continue;
				entityClass *n = ents->get(nearbyName);
				ENGINE_GET_COMPONENT(transform, nt, n);
				vec3 d = nt.position - t.position;
				real l = d.length();
				if (l > 1e-7)
				{
					vec3 dn = d / l;
					phys.acceleration -= dn / sqr(max(l - t.scale - nt.scale, 1e-7)) * shipSeparation; // separation
				}
				if (n->has(ownerComponent::component))
				{
					GAME_GET_COMPONENT(owner, no, n);
					if (no.owner == owner.owner)
					{
						// friend
						if (n->has(shipComponent::component))
						{
							GAME_GET_COMPONENT(physics, np, n);
							avgPos += nt.position;
							if (np.velocity.squaredLength() > 1e-10)
								avgDir += np.velocity.normalize();
							avgCnt++;
						}
					}
					else
					{
						// enemy
						if (nearbyName == s.currentTarget)
							currentTargetInVicinity = true;
						else
						{
							if (l < closestTargetDistance)
							{
								closestTargetDistance = l;
								closestTargetName = nearbyName;
							}
						}
					}
				}
			}
			if (avgCnt > 0)
			{
				avgPos = avgPos / avgCnt - t.position;
				if (avgPos.squaredLength() > 1e-10)
					phys.acceleration += avgPos.normalize() * shipCohesion; // cohesion
				if (avgDir.squaredLength() > 1e-10)
					phys.acceleration += avgDir.normalize() * shipAlignment; // alignment
			}

			// update target
			if (!currentTargetInVicinity && closestTargetName)
				s.currentTarget = closestTargetName;
			else
			{
				// use long-term target
				if (!ents->has(s.longtermTarget))
					s.longtermTarget = pickTargetPlanet(owner.owner);
				s.currentTarget = s.longtermTarget;
			}

			if (ents->has(s.currentTarget))
			{
				entityClass *target = ents->get(s.currentTarget);
				// accelerate towards target
				ENGINE_GET_COMPONENT(transform, targetTransform, target);
				vec3 f = targetTransform.position - t.position;
				real l = f.length() - t.scale - targetTransform.scale;
				if (l > 1e-7)
					phys.acceleration += f.normalize() * shipSteadfast;
				if (l < shipDetectRadius)
				{
					// fire at the target
					CAGE_ASSERT_RUNTIME(target->has(lifeComponent::component));
					GAME_GET_COMPONENT(life, targetLife, target);
					targetLife.life--;
					shots.emplace_back(e, target);
				}
			}
			else
				phys.acceleration = -phys.velocity;

			{
				// update ship orientation
				if (phys.acceleration.squaredLength() > 1e-10)
					t.orientation = quat(phys.acceleration.normalize(), t.orientation * vec3(0, 1, 0));
			}

			CAGE_ASSERT_RUNTIME(phys.acceleration.valid() && phys.velocity.valid(), phys.acceleration, phys.velocity, t.position);
		}

		{
			// generate lasers
			scopeLock<mutexClass> lock(mutex);
			for (auto &it : shots)
			{
				entityClass *from = it.first;
				entityClass *target = it.second;
				ENGINE_GET_COMPONENT(transform, ft, from);
				ENGINE_GET_COMPONENT(transform, tt, target);
				entityClass *e = ents->createAnonymous();
				ENGINE_GET_COMPONENT(transform, t, e);
				vec3 f = tt.position - ft.position;
				t.scale = f.length() - ft.scale - tt.scale;
				f = f.normalize();
				t.position = ft.position + f * ft.scale;
				t.orientation = quat(f, vec3(0, 1, 0));
				ENGINE_GET_COMPONENT(render, fr, from);
				ENGINE_GET_COMPONENT(render, r, e);
				r.object = hashString("ants/laser/laser.obj");
				r.color = fr.color;
				GAME_GET_COMPONENT(timeout, ttl, e);
				ttl.ttl = 1;
			}
		}
	}

	void engineUpdate()
	{
		// add all physics objects into spatial data
		spatialData->clear();
		for (entityClass *e : physicsComponent::component->entities())
		{
			ENGINE_GET_COMPONENT(transform, t, e);
			spatialData->update(e->name(), sphere(t.position, t.scale));
		}
		spatialData->rebuild();

		// update ships
		threads->function.bind<&shipsUpdateEntry>();
		threads->run();
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
