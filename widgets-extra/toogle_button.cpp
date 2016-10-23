/*
 * toogle_button.cpp
 *
 *  Created on: 29 de abr de 2016
 *      Author: felipe
 */

#include "toogle_button.hpp"

#include "widgets_util.hpp"

using WidgetsExtra::ToogleButton;

ToogleButton::ToogleButton(WinBase *pw,Style st,Rect rect,Label lab,void (*_cmd)(CheckBox*),Id _id)
: CheckBox(pw, st, rect, lab, _cmd, _id)
{}

void ToogleButton::draw()
{
	if (!win)
		win=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w,tw_area.h,16,0,0,0,0);

	static const Color5 *grad_colors[5]={
			&Color5::GradientBlue,&Color5::GradientGrey,&Color5::GradientGreen,&Color5::GradientWheat,&Color5::GradientRose
	};
	const Color5 *bcol=(style.param<5 && style.param>0 ? grad_colors[style.param] : grad_colors[0]);

	int y=tw_area.h/2-8;

	if (*d)
		draw_raised(0,bcol->c[2],false);
	else
		bcol->draw_gradient(this, Rect(0,0,tw_area.w,tw_area.h));
	label.draw(win,id.id2,Point(3,y));
}
