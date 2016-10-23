/*
 * multi_line_text_renderer.hpp
 *
 *  Created on: 6 de out de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_MULTI_LINE_TEXT_RENDERER_HPP_
#define WIDGETS_MULTI_LINE_TEXT_RENDERER_HPP_
#include <ciso646>

#include <string>
#include <vector>

#include "SDL_widgets.h"

#include "util.hpp"

namespace WidgetsExtra
{
	struct MultiLineTextRenderer
	{
		RenderText* renderText;
		SDL_Surface* surface;
		Point position;
		int padding;

		MultiLineTextRenderer(RenderText* renderText=null, SDL_Surface* surface=null, Point position=Point(), unsigned padding=0);

		void draw();
		void draw(SDL_Surface* surface);
		void draw(SDL_Surface* surface, Point position, unsigned padding=0);

		unsigned getLineCount() const;

		unsigned getTextHeight() const;
		unsigned getTextHeight(unsigned padding) const;
		unsigned getTextWidth() const;

		void breakLines(Uint16 maxWidth);
		void setText(std::string text, Uint16 maxWidth);

		protected:
		std::vector<std::string> lines;
		std::string fullText;
		unsigned width;
	};
}

#endif /* WIDGETS_MULTI_LINE_TEXT_RENDERER_HPP_ */
