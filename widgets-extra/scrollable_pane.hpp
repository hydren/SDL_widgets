/*
 * scrollable_pane.hpp
 *
 *  Created on: 17 de mai de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_SCROLLABLE_PANE_HPP_
#define WIDGETS_SCROLLABLE_PANE_HPP_

#include "SDL_widgets.h"
#include "util.hpp"

namespace WidgetsExtra
{
	struct ScrollablePane extends WinBase
	{
		static const unsigned DEFAULT_SCROLLBAR_THICKNESS = 12;

		/// read-only attribute. use setOffset() to make a recursive offsetting.
		Point offset;

		/// read-only attribute. the scrollbar's thickness
		unsigned scrollbarThickness;

		/// The actually scrolled component. Add any childs to this BgrWin instead of adding to this ScrollablePane.
		BgrWin content;

		HScrollbar scrollbarHorizontal;
		VScrollbar scrollbarVertical;

		/// references the scrolling speed (in pixels) of each scrollbar
		int &scrollingSpeedHorizontal, &scrollingSpeedVertical;

		ScrollablePane(WinBase* parent, Style style, Rect bounds, Uint32 bgColor=0, Id id=0);
		virtual ~ScrollablePane();

		virtual void draw();

		virtual void refresh();

		/// updates the offset displacing all childs of the 'content' BgrWin. dx and dy are deltas.
		virtual void updateOffset(int dx=0, int dy=0);

		/// updates the offset displacing all childs of the 'content' BgrWin.
		virtual void setOffset(int x, int y);

		/// overrides WinBase::widen()
		virtual void widen(int dx, int dy);

		/// preferred way to call the widen() method for the 'content' BgrWin.
		virtual void widenContent(int dx, int dy);

		/// preferred way to enable/disable the vertical scrollbar
		virtual void setScrollbarVerticalVisible(bool desiredVisibility=true);

		/// preferred way to enable/disable the horizontal scrollbar
		virtual void setScrollbarHorizontalVisible(bool desiredVisibility=true);

		/// transverse all tree hierarchy searching for a ScrollablePane ancestral
		static void scrollContentOnMouseWheel(WinBase* child, int but);

		protected:
		BgrWin decorativeMask;
		static void hscrollbarCallback(HScrollbar*,int val,int range);
		static void vscrollbarCallback(VScrollbar*,int val,int range);
		static void scrollContentOnMouseWheel(BgrWin*,int x,int y,int but);
	};

}

#endif /* WIDGETS_SCROLLABLE_PANE_HPP_ */
