#include <initializer_list>

#include "common.h"

#include <cage-engine/gui.h>

namespace
{
	real *propertyValues[] = {
		&shipTargetShips,
		&shipTargetPlanets,
		&shipSeparation,
		&shipCohesion,
		&shipAlignment,
		&shipDetectRadius,
		&shipLaserRadius,
	};

	const string propertyNames[] = {
		"Target Ships: ",
		"Target Planets: ",
		"Separation: ",
		"Cohesion: ",
		"Alignment: ",
		"Detect Radius: ",
		"Laser Radius: ",
	};

	void engineUpdate()
	{
		// nothing
	}

	eventListener<void(uint32)> guiListener;

	void guiEvent(uint32 name)
	{
		uint32 index = name - 20;
		if (index < sizeof(propertyValues) / sizeof(propertyValues[0]))
		{
			entity *e = gui()->entities()->get(name);
			CAGE_COMPONENT_GUI(input, input, e);
			if (input.valid)
				*propertyValues[index] = input.value.toFloat();
		}
	}

	void engineInitialize()
	{
		guiManager *g = cage::gui();
		guiListener.attach(g->widgetEvent);
		guiListener.bind<&guiEvent>();

		entity *topLeft = g->entities()->createUnique();
		{
			CAGE_COMPONENT_GUI(scrollbars, sc, topLeft);
		}

		entity *table = g->entities()->createUnique();
		{
			CAGE_COMPONENT_GUI(spoiler, c, table);
			CAGE_COMPONENT_GUI(layoutTable, l, table);
			CAGE_COMPONENT_GUI(parent, child, table);
			child.parent = topLeft->name();
			CAGE_COMPONENT_GUI(text, t, table);
			t.value = "Ships";
		}

		static_assert(sizeof(propertyNames) / sizeof(propertyNames[0]) == sizeof(propertyValues) / sizeof(propertyValues[0]), "arrays must have same length");
		for (uint32 i = 0; i < sizeof(propertyNames) / sizeof(propertyNames[0]); i++)
		{
			entity *lab = g->entities()->createUnique();
			{
				CAGE_COMPONENT_GUI(parent, child, lab);
				child.parent = table->name();
				child.order = i * 2 + 0;
				CAGE_COMPONENT_GUI(label, c, lab);
				CAGE_COMPONENT_GUI(text, t, lab);
				t.value = propertyNames[i];
			}
			entity *con = g->entities()->create(20 + i);
			{
				CAGE_COMPONENT_GUI(parent, child, con);
				child.parent = table->name();
				child.order = i * 2 + 1;
				CAGE_COMPONENT_GUI(input, c, con);
				c.type = inputTypeEnum::Real;
				c.min.f = 0;
				c.max.f = 0.05;
				c.step.f = 0.0002;
				c.value = stringizer() + *propertyValues[i];
			}
		}

		for (uint32 i : { 5, 6 })
		{
			// radiuses
			CAGE_COMPONENT_GUI(input, c, g->entities()->get(20 + i));
			c.min.f = 0.1;
			c.max.f = 10.0;
			c.step.f = 0.1;
		}
	}

	class callbacksInitClass
	{
		eventListener<void()> engineInitListener;
		eventListener<void()> engineUpdateListener;
	public:
		callbacksInitClass()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInitialize>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
