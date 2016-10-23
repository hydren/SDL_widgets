/*
 * abstract_layout.cpp
 *
 *  Created on: 15 de abr de 2016
 *      Author: carlosfaruolo
 */

#include "abstract_layout.hpp"

#include "util.hpp"

#include <typeinfo>

using std::vector;
using WidgetsExtra::Layout;

// WinBaseWrapper =================================================================

Layout::WinBaseWrapper::WinBaseWrapper(WinBase* winBase)
: base(winBase)
{}

Layout::WinBaseWrapper::~WinBaseWrapper()
{}

Point Layout::WinBaseWrapper::getPosition() const
{
	return base->area;
}

void Layout::WinBaseWrapper::setPosition(Point position)
{
	setWinBasePosition(this->base, position);
}

void Layout::WinBaseWrapper::setPosition(int x, int y)
{
	setX(x); setY(y);
}

void Layout::WinBaseWrapper::setX(int x)
{
	setWinBasePositionX(this->base, x);
}

void Layout::WinBaseWrapper::setY(int y)
{
	setWinBasePositionY(this->base, y);
}

Rect Layout::WinBaseWrapper::getSize() const
{
	return base->tw_area;
}

void Layout::WinBaseWrapper::setSize(Rect size)
{
	base->widen(size.w - base->tw_area.w, size.h - base->tw_area.h);
}

bool Layout::WinBaseWrapper::isStretched() const
{
	return false;
}

bool Layout::WinBaseWrapper::operator==(const Element& someElement) const
{
	if(typeid(someElement) != typeid(WinBaseWrapper))
		return false;

	else if(this->base != ((const WinBaseWrapper&) someElement).base)
		return false;

	else return true;
}

void Layout::WinBaseWrapper::setWinBasePosition(WinBase* wb, Point pos)
{
	setWinBasePositionX(wb, pos.x);
	setWinBasePositionY(wb, pos.y);
}

void Layout::WinBaseWrapper::setWinBasePositionX(WinBase* wb, int x)
{
	wb->area.x = x;
	wb->tw_area.x = x;
	wb->title_area.x = x;
	wb->title_top.x = x;
	if (wb->parent)
	{
		wb->tw_area.x=wb->area.x+wb->parent->tw_area.x;
	}
}

void Layout::WinBaseWrapper::setWinBasePositionY(WinBase* wb, int y)
{
	wb->area.y = y;
	wb->tw_area.y = y;
	wb->title_area.y = y-17;
	wb->title_top.y = y-17;
	if (wb->parent)
	{
		wb->tw_area.y=wb->area.y+wb->parent->tw_area.y;
	}
}

// Spacer ========================================================================================

Layout::Spacer::Spacer(Layout* layout, Rect size)
: bounds(layout->position.x, layout->position.y, size.w, size.h),
  stretched(size.w == 0 && size.h == 0)
{}

Layout::Spacer::Spacer(Rect bounds)
: bounds(bounds), stretched(bounds.w == 0 && bounds.h == 0)
{}

Layout::Spacer::~Spacer() {}

Point Layout::Spacer::getPosition() const
{
	return Point(this->bounds.x, this->bounds.y);
}

void Layout::Spacer::setPosition(Point position)
{
	this->bounds.x = position.x;
	this->bounds.y = position.y;
}

Rect Layout::Spacer::getSize() const
{
	return Rect(0, 0, this->bounds.w, this->bounds.h);
}

void Layout::Spacer::setSize(Rect size)
{
	this->bounds.w = size.w;
	this->bounds.h = size.h;
}

bool Layout::Spacer::isStretched() const
{
	return this->stretched;
}

bool Layout::Spacer::operator == (const Element& b) const
{
	return &b == this;
}

// Separator =====================================================================================

Layout::Separator::Separator(WinBase* parent, Orientation orient, unsigned size, unsigned thick)
: WinBase(parent, null, 0, 0, orient == VERTICAL? size : thick!=0? thick : size/4, orient == HORIZONTAL? size : thick!=0? thick : size/4, parent->bgcol, 0),
  Layout::WinBaseWrapper(this),
  orientation(orient)
{}

#include <iostream>
void Layout::Separator::draw()
{
	this->init_gui();
	SDL_FillRect(this->win, null, this->bgcol); //clears the screen
	if(orientation == HORIZONTAL)
	{
		filledCircleColor(this->win, this->tw_area.w * 0.5, this->tw_area.h * (0.250), 1, 0x606060ff);
		filledCircleColor(this->win, this->tw_area.w * 0.5, this->tw_area.h * (0.375), 1, 0x606060ff);
		filledCircleColor(this->win, this->tw_area.w * 0.5, this->tw_area.h * (0.500), 1, 0x606060ff);
		filledCircleColor(this->win, this->tw_area.w * 0.5, this->tw_area.h * (0.625), 1, 0x606060ff);
		filledCircleColor(this->win, this->tw_area.w * 0.5, this->tw_area.h * (0.750), 1, 0x606060ff);
	}
	else
	{
		filledCircleColor(this->win, this->tw_area.w * (0.250), this->tw_area.h * 0.5, 1, 0x606060ff);
		filledCircleColor(this->win, this->tw_area.w * (0.375), this->tw_area.h * 0.5, 1, 0x606060ff);
		filledCircleColor(this->win, this->tw_area.w * (0.500), this->tw_area.h * 0.5, 1, 0x606060ff);
		filledCircleColor(this->win, this->tw_area.w * (0.625), this->tw_area.h * 0.5, 1, 0x606060ff);
		filledCircleColor(this->win, this->tw_area.w * (0.750), this->tw_area.h * 0.5, 1, 0x606060ff);
	}
}

// Layout ========================================================================================

Layout::Layout(int x, int y)
{
	this->position.x = x;
	this->position.y = y;
}

Layout::Layout(Point& p)
{
	this->position = p;
}

Layout::~Layout(){}

void Layout::addComponent(WinBase& base, int index)
{
	addComponent(&base, index);
}

void Layout::addComponent(WinBase* base, int index)
{
	WinBaseWrapper* component = new WinBaseWrapper(base); //creater wrapper for winbase
	innerWrappers.push_back(component); //register the layout-created wrapper
	addComponent(static_cast<Element*>(component), index);
}

void Layout::addComponent(Element& component, int index)
{
	addComponent(&component, index);
}

void Layout::addComponent(Element* component, int index)
{
	if(index < 0)
		this->components.push_back(component);
	else
		this->components.insert(components.begin()+index, component);
}

/** Returns a pointer to the component at the specified index */
Layout::Element* Layout::getComponentAt(unsigned index)
{
	if(index > this->components.size() - 1)
		throw std::out_of_range("out of range: " + index);

	return this->components[index];
}

/** Returns a pointer to the wrapper component corresponding to the given WinBase */
Layout::WinBaseWrapper* Layout::getWrapperComponent(WinBase* base)
{
	for(unsigned i = 0; i < this->components.size(); i++)
	{
		WinBaseWrapper* wrapper = dynamic_cast<WinBaseWrapper*>(this->components.at(i));
		if(wrapper != null && wrapper->base == base)
			return wrapper;
	}
	return null;
}

void Layout::removeComponentAt(unsigned index)
{
	if(index > this->components.size() - 1)
		throw std::out_of_range("out of range: " + index);

	//if there is a pointer to the element on innerWrappers, then it is a WinBaseWrapper created by this layout
	for(int w = index_of(this->innerWrappers, static_cast<WinBaseWrapper*>(this->components[index])); w != -1; w = -1)
	{
		delete this->innerWrappers[w]; //delete the wrapper
		this->innerWrappers.erase(this->innerWrappers.begin() + w); //remove the pointer
	}

	//remove the pointer
	this->components.erase(components.begin() + index);
}

void Layout::removeComponent(WinBase& component)
{
	Layout::removeComponent(&component);
}

void Layout::removeComponent(WinBase* component)
{
	for(unsigned i = 0; i < this->components.size(); i++)
	{
		WinBaseWrapper* wrapper = dynamic_cast<WinBaseWrapper*>(this->components.at(i));
		if(wrapper != null && wrapper->base == component)
		{
			Layout::removeComponentAt(i);
			break;
		}
	}
}

void Layout::removeComponent(Element& component)
{
	Layout::removeComponent(&component);
}

void Layout::removeComponent(Element* component)
{
	int index = index_of(this->components, component);
	if(index < 0) return;
	Layout::removeComponentAt(index);
}


/** Returns the number of components on this layout. */
unsigned Layout::getComponentCount() const
{
	return this->components.size();
}


/** Hides all components on this layout by calling their hide() method. */
void Layout::hideAll()
{
	for(unsigned i = 0; i < this->components.size(); i++)
	{
		WinBaseWrapper* wrapper = dynamic_cast<WinBaseWrapper*>(this->components.at(i));
		if(wrapper != null)
		{
			wrapper->base->hide();
		}
	}
}

/** Show all components on this layout by calling their show() method. This should be called after a hideAll() call to make its components visible again. */
void Layout::showAll()
{
	for(unsigned i = 0; i < this->components.size(); i++)
	{
		WinBaseWrapper* wrapper = dynamic_cast<WinBaseWrapper*>(this->components.at(i));
		if(wrapper != null)
		{
			wrapper->base->show();
		}
	}
}


