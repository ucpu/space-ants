#include "common.h"

#include <cage-engine/gui.h>

#include <initializer_list>

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

	EventListener<void(uint32)> guiListener;

	void guiEvent(uint32 name)
	{
		uint32 index = name - 20;
		if (index < sizeof(propertyValues) / sizeof(propertyValues[0]))
		{
			Entity *e = engineGui()->entities()->get(name);
			CAGE_COMPONENT_GUI(Input, input, e);
			if (input.valid)
				*propertyValues[index] = input.value.toFloat();
		}
	}

	void engineInitialize()
	{
		Gui *g = cage::engineGui();
		guiListener.attach(g->widgetEvent);
		guiListener.bind<&guiEvent>();

		Entity *topLeft = g->entities()->createUnique();
		{
			CAGE_COMPONENT_GUI(Scrollbars, sc, topLeft);
		}

		Entity *table = g->entities()->createUnique();
		{
			CAGE_COMPONENT_GUI(Spoiler, c, table);
			CAGE_COMPONENT_GUI(LayoutTable, l, table);
			CAGE_COMPONENT_GUI(Parent, child, table);
			child.parent = topLeft->name();
			CAGE_COMPONENT_GUI(Text, t, table);
			t.value = "Ships";
		}

		static_assert(sizeof(propertyNames) / sizeof(propertyNames[0]) == sizeof(propertyValues) / sizeof(propertyValues[0]), "arrays must have same length");
		for (uint32 i = 0; i < sizeof(propertyNames) / sizeof(propertyNames[0]); i++)
		{
			Entity *lab = g->entities()->createUnique();
			{
				CAGE_COMPONENT_GUI(Parent, child, lab);
				child.parent = table->name();
				child.order = i * 2 + 0;
				CAGE_COMPONENT_GUI(Label, c, lab);
				CAGE_COMPONENT_GUI(Text, t, lab);
				t.value = propertyNames[i];
			}
			Entity *con = g->entities()->create(20 + i);
			{
				CAGE_COMPONENT_GUI(Parent, child, con);
				child.parent = table->name();
				child.order = i * 2 + 1;
				CAGE_COMPONENT_GUI(Input, c, con);
				c.type = InputTypeEnum::Real;
				c.min.f = 0;
				c.max.f = 0.05;
				c.step.f = 0.0002;
				c.value = stringizer() + *propertyValues[i];
			}
		}

		for (uint32 i : { 5, 6 })
		{
			// radiuses
			CAGE_COMPONENT_GUI(Input, c, g->entities()->get(20 + i));
			c.min.f = 0.1;
			c.max.f = 10.0;
			c.step.f = 0.1;
		}
	}

	class Callbacks
	{
		EventListener<void()> engineInitListener;
		EventListener<void()> engineUpdateListener;
	public:
		Callbacks()
		{
			engineInitListener.attach(controlThread().initialize);
			engineInitListener.bind<&engineInitialize>();
			engineUpdateListener.attach(controlThread().update);
			engineUpdateListener.bind<&engineUpdate>();
		}
	} callbacksInitInstance;
}
