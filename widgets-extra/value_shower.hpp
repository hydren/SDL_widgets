/*
 * value_shower.hpp
 *
 *  Created on: 22 de ago de 2016
 *      Author: felipe
 */

#ifndef WIDGETS_VALUE_SHOWER_HPP_
#define WIDGETS_VALUE_SHOWER_HPP_

#include "abstract_layout.hpp"

namespace WidgetsExtra
{
	struct ValueShower extends Message, Layout::Element
	{
		Rect size;

		ValueShower(WinBase* pw, Style style, Rect size, const char* lab);
		ValueShower(WinBase* pw, Style style, Point pt, unsigned n_characters, const char* lab);

		Point getPosition() const;
		void setPosition(Point position);
		Rect getSize() const;
		void setSize(Rect size);
		bool isStretched() const;
		bool operator == (const Element& b) const;
	};
}

#endif /* WIDGETS_VALUE_SHOWER_HPP_ */
