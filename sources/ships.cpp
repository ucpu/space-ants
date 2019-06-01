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

real shipSeparation = 0.002;
real shipTargetShips = 0.005;
real shipTargetPlanets = 0.001;
real shipCohesion = 0.003;
real shipAlignment = 0.001;
real shipDetectRadius = 3;
real shipLaserRadius = 2;

namespace
{
	holder<spatialDataClass> initSpatialData()
	{
		spatialDataCreateConfig cfg;
		return newSpatialData(cfg);
	}

	holder<spatialDataClass> spatialData = initSpatialData();
	holder<threadPoolClass> threads = newThreadPool("ships_");
	holder<mutexClass> mutex = newMutex();

	uint32 tickIndex = 1;
	holder<timerClass> timer = newTimer();
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
		holder<spatialQueryClass> spatialQuery = newSpatialQuery(spatialData.get());
		entityManagerClass *ents = entities();

		entityClass *const *entsArr = shipComponent::component->group()->array();
		uint32 entsTotal = shipComponent::component->group()->count();
		uint32 entsPerThr = entsTotal / thrCount;
		uint32 myStart = entsPerThr * thrIndex;
		uint32 myEnd = thrIndex + 1 == thrCount ? entsTotal : myStart + entsPerThr;

		std::vector<laserStruct> shots;
		shots.reserve(myEnd - myStart);

		uint32 shipsInteracted = 0;

		for (uint32 entIndex = myStart; entIndex != myEnd; entIndex++)
		{
			entityClass *e = entsArr[entIndex];

			ENGINE_GET_COMPONENT(transform, t, e);
			GAME_GET_COMPONENT(ship, s, e);
			GAME_GET_COMPONENT(owner, owner, e);
			GAME_GET_COMPONENT(physics, phys, e);

			// destroy ships that wandered too far away
			if (t.position.squaredLength() > sqr(200))
			{
				GAME_GET_COMPONENT(life, life, e);
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
			shipsInteracted += spatialQuery->resultCount();
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
				if (avgPos.squaredLength() > 1e-10)
					phys.acceleration += avgPos.normalize() * shipCohesion; // cohesion
				if (avgDir.squaredLength() > 1e-10)
					phys.acceleration += avgDir.normalize() * shipAlignment; // alignment
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
				entityClass *target = ents->get(targetName);
				ENGINE_GET_COMPONENT(transform, targetTransform, target);
				vec3 f = targetTransform.position - t.position;
				real l = f.length() - t.scale - targetTransform.scale;
				if (l > 1e-7)
					phys.acceleration += f.normalize() * shipTargetShips;
			}

			// fire at closest enemy
			if (ents->has(closestTargetName))
			{
				entityClass *target = ents->get(closestTargetName);
				ENGINE_GET_COMPONENT(transform, tt, target);
				vec3 o = front(t);
				vec3 d = tt.position - o;
				real l = d.length();
				if (l < shipLaserRadius + tt.scale)
				{
					// fire at the target
					CAGE_ASSERT_RUNTIME(target->has(lifeComponent::component));
					GAME_GET_COMPONENT(life, targetLife, target);
					targetLife.life--;
					const transform &th = e->value<transformComponent>(transformComponent::componentHistory);
					const transform &tth = target->value<transformComponent>(transformComponent::componentHistory);
					laserStruct laser;
					laser.tr.position = o;
					laser.tr.orientation = quat(d, vec3(0, 1, 0));
					laser.tr.scale = l;
					laser.trh.position = front(th);
					vec3 dh = tth.position - laser.trh.position;
					real lh = dh.length();
					laser.trh.orientation = quat(dh, vec3(0, 1, 0));
					laser.trh.scale = lh;
					ENGINE_GET_COMPONENT(render, render, e);
					laser.color = render.color;
					shots.push_back(laser);
				}
			}

			CAGE_ASSERT_RUNTIME(phys.acceleration.valid() && phys.velocity.valid(), phys.acceleration, phys.velocity, t.position);

			{
				// update ship orientation
				if (phys.acceleration.squaredLength() > 1e-10)
					t.orientation = quat(phys.acceleration.normalize(), t.orientation * vec3(0, 1, 0));
			}
		}

		{
			scopeLock<mutexClass> lock(mutex);

			// generate lasers
			for (auto &it : shots)
			{
				entityClass *e = ents->createAnonymous();
				ENGINE_GET_COMPONENT(transform, t, e);
				t = it.tr;
				transform &th = e->value<transformComponent>(transformComponent::componentHistory);
				th = it.trh;
				ENGINE_GET_COMPONENT(render, r, e);
				r.object = hashString("ants/laser/laser.obj");
				r.color = it.color;
				GAME_GET_COMPONENT(timeout, ttl, e);
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

	void engineUpdate()
	{
		// add all physics objects into spatial data
		timer->reset();
		spatialData->clear();
		for (entityClass *e : physicsComponent::component->entities())
		{
			if (e->name() == 0)
				continue;
			ENGINE_GET_COMPONENT(transform, t, e);
			spatialData->update(e->name(), sphere(t.position, t.scale));
		}
		spatialData->rebuild();
		smoothTimeSpatialBuild.add(timer->microsSinceLast());

		// update ships
		timer->reset();
		threads->function.bind<&shipsUpdateEntry>();
		threads->run();
		smoothTimeShipsUpdate.add(timer->microsSinceLast());

		// log timing
		if (tickIndex++ % 120 == 0)
		{
			CAGE_LOG(severityEnum::Info, "performance", string() + "Spatial prepare time: " + smoothTimeSpatialBuild.smooth() + " us");
			CAGE_LOG(severityEnum::Info, "performance", string() + "Ships update time: " + smoothTimeShipsUpdate.smooth() + " us");
			CAGE_LOG(severityEnum::Info, "performance", string() + "Ships interaction ratio: " + real(shipsInteractionRatio.smooth()) * 0.001);
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

			//currentRandomGenerator().s[0] = 13;
			//currentRandomGenerator().s[1] = 42;
		}
	} callbacksInitInstance;
}
