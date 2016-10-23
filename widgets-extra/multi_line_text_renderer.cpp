/*
 * multi_line_text_renderer.cpp
 *
 *  Created on: 6 de out de 2016
 *      Author: carlosfaruolo
 */

#include "multi_line_text_renderer.hpp"

#include "widgets_util.hpp"

using WidgetsExtra::MultiLineTextRenderer;
using std::string;
using std::vector;

MultiLineTextRenderer::MultiLineTextRenderer(RenderText* renderText, SDL_Surface* surface, Point position, unsigned padding)
: renderText(renderText), surface(surface), position(position), padding(padding), width(0)
{}

void MultiLineTextRenderer::draw()
{
	this->draw(this->surface, this->position, this->padding);
}

void MultiLineTextRenderer::draw(SDL_Surface* surface)
{
	this->draw(surface, this->position, this->padding);
}

void MultiLineTextRenderer::draw(SDL_Surface* surface, Point position, unsigned padding)
{
	const int lineHeight = TTF_FontHeight(this->renderText->ttf_font);
	int lineNumber = 0;
	foreach(string&, line, vector<string>, lines)
	{
		renderText->draw_string(surface, line.c_str(), Point(position.x + padding, position.y + padding + lineHeight * lineNumber++));
	}
}

unsigned MultiLineTextRenderer::getLineCount() const
{
	return this->lines.size();
}

unsigned MultiLineTextRenderer::getTextHeight() const
{
	return this->lines.size() * TTF_FontHeight(this->renderText->ttf_font) + this->padding;
}

unsigned MultiLineTextRenderer::getTextHeight(unsigned padding) const
{
	return this->lines.size() * TTF_FontHeight(this->renderText->ttf_font) + padding;
}

unsigned MultiLineTextRenderer::getTextWidth() const
{
	return width;
}

void MultiLineTextRenderer::breakLines(Uint16 maxWidth)
{
	this->lines.clear();
	WidgetsExtra::getLineWrappedText(this->fullText, this->renderText, maxWidth - 2*this->padding, &this->lines);
}

void MultiLineTextRenderer::setText(std::string text, Uint16 maxWidth)
{
	this->fullText = text;
	this->breakLines(maxWidth);
	this->width = maxWidth;
}
