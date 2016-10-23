/*
 * tabs.cpp
 *
 *  Created on: 28 de abr de 2016
 *      Author: carlosfaruolo
 */

#include "tabs.hpp"

#include "widgets_util.hpp"

using WidgetsExtra::TabSet;
using WidgetsExtra::TabbedPane;
using WidgetsExtra::FlowLayout;

TabSet::TabSet(WinBase* parent, int x, int y, unsigned w, unsigned h)
: commonParent(parent),
  controller(),
  tabBtnLayout(x, y, w, h)
{}

void TabSet::addTab(Label lab, BgrWin* content)
{
	controller.addTab(commonParent, Rect(0,0,1,1), lab, content);
	RExtButton* tabBtn = controller.tabButtons.back();
	WidgetsExtra::packLabeledComponent(tabBtn);
	tabBtnLayout.addComponent(tabBtn);
	tabBtnLayout.pack();
}

void TabSet::setActiveTab(BgrWin* tabContent)
{
	controller.setActiveTab(tabContent);
}

void TabSet::setActiveTab(unsigned index)
{
	controller.setActiveTab(index);
}

void TabSet::widenAll(int dx, int dy)
{
	controller.widenAll(dx, dy);
}

void TabSet::drawTabStyleBgrWin(BgrWin* bgrWin)
{
	WidgetsExtra::drawBgrWin(bgrWin);
	lineColor(bgrWin->win, 2, 1, bgrWin->tw_area.w - 2, 1, 0x606060ff); //top line
	lineColor(bgrWin->win, 2, bgrWin->tw_area.h - 1, bgrWin->tw_area.w - 2, bgrWin->tw_area.h - 1, 0x606060ff); //bottom line
	lineColor(bgrWin->win, 1, 2, 1, bgrWin->tw_area.h - 2, 0x606060ff); //leftmost line
	lineColor(bgrWin->win, bgrWin->tw_area.w - 1, 2, bgrWin->tw_area.w - 1, bgrWin->tw_area.h - 2, 0x606060ff); //rightmost line
}

// :  :  :  :  :  :  tabbed pane  :  :  :  :  :  :  :  :  :  :

unsigned estimateButtonSize()
{
	Button btn(null, 0, Rect(0,0,1,1), "UPPERCASE_SAMPLE", null);
	WidgetsExtra::packLabeledComponent(&btn);
	return btn.tw_area.h;
}

TabbedPane::TabbedPane(WinBase* parent, Rect bounds, Id id)
: WinBase(parent, null, bounds.x, bounds.y, bounds.w, bounds.h, parent->bgcol, id),
  TabSet(this, 0, 0, bounds.w, estimateButtonSize())
{}

TabbedPane::~TabbedPane() {}

void TabbedPane::draw()
{
	init_gui();
	SDL_FillRect(this->win, null, this->bgcol);
}

void TabbedPane::widen(int dx, int dy)
{
	WinBase::widen(dx, dy);
	TabSet::widenAll(dx, dy);
}
