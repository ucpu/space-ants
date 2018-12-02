#ifndef ants_common_h_sdg456ds4hg6
#define ants_common_h_sdg456ds4hg6

#include <atomic>

#include <cage-core/core.h>
#include <cage-core/log.h>
#include <cage-core/math.h>
#include <cage-core/entities.h>

#include <cage-client/core.h>
#include <cage-client/engine.h>

using namespace cage;

struct physicsComponent
{
	static componentClass *component;

	quat rotation;
	vec3 velocity;
	vec3 acceleration;
	real maxSpeed;

	physicsComponent();
};

struct ownerComponent
{
	static componentClass *component;

	uint32 owner;

	ownerComponent();
};

struct lifeComponent
{
	static componentClass *component;

	std::atomic<sint32> life;

	lifeComponent();
};

struct shipComponent
{
	static componentClass *component;

	uint32 currentTarget;
	uint32 longtermTarget;

	shipComponent();
};

struct planetComponent
{
	static componentClass *component;

	uint32 production; // added resources per tick
	uint32 resources; // current resources
	uint32 batch; // required resources for next batch of ships

	planetComponent();
};

struct timeoutComponent
{
	static componentClass *component;

	sint32 ttl;

	timeoutComponent();
};

extern groupClass *entitiesToDestroy;

#define GAME_GET_COMPONENT(T,C,E) ::CAGE_JOIN(T, Component) &C = (E)->value<::CAGE_JOIN(T, Component)>(::CAGE_JOIN(T, Component)::component);

uint32 pickTargetPlanet(uint32 shipOwner);

extern real shipSeparation;
extern real shipSteadfast;
extern real shipCohesion;
extern real shipAlignment;
extern real shipDetectRadius;

#endif
