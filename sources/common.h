#ifndef ants_common_h_sdg456ds4hg6
#define ants_common_h_sdg456ds4hg6

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

	quat rotation; // = angular velocity
	quat angularAcceleration;
	vec3 velocity; // = translational velocity
	vec3 acceleration; // = translational acceleration
	real radius;

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

	sint32 life;

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

	uint32 production;

	planetComponent();
};

struct timeoutComponent
{
	static componentClass *component;

	uint32 ttl;

	timeoutComponent();
};

extern groupClass *entitiesToDestroy;

#define GAME_GET_COMPONENT(T,C,E) ::CAGE_JOIN(T, Component) &C = (E)->value<::CAGE_JOIN(T, Component)>(::CAGE_JOIN(T, Component)::component);

#endif
