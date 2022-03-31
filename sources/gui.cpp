#include "common.h"

#include <cage-core/string.h>
#include <cage-engine/guiComponents.h>
#include <cage-engine/guiManager.h>

#include <initializer_list>

namespace
{
	constexpr Real *propertyValues[] = {
		&shipTargetShips,
		&shipTargetPlanets,
		&shipSeparation,
		&shipCohesion,
		&shipAlignment,
		&shipDetectRadius,
		&shipLaserRadius,
	};

	constexpr const String propertyNames[] = {
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

	InputListener<InputClassEnum::GuiWidget, InputGuiWidget> guiListener;

	void guiEvent(InputGuiWidget in)
	{
		uint32 index = in.widget - 20;
		if (index < sizeof(propertyValues) / sizeof(propertyValues[0]))
		{
			Entity *e = engineGuiEntities()->get(in.widget);
			GuiInputComponent &input = e->value<GuiInputComponent>();
			if (input.valid)
				*propertyValues[index] = toFloat(input.value);
		}
	}

	void engineInitialize()
	{
		GuiManager *g = engineGuiManager();
		guiListener.attach(g->widgetEvent);
		guiListener.bind<&guiEvent>();

		Entity *topLeft = g->entities()->createUnique();
		{
			GuiScrollbarsComponent &sc = topLeft->value<GuiScrollbarsComponent>();
		}

		Entity *table = g->entities()->createUnique();
		{
			GuiSpoilerComponent &c = table->value<GuiSpoilerComponent>();
			GuiLayoutTableComponent &l = table->value<GuiLayoutTableComponent>();
			GuiParentComponent &child = table->value<GuiParentComponent>();
			child.parent = topLeft->name();
			GuiTextComponent &t = table->value<GuiTextComponent>();
			t.value = "Ships";
		}

		static_assert(sizeof(propertyNames) / sizeof(propertyNames[0]) == sizeof(propertyValues) / sizeof(propertyValues[0]), "arrays must have same length");
		for (uint32 i = 0; i < sizeof(propertyNames) / sizeof(propertyNames[0]); i++)
		{
			Entity *lab = g->entities()->createUnique();
			{
				GuiParentComponent &child = lab->value<GuiParentComponent>();
				child.parent = table->name();
				child.order = i * 2 + 0;
				GuiLabelComponent &c = lab->value<GuiLabelComponent>();
				GuiTextComponent &t = lab->value<GuiTextComponent>();
				t.value = propertyNames[i];
			}
			Entity *con = g->entities()->create(20 + i);
			{
				GuiParentComponent &child = con->value<GuiParentComponent>();
				child.parent = table->name();
				child.order = i * 2 + 1;
				GuiInputComponent &c = con->value<GuiInputComponent>();
				c.type = InputTypeEnum::Real;
				c.min.f = 0;
				c.max.f = 0.05;
				c.step.f = 0.0002;
				c.value = Stringizer() + *propertyValues[i];
			}
		}

		for (uint32 i : { 5, 6 })
		{
			// radiuses
			GuiInputComponent &c = g->entities()->get(20 + i)->value<GuiInputComponent>();
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
