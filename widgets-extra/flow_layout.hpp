/*
 * flow_layout.hpp
 *
 *  Created on: 28 de mar de 2016
 *      Author: carlosfaruolo
 */

#ifndef FLOW_LAYOUT_HPP_
#define FLOW_LAYOUT_HPP_

#include "SDL_widgets.h"

#include "abstract_layout.hpp"

namespace WidgetsExtra
{
	/** A layout that places its elements side by side, with a minimum spacing.
	 *  If it contains a stretching component, the component is stretch until reach this layout maxSize.
	 *  If maxSize is zero, the layout uses all the space after its position.*/
	struct FlowLayout extends Layout
	{
		static const unsigned DEFAULT_SPACING = 4;

		//The max size that this layout expands to.
		//If w==0, then it goes until the right window border. If h==0, then it goes until the window bottom.
		unsigned maxWidth, maxHeight;

		//the spacing between components. default is DEFAULT_SPACING.
		unsigned spacing_h, spacing_v;

		//the alignment of the components on the perpendicular axis.
		enum Alignment {BEFORE, MIDDLE, AFTER} alignment;

		Orientation orientation;

		FlowLayout(int x, int y, unsigned w=0, unsigned h=0);

		FlowLayout(Point p);

		virtual ~FlowLayout();

		void pack();

		protected:
		void packHorizontal();
		void packVertical();
		bool needsStretching() const; //check if there is a stretching element
		unsigned computeFreeHorizontalSpaceOnLayout() const; //computes the free space to stretch-out
		unsigned computeFreeVerticalSpaceOnLayout() const; //computes the free space to stretch-out
		unsigned getStretchingElementsCount() const; //counts the number of stretching elements
		int computeAlignment(Element* e, int pos) const;
	};
}

#endif /* FLOW_LAYOUT_HPP_ */
