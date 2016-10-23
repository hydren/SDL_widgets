/*
 * scrollable_pane.cpp
 *
 *  Created on: 17 de mai de 2016
 *      Author: carlosfaruolo
 */

#include "scrollable_pane.hpp"

#include "widgets_util.hpp"

using WidgetsExtra::ScrollablePane;

ScrollablePane::ScrollablePane(WinBase* parent, Style style, Rect bounds, Uint32 bgColor, Id id)
: WinBase(parent, null, bounds.x, bounds.y, bounds.w, bounds.h, bgColor, id),
  offset(0, 0),
  scrollbarThickness(style.param != 0 ? style.param : ScrollablePane::DEFAULT_SCROLLBAR_THICKNESS),
  content(this, Rect(0, 0, bounds.w - scrollbarThickness, bounds.h - scrollbarThickness), null, WidgetsExtra::drawBgrWin, ScrollablePane::scrollContentOnMouseWheel, null, null, bgColor, id),
  scrollbarHorizontal(this, Style(1,style.st,5), Rect(0, bounds.h - scrollbarThickness, bounds.w - scrollbarThickness, scrollbarThickness), content.tw_area.w, ScrollablePane::hscrollbarCallback, id),
  scrollbarVertical(this, Style(1,style.st,5), Rect(bounds.w - scrollbarThickness, 0, scrollbarThickness, bounds.h - scrollbarThickness), content.tw_area.h, ScrollablePane::vscrollbarCallback, id),
  scrollingSpeedHorizontal(scrollbarHorizontal.style.param2),
  scrollingSpeedVertical(scrollbarVertical.style.param2),
  decorativeMask(this, Rect(scrollbarVertical.area.x, scrollbarHorizontal.area.y, scrollbarThickness, scrollbarThickness), null, WidgetsExtra::drawBgrWin, null, null, null, bgColor)
{}

ScrollablePane::~ScrollablePane()
{}

void ScrollablePane::draw()
{
	this->init_gui();
	SDL_FillRect(this->win, null, this->bgcol); //clears the screen
}

void ScrollablePane::refresh()
{
	this->draw_blit_recur();
	this->hidden = true;
	this->show();
}

void ScrollablePane::updateOffset(int dx, int dy)
{
	offset.x += dx; offset.y += dy;
	content.move(dx, dy);
	this->refresh();
}

void ScrollablePane::setOffset(int x, int y)
{
	this->updateOffset(x - offset.x, y - offset.y);
}

void ScrollablePane::widen(int dx, int dy)
{
	WinBase::widen(dx, dy);
	scrollbarVertical.move(dx, 0); scrollbarVertical.widen(0, dy);
	scrollbarHorizontal.move(0, dy); scrollbarHorizontal.widen(dx, 0);
	decorativeMask.move(dx, dy);
}

void ScrollablePane::widenContent(int dx, int dy)
{
	content.widen(dx, dy);
	scrollbarHorizontal.set_range(content.tw_area.w);
	scrollbarVertical.set_range(content.tw_area.h);

	// stabilize scrollbars
	if(content.tw_area.h + offset.y < this->tw_area.h)
	{
		const int diff = content.tw_area.h + offset.y - this->tw_area.h;
		scrollbarVertical.p0 = 0;
		scrollbarVertical.calc_ypos(diff);
	}
	if(content.tw_area.w + offset.x < this->tw_area.w)
	{
		const int diff = content.tw_area.w + offset.x - this->tw_area.w;
		scrollbarHorizontal.p0 = 0;
		scrollbarHorizontal.calc_xpos(diff);
	}
}

void ScrollablePane::setScrollbarVerticalVisible(bool visibleDesired)
{
	if(visibleDesired && scrollbarVertical.hidden)
	{
		scrollbarVertical.hidden = false;
		scrollbarHorizontal.widen(-scrollbarThickness, 0);
		scrollbarHorizontal.set_range(scrollbarVertical.range);
	}
	else if(not visibleDesired && not scrollbarVertical.hidden)
	{
		scrollbarVertical.hidden = true;
		scrollbarHorizontal.widen(scrollbarThickness, 0);
		scrollbarHorizontal.set_range(scrollbarVertical.range);
	}
}

void ScrollablePane::setScrollbarHorizontalVisible(bool visibleDesired)
{
	if(visibleDesired && scrollbarHorizontal.hidden)
	{
		scrollbarHorizontal.hidden = false;
		scrollbarVertical.widen(0, -scrollbarThickness);
		scrollbarVertical.set_range(scrollbarVertical.range);
	}
	else if(not visibleDesired && not scrollbarVertical.hidden)
	{
		scrollbarHorizontal.hidden = true;
		scrollbarVertical.widen(0, scrollbarThickness);
		scrollbarVertical.set_range(scrollbarVertical.range);
	}
	decorativeMask.hidden = (scrollbarHorizontal.hidden or scrollbarVertical.hidden);
}

void ScrollablePane::scrollContentOnMouseWheel(WinBase* component, int but)
{
	if(component == null) return;

	while(component->parent != null and dynamic_cast<ScrollablePane*>(component->parent)==null)
		component = component->parent;

	ScrollablePane* self = dynamic_cast<ScrollablePane*>(component->parent);
	if(self != null)
	{
		if(but == SDL_BUTTON_WHEELUP)
			self->scrollbarVertical.inc_value(false);
		else if(but == SDL_BUTTON_WHEELDOWN)
			self->scrollbarVertical.inc_value(true);
	}
}

void ScrollablePane::hscrollbarCallback(HScrollbar* bar, int val, int range)
{
	ScrollablePane* self = static_cast<ScrollablePane*>(bar->parent);
	self->setOffset(-val, self->offset.y);
}


void ScrollablePane::vscrollbarCallback(VScrollbar* bar, int val, int range)
{
	ScrollablePane* self = static_cast<ScrollablePane*>(bar->parent);
	self->setOffset(self->offset.x, -val);
}

void ScrollablePane::scrollContentOnMouseWheel(BgrWin* content, int x, int y, int but)
{
	ScrollablePane* self = static_cast<ScrollablePane*>(content->parent);
	if(but == SDL_BUTTON_WHEELUP)
		self->scrollbarVertical.inc_value(false);
	else if(but == SDL_BUTTON_WHEELDOWN)
		self->scrollbarVertical.inc_value(true);
}
