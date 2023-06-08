#include "common.h"

#include <cage-core/string.h>
#include <cage-engine/guiBuilder.h>
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

	EventListener<bool(const GenericInput &)> guiListener;

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

	const auto engineInitListener = controlThread().initialize.listen(
		[]()
		{
			Holder<GuiBuilder> g = newGuiBuilder(engineGuiEntities());

			guiListener.attach(engineGuiManager()->widgetEvent);
			guiListener.bind(inputListener<InputClassEnum::GuiWidget, InputGuiWidget>(&guiEvent));

			auto _1 = g->alignment(Vec2());
			auto _2 = g->spoiler().text("Ships");
			auto _3 = g->verticalTable(2);

			static_assert(sizeof(propertyNames) / sizeof(propertyNames[0]) == sizeof(propertyValues) / sizeof(propertyValues[0]), "arrays must have same length");
			for (uint32 i = 0; i < sizeof(propertyNames) / sizeof(propertyNames[0]); i++)
			{
				g->label().text(propertyNames[i]);
				g->setNextName(20 + i).input(*propertyValues[i], 0, 0.05, 0.0002);
			}

			for (uint32 i : { 5, 6 })
			{
				// radiuses
				GuiInputComponent &c = engineGuiEntities()->get(20 + i)->value<GuiInputComponent>();
				c.min.f = 0.1;
				c.max.f = 10.0;
				c.step.f = 0.1;
			}
		});
}
