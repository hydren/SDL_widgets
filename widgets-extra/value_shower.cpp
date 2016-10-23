/*
 * value_shower.cpp
 *
 *  Created on: 22 de ago de 2016
 *      Author: felipe
 */

#include "value_shower.hpp"

#include <typeinfo>

using WidgetsExtra::ValueShower;

ValueShower::ValueShower(WinBase* pw, Style style, Rect size, const char* lab)
: Message(pw, style, lab, Point(size.x, size.y)),
  Element(),
  size(size)
{}

ValueShower::ValueShower(WinBase* pw, Style style, Point pt, unsigned n_characters, const char* lab)
: Message(pw, style, lab, pt),
  Element(),
  size(Rect(pt.x, pt.y, 0, TTF_FontHeight(style.st==2? draw_ttf->ttf_font: draw_title_ttf->ttf_font)))
{
	RenderText* rt = style.st==2? draw_ttf : draw_title_ttf;
	char* tmp = new char[n_characters];
	for(unsigned i = 0; i < n_characters-1; i++) tmp[i] = '_';
	tmp[n_characters-1] = '\0';
	size.w = rt->text_width(tmp);
	delete [] tmp;
}

Point ValueShower::getPosition() const
{
	return this->lab_pt;
}

void ValueShower::setPosition(Point position)
{
	this->lab_pt = position;
	this->mes_r.y = position.y;
	this->mes_r.x= label ? draw_ttf->text_width(label)+position.x+2 : position.x;
}

Rect ValueShower::getSize() const
{
	return size;
}

void ValueShower::setSize(Rect size)
{
	this->size = size;
}

bool ValueShower::isStretched() const
{
	return false;
}

bool ValueShower::operator ==(const Element& b) const
{
	if(typeid(b) != typeid(ValueShower))
		return false;

	else if(this != &((const ValueShower&) b))
		return false;

	else return true;
}
