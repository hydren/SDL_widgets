/*
 * icon_button.hpp
 *
 *  Created on: 29 de set de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_ICON_BUTTON_HPP_
#define WIDGETS_ICON_BUTTON_HPP_
#include <ciso646>

#include "SDL_widgets.h"
#include "util.hpp"

#include "toogle_button.hpp"

namespace WidgetsExtra
{
	struct IconLabel
	{
		// pass one of this modes to style.param2
		enum IconMode { ICON_CENTER, ICON_LEFT, ICON_RIGHT, ICON_CENTER_UP, ICON_CENTER_DOWN, ICON_LEFT_UP, ICON_LEFT_DOWN, ICON_RIGHT_UP, ICON_RIGHT_DOWN };

		SDL_Surface* image;

		IconLabel(SDL_Surface* iconImage);
		virtual ~IconLabel();

		virtual void drawOn(WinBase* wb, IconMode=ICON_CENTER);
		virtual void packButton(Button* btn, int height=0);
	};

	struct IconButton extends Button
	{
		IconLabel icon;

		IconButton(WinBase *parent, Style style, Rect bounds, Label label, SDL_Surface* icon, void (*cmd)(Button*), Id id=0);
		virtual ~IconButton(){}

		virtual void draw();
	};

	struct IconToogleButton extends ToogleButton
	{
		IconLabel icon;

		IconToogleButton(WinBase *parent, Style style, Rect bounds, Label label, SDL_Surface* icon, void (*_cmd)(CheckBox*), Id id=0);
		virtual ~IconToogleButton(){}

		virtual void draw();
	};
}

#endif /* WIDGETS_ICON_BUTTON_HPP_ */
