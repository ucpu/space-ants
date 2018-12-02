#include "common.h"

#include <cage-client/gui.h>

namespace
{
	real *propertyValues[] = {
		&shipSeparation,
		&shipSteadfast,
		&shipCohesion,
		&shipAlignment,
		&shipDetectRadius,
	};

	const string propertyNames[] = {
		"Separation: ",
		"Steadfast: ",
		"Cohesion: ",
		"Alignment: ",
		"Detect Radius: ",
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
			entityClass *e = gui()->entities()->get(name);
			GUI_GET_COMPONENT(input, input, e);
			if (input.valid)
				*propertyValues[index] = input.value.toFloat();
		}
	}

	void engineInitialize()
	{
		guiClass *g = cage::gui();
		guiListener.attach(g->widgetEvent);
		guiListener.bind<&guiEvent>();

		entityClass *topLeft = g->entities()->createUnique();
		{
			GUI_GET_COMPONENT(scrollbars, sc, topLeft);
		}

		entityClass *table = g->entities()->createUnique();
		{
			GUI_GET_COMPONENT(panel, c, table);
			GUI_GET_COMPONENT(layoutTable, l, table);
			GUI_GET_COMPONENT(parent, child, table);
			child.parent = topLeft->name();
		}

		CAGE_ASSERT_COMPILE(sizeof(propertyNames) / sizeof(propertyNames[0]) == sizeof(propertyValues) / sizeof(propertyValues[0]), arrays_must_have_same_length);
		for (uint32 i = 0; i < sizeof(propertyNames) / sizeof(propertyNames[0]); i++)
		{
			entityClass *lab = g->entities()->createUnique();
			{
				GUI_GET_COMPONENT(parent, child, lab);
				child.parent = table->name();
				child.order = i * 2 + 0;
				GUI_GET_COMPONENT(label, c, lab);
				GUI_GET_COMPONENT(text, t, lab);
				t.value = propertyNames[i];
			}
			entityClass *con = g->entities()->create(20 + i);
			{
				GUI_GET_COMPONENT(parent, child, con);
				child.parent = table->name();
				child.order = i * 2 + 1;
				GUI_GET_COMPONENT(input, c, con);
				c.type = inputTypeEnum::Real;
				c.min.f = 0;
				c.max.f = 0.1;
				c.step.f = 0.001;
				c.value = *propertyValues[i];
			}
		}

		{
			// detection radius
			GUI_GET_COMPONENT(input, c, g->entities()->get(20 + 4));
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
