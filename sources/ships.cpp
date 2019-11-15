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
#include <cage-core/timer.h>
#include <cage-core/variableSmoothingBuffer.h>

uint32 pickTargetPlanet(uint32 shipOwner)
{
	auto range = planetComponent::component->entities();
	std::vector<entity*> planets(range.begin(), range.end());
	std::shuffle(planets.begin(), planets.end(), std::default_random_engine((unsigned)currentRandomGenerator().next()));
	for (entity *e : planets)
	{
		ANTS_COMPONENT(owner, owner, e);
		if (owner.owner != shipOwner)
			return e->name();
	}
	return 0;
}

real shipSeparation = 0.002;
real shipTargetShips = 0.005;
real shipTargetPlanets = 0.001;
real shipCohesion = 0.003;
real shipAlignment = 0.001;
real shipDetectRadius = 3;
real shipLaserRadius = 2;

namespace
{
	holder<spatialData> initSpatialData()
	{
		spatialDataCreateConfig cfg;
		return newSpatialData(cfg);
	}

	holder<threadPool> initThreadPool();

	holder<spatialData> spatialSearchData = initSpatialData();
	holder<threadPool> threads = initThreadPool();
	holder<syncMutex> mutex = newSyncMutex();

	uint32 tickIndex = 1;
	holder<timer> clock = newTimer();
	variableSmoothingBuffer<uint64, 512> smoothTimeSpatialBuild;
	variableSmoothingBuffer<uint64, 512> smoothTimeShipsUpdate;
	variableSmoothingBuffer<uint32, 2048> shipsInteractionRatio;

	vec3 front(const transform &t)
	{
		return t.position + t.orientation * vec3(0, 0, -1) * t.scale;
	}

	struct laserStruct
	{
		transform tr;
		transform trh;
		vec3 color;
	};

	void shipsUpdateEntry(uint32 thrIndex, uint32 thrCount)
	{
		OPTICK_EVENT("ships");

		holder<spatialQuery> spatialQuery = newSpatialQuery(spatialSearchData.get());
		entityManager *ents = entities();

		entity *const *entsArr = shipComponent::component->group()->array();
		uint32 entsTotal = shipComponent::component->group()->count();
		uint32 entsPerThr = entsTotal / thrCount;
		uint32 myStart = entsPerThr * thrIndex;
		uint32 myEnd = thrIndex + 1 == thrCount ? entsTotal : myStart + entsPerThr;

		std::vector<laserStruct> shots;
		shots.reserve(myEnd - myStart);

		uint32 shipsInteracted = 0;

		for (uint32 entIndex = myStart; entIndex != myEnd; entIndex++)
		{
			entity *e = entsArr[entIndex];

			CAGE_COMPONENT_ENGINE(transform, t, e);
			ANTS_COMPONENT(ship, s, e);
			ANTS_COMPONENT(owner, owner, e);
			ANTS_COMPONENT(physics, phys, e);

			// destroy ships that wandered too far away
			if (lengthSquared(t.position) > sqr(200))
			{
				ANTS_COMPONENT(life, life, e);
				life.life = 0;
				continue;
			}

			// find all nearby objects
			spatialQuery->intersection(sphere(t.position, t.scale + shipDetectRadius));
			uint32 myName = e->name();
			vec3 avgPos;
			vec3 avgDir;
			uint32 avgCnt = 0;
			uint32 closestTargetName = 0;
			real closestTargetDistance = real::Infinity();
			shipsInteracted += numeric_cast<uint32>(spatialQuery->result().size());
			for (uint32 nearbyName : spatialQuery->result())
			{
				if (nearbyName == myName)
					continue;
				entity *n = ents->get(nearbyName);
				CAGE_COMPONENT_ENGINE(transform, nt, n);
				vec3 d = nt.position - t.position;
				real l = length(d);
				if (l > 1e-7)
				{
					vec3 dn = d / l;
					phys.acceleration -= dn / sqr(max(l - t.scale - nt.scale, 1e-7)) * shipSeparation; // separation
				}
				if (n->has(ownerComponent::component))
				{
					ANTS_COMPONENT(owner, no, n);
					if (no.owner == owner.owner)
					{
						// friend
						if (n->has(shipComponent::component))
						{
							ANTS_COMPONENT(physics, np, n);
							avgPos += nt.position;
							if (lengthSquared(np.velocity) > 1e-10)
								avgDir += normalize(np.velocity);
							avgCnt++;
						}
					}
					else
					{
						// enemy
						if (l < closestTargetDistance)
						{
							closestTargetDistance = l;
							closestTargetName = nearbyName;
						}
					}
				}
			}
			if (avgCnt > 0)
			{
				avgPos = avgPos / avgCnt - t.position;
				if (lengthSquared(avgPos) > 1e-10)
					phys.acceleration += normalize(avgPos) * shipCohesion; // cohesion
				if (lengthSquared(avgDir) > 1e-10)
					phys.acceleration += normalize(avgDir) * shipAlignment; // alignment
			}

			// choose a target to follow
			uint32 targetName = 0;
			if (ents->has(s.currentTarget))
				targetName = s.currentTarget;
			else if (closestTargetName)
				targetName = s.currentTarget = closestTargetName;
			else
			{
				// use long-term goal
				if (!ents->has(s.longtermTarget))
					s.longtermTarget = pickTargetPlanet(owner.owner);
				targetName = s.longtermTarget;
			}

			// accelerate towards target
			if (ents->has(targetName))
			{
				entity *target = ents->get(targetName);
				CAGE_COMPONENT_ENGINE(transform, targetTransform, target);
				vec3 f = targetTransform.position - t.position;
				real l = length(f) - t.scale - targetTransform.scale;
				if (l > 1e-7)
					phys.acceleration += normalize(f) * shipTargetShips;
			}

			// fire at closest enemy
			if (ents->has(closestTargetName))
			{
				entity *target = ents->get(closestTargetName);
				CAGE_COMPONENT_ENGINE(transform, tt, target);
				vec3 o = front(t);
				vec3 d = tt.position - o;
				real l = length(d);
				if (l < shipLaserRadius + tt.scale)
				{
					// fire at the target
					CAGE_ASSERT(target->has(lifeComponent::component));
					ANTS_COMPONENT(life, targetLife, target);
					targetLife.life--;
					const transform &th = e->value<transformComponent>(transformComponent::componentHistory);
					const transform &tth = target->value<transformComponent>(transformComponent::componentHistory);
					laserStruct laser;
					laser.tr.position = o;
					laser.tr.orientation = quat(d, vec3(0, 1, 0));
					laser.tr.scale = l;
					laser.trh.position = front(th);
					vec3 dh = tth.position - laser.trh.position;
					real lh = length(dh);
					laser.trh.orientation = quat(dh, vec3(0, 1, 0));
					laser.trh.scale = lh;
					CAGE_COMPONENT_ENGINE(render, render, e);
					laser.color = render.color;
					shots.push_back(laser);
				}
			}

			CAGE_ASSERT(phys.acceleration.valid() && phys.velocity.valid(), phys.acceleration, phys.velocity, t.position);

			{
				// update ship orientation
				if (lengthSquared(phys.acceleration) > 1e-10)
					t.orientation = quat(normalize(phys.acceleration), t.orientation * vec3(0, 1, 0));
			}
		}

		{
			OPTICK_EVENT("generate lasers");
			scopeLock<syncMutex> lock(mutex);

			// generate lasers
			for (auto &it : shots)
			{
				entity *e = ents->createAnonymous();
				CAGE_COMPONENT_ENGINE(transform, t, e);
				t = it.tr;
				transform &th = e->value<transformComponent>(transformComponent::componentHistory);
				th = it.trh;
				CAGE_COMPONENT_ENGINE(render, r, e);
				r.object = hashString("ants/laser/laser.obj");
				r.color = it.color;
				ANTS_COMPONENT(timeout, ttl, e);
				ttl.ttl = 1;
			}

			// stats
			{
				uint32 cnt = myEnd - myStart;
				if (cnt > 0)
					shipsInteractionRatio.add(1000 * shipsInteracted / cnt);
			}
		}
	}

	holder<threadPool> initThreadPool()
	{
		auto thrs = newThreadPool("ships ", processorsCount() - 1);
		thrs->function.bind<&shipsUpdateEntry>();
		return thrs;
	}

	void engineUpdate()
	{
		OPTICK_EVENT("ships");

		// add all physics objects into spatial data
		clock->reset();
		{
			OPTICK_EVENT("spatial update");
			spatialSearchData->clear();
			for (entity *e : physicsComponent::component->entities())
			{
				if (e->name() == 0)
					continue;
				CAGE_COMPONENT_ENGINE(transform, t, e);
				spatialSearchData->update(e->name(), sphere(t.position, t.scale));
			}
		}
		{
			OPTICK_EVENT("spatial rebuild");
			spatialSearchData->rebuild();
		}
		smoothTimeSpatialBuild.add(clock->microsSinceLast());

		// update ships
		clock->reset();
		{
			OPTICK_EVENT("ships update");
			threads->run();
		}
		smoothTimeShipsUpdate.add(clock->microsSinceLast());

		// log timing
		if (tickIndex++ % 120 == 0)
		{
			CAGE_LOG(severityEnum::Info, "performance", stringizer() + "Spatial prepare time: " + smoothTimeSpatialBuild.smooth() + " us");
			CAGE_LOG(severityEnum::Info, "performance", stringizer() + "Ships update time: " + smoothTimeShipsUpdate.smooth() + " us");
			CAGE_LOG(severityEnum::Info, "performance", stringizer() + "Ships interaction ratio: " + real(shipsInteractionRatio.smooth()) * 0.001);
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
