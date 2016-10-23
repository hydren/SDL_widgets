/*
 * list_win.cpp
 *
 *  Created on: 14 de jun de 2016
 *      Author: carlosfaruolo
 */

#include "list_win.hpp"
#include "scrollable_pane.hpp"

using std::string;
using WidgetsExtra::AbstractListWin;

static SDL_Color white = {255, 255, 255, 0};

AbstractListWin::AbstractListWin(WinBase* parent, Style style, Rect rect, Id id)
: BgrWin(parent, rect, null, drawBgrWinAsListWin, onMouseDown, onMouseMove, onMouseUp, calc_color(0xffffffff), id),
  style(style), padding(1,1), spacing(1),
  textRenderer(null), textRendererCaseSelected(null),
  selection(), preventRedrawOnClick(false), enableScrollingIfScrollablePaneParent(true), lastClickedIndex(0)
{
	if(this->style.param == 0)
		this->style.param = 0xffA0D0E0;

	if(draw_white_ttf == null) draw_white_ttf = new RenderText(draw_title_ttf->ttf_font, white);
}

AbstractListWin::~AbstractListWin() {}

void AbstractListWin::draw()
{
	this->init_gui();
	SDL_FillRect(this->win, null, this->bgcol);

	RenderText* textRenderer = getResolvedTextRenderer(), *textRendererCaseSelected = getResolvedTextRendererCaseSelected();
	const unsigned lineHeight = TTF_FontHeight(textRenderer->ttf_font);

	SDL_Rect selectedLineRect = SDL_Rect(); //area to draw selected item background
	selectedLineRect.w = this->tw_area.w; selectedLineRect.h = lineHeight;

	Point itemLocation = this->padding; //start after padding

	for(unsigned index = 0; index < this->size(); index++)
	{
		string itemStr = this->getAsString(index);
		if(selection.isSelected(index))
		{
			selectedLineRect.y = itemLocation.y;
			SDL_FillRect(this->win, &selectedLineRect, calc_color(style.param));
			textRendererCaseSelected->draw_string(this->win, itemStr.c_str(), itemLocation);
		}
		else
			textRenderer->draw_string(this->win, itemStr.c_str(), itemLocation);

		itemLocation.y += lineHeight + this->spacing;
	}
}

unsigned AbstractListWin::getListHeight()
{
	RenderText* textRenderer = getResolvedTextRenderer();
	return this->padding.y + this->size() * (TTF_FontHeight(textRenderer->ttf_font) + this->spacing);
}

void AbstractListWin::clickList(const Point& point)
{
	 //if out of bounds or empty list, don't do anything
	if(point.x < padding.x or point.y < padding.y or this->size() == 0) return;

	//infer index by mouse position
	RenderText* textRenderer = getResolvedTextRenderer();
	unsigned index = (point.y - padding.y) / (TTF_FontHeight(textRenderer->ttf_font) + spacing);

	//set selected item if inside range. if not, clear and leave
	if(index > this->size() - 1)
	{
		this->selection.clear();
		return;
	}

	if(SDL_GetModState() & KMOD_CTRL) // check if click-wise selection
	{
		if(SDL_GetModState() & KMOD_SHIFT) // check if click-wise, multi-selection
		{
			if(index > lastClickedIndex)
				this->selection.select(lastClickedIndex, index); // set range selected
			else
				this->selection.select(index, lastClickedIndex); // set range selected (inverted)
		}
		else // click-wise selection only
		{
			this->selection.toogle(index);
			this->lastClickedIndex = index;
		}
	}

	else if(SDL_GetModState() & KMOD_SHIFT) // check if multi selection
	{
		if(index > lastClickedIndex)
			this->selection.setSelected(lastClickedIndex, index); // set range selected
		else
			this->selection.setSelected(index, lastClickedIndex); // set range selected (inverted)
	}
	else // single selection
	{
		this->selection.setSelected(index);
		this->lastClickedIndex = index;
	}
}

void AbstractListWin::onMouseDown(Point point, int buttonNumber)
{
	if(this->enableScrollingIfScrollablePaneParent) ScrollablePane::scrollContentOnMouseWheel(this, buttonNumber);
	if(buttonNumber != SDL_BUTTON_LEFT) return; // only accepts left-button clicks
	this->clickList(point);
	if(preventRedrawOnClick) return;
	this->draw_blit_upd(); // redraw
}

void AbstractListWin::onMouseMove(Point point, int buttonNumber) {}  // by default does nothing

void AbstractListWin::onMouseUp(Point point, int buttonNumber) {}  // by default does nothing

RenderText* AbstractListWin::getResolvedTextRenderer()
{
	if(this->textRenderer != null) return textRenderer;
	else switch(this->style.st)
	{
		default:
		case 0: return draw_ttf;
		case 1: return draw_mono_ttf;
		case 2: return draw_blue_ttf;
		case 3: return draw_title_ttf;
		case 4: return draw_white_ttf;
	}
}

RenderText* AbstractListWin::getResolvedTextRendererCaseSelected()
{
	if(this->textRendererCaseSelected != null) return textRendererCaseSelected;
	else switch(this->style.param2)
	{
		default:
		case 0: return draw_blue_ttf;
		case 1: return draw_mono_ttf;
		case 2: return draw_ttf;
		case 3: return draw_title_ttf;
		case 4: return draw_white_ttf;
	}
}

RenderText* AbstractListWin::draw_white_ttf = null;

// ################# static functions to be used when referencing AbstractListWin's as a BgrWin's.  #################
void AbstractListWin::drawBgrWinAsListWin(BgrWin* listWinAsBgrWin) 				{ static_cast<AbstractListWin*>(listWinAsBgrWin)->draw(); }
void AbstractListWin::onMouseDown(BgrWin* listWinAsBgrWin,int x,int y,int but) 	{ static_cast<AbstractListWin*>(listWinAsBgrWin)->onMouseDown(Point(x, y), but); }
void AbstractListWin::onMouseMove(BgrWin* listWinAsBgrWin,int x,int y,int but) 	{ static_cast<AbstractListWin*>(listWinAsBgrWin)->onMouseMove(Point(x, y), but); }
void AbstractListWin::onMouseUp(BgrWin* listWinAsBgrWin,int x,int y,int but) 	{ static_cast<AbstractListWin*>(listWinAsBgrWin)->onMouseUp(Point(x, y), but); }

