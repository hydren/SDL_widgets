/*
 * toogle_button.hpp
 *
 *  Created on: 29 de abr de 2016
 *      Author: felipe
 */

#ifndef WIDGETS_TOOGLE_BUTTON_HPP_
#define WIDGETS_TOOGLE_BUTTON_HPP_

#include "SDL_widgets.h"
#include "util.hpp"

namespace WidgetsExtra
{
	struct ToogleButton extends CheckBox
	{
		ToogleButton(WinBase *pw,Style st,Rect rect,Label lab,void (*_cmd)(CheckBox*),Id id=0);
		virtual void draw(); //draw() from base is not virtual, be careful
		virtual ~ToogleButton() {};
	};
}



#endif /* WIDGETS_TOOGLE_BUTTON_HPP_ */
