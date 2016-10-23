/*
 * icon_button.cpp
 *
 *  Created on: 29 de set de 2016
 *      Author: carlosfaruolo
 */

#include "icon_button.hpp"

#include "widgets_util.hpp"

using WidgetsExtra::IconLabel;
using WidgetsExtra::IconButton;
using WidgetsExtra::IconToogleButton;

IconButton::IconButton(WinBase *parent, Style style, Rect bounds, Label label, SDL_Surface* icon, void (*cmd)(Button*), Id id)
: Button(parent, style, bounds, label, cmd, id),
  icon(icon)
{}

void IconButton::draw()
{
	init_gui();
	Button::draw();
	icon.drawOn(this, static_cast<IconLabel::IconMode>(style.param2));
}

IconToogleButton::IconToogleButton(WinBase *parent, Style style, Rect bounds, Label label, SDL_Surface* icon, void (*cmd)(CheckBox*), Id id)
: ToogleButton(parent, style, bounds, label, cmd, id),
  icon(icon)
{}

void IconToogleButton::draw()
{
	init_gui();
	ToogleButton::draw();
	icon.drawOn(this, static_cast<IconLabel::IconMode>(style.param2));
}

IconLabel::IconLabel(SDL_Surface* iconImage)
: image(iconImage)
{}

IconLabel::~IconLabel()
{
	SDL_FreeSurface(this->image);
}

void IconLabel::drawOn(WinBase* wb, IconMode mode)
{
	if(image == null or wb == null) return;

	SDL_Rect dstrect;
	if(mode == ICON_CENTER || mode == ICON_CENTER_DOWN || mode == ICON_CENTER_UP)
		dstrect.x = (wb->tw_area.w - image->w)/2;
	else if(mode == ICON_RIGHT || mode == ICON_RIGHT_DOWN || mode == ICON_RIGHT_UP)
		dstrect.x = wb->tw_area.w - image->w;

	if(mode == ICON_CENTER || mode == ICON_LEFT || mode == ICON_RIGHT)
		dstrect.y = (wb->tw_area.h - image->h)/2;
	else if(mode == ICON_CENTER_DOWN || mode == ICON_LEFT_DOWN || mode == ICON_RIGHT_DOWN)
		dstrect.y = wb->tw_area.h - image->h;

	SDL_BlitSurface(image, null, wb->win, &dstrect);
}

void IconLabel::packButton(Button* btn, int height)
{
	if(image == null or btn == null) return;

	if(btn->label.render_t->text_width(btn->label.str) < image->w)
	{
		int properWidth = image->w + 4;
		int properHeight = height != 0? height : TTF_FontHeight(btn->label.render_t->ttf_font) + 4;
		btn->widen(properWidth - btn->tw_area.w, properHeight - btn->tw_area.h);
	}
	else
		packLabeledComponent(btn, height);
}
