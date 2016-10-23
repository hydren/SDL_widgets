/*
 * label_win.hpp
 *
 *  Created on: 19 de abr de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_LABEL_WIN_HPP_
#define WIDGETS_LABEL_WIN_HPP_

#include <string>
#include <vector>

#include "SDL_widgets.h"
#include "multi_line_text_renderer.hpp"

#include "util.hpp"

namespace WidgetsExtra
{
	struct LabelWin extends WinBase
	{
		std::string txt;
		RenderText* textRenderer;

		LabelWin(WinBase *pw, Rect bounds, std::string str, Id id=0);

		virtual ~LabelWin();
		virtual void draw();

		/** Safer than changing textRenderer directly, as this automatically adjusts size. */
		virtual void setTextRenderer(RenderText* txtrnd);
	};

	struct MultiLineLabelWin extends LabelWin
	{
		MultiLineTextRenderer multiRender;

		MultiLineLabelWin(WinBase *pw, Rect bounds, std::string str, unsigned padding=0, Id id=0);

		virtual ~MultiLineLabelWin();
		virtual void draw();

		// pseudo-overrides WinBase::widen()
		virtual void widen(int dx,int dy);

		/** Safer than changing textRenderer directly, as this automatically adjusts size. */
		virtual void setTextRenderer(RenderText* txtrnd);
	};
}

#endif /* WIDGETS_LABEL_WIN_HPP_ */
