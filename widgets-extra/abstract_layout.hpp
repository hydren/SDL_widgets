/*
 * layout.hpp
 *
 *  Created on: 15 de abr de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_ABSTRACT_LAYOUT_HPP_
#define WIDGETS_ABSTRACT_LAYOUT_HPP_
#include <ciso646>

#include <vector>

#include "SDL_widgets.h"
#include "util.hpp"

namespace WidgetsExtra
{
	/** An abstract layout manager. */
	struct Layout
	{
		enum Orientation {HORIZONTAL, VERTICAL};

		/** An abstract layout element. It defines basic manipulations. */
		struct Element
		{
			Point offset;
			virtual ~Element() {}
			virtual Point getPosition() const abstract;
			virtual void setPosition(Point position) abstract;
			virtual Rect getSize() const abstract;
			virtual void setSize(Rect size) abstract;
			virtual bool isStretched() const abstract;
			virtual bool operator == (const Element& b) const abstract;
		};

		/** A WinBase element. It knows how to manipulate WinBase elements. */
		struct WinBaseWrapper extends Element
		{
			WinBase* base;
			WinBaseWrapper(WinBase* winBase);
			virtual ~WinBaseWrapper();
			virtual Point getPosition() const;
			virtual void setPosition(Point position);
			virtual void setPosition(int x, int y);
			virtual void setX(int x);
			virtual void setY(int y);
			virtual Rect getSize() const;
			virtual void setSize(Rect size);
			virtual bool isStretched() const;
			virtual bool operator == (const Element& b) const;

			static void setWinBasePosition(WinBase* wb, Point pos);
			static void setWinBasePositionX(WinBase* wb, int x);
			static void setWinBasePositionY(WinBase* wb, int y);
		};

		/** An empty element to provide a way to offset elements.
		 *  If width and height are set to zero, it can stretch, effectively pushing elements after it aside. */
		struct Spacer extends Element
		{
			Rect bounds;

			Spacer(Layout* layout, Rect size=Rect());
			Spacer(Rect bounds);
			virtual ~Spacer();
			virtual Point getPosition() const;
			virtual void setPosition(Point position);
			virtual Rect getSize() const;
			virtual void setSize(Rect size);
			virtual bool isStretched() const;
			virtual bool operator == (const Element& b) const;

			protected:
			bool stretched;
		};

		struct Separator extends WinBase, WinBaseWrapper
		{
			Orientation orientation;
			Separator(WinBase* parent, Orientation orient, unsigned size, unsigned thick=0);
			virtual void draw();
		};

		/** The position of this layout */
		Point position;

		/** Creates a layout panel that sits its components from the given coordinates */
		Layout(int x, int y);

		/** Creates a layout panel that sits its components from the given point (copies the point)*/
		Layout(Point& p);

		virtual ~Layout();

		/** Add a component to the layout */
		void addComponent(WinBase& component, int index=-1);
		/** Add a component to the layout */
		void addComponent(WinBase* component, int index=-1);
		/** Add a component to the layout */
		void addComponent(Element& component, int index=-1);
		/** Add a component to the layout */
		void addComponent(Element* component, int index=-1);

		/** Returns a pointer to the component at the specified index */
		Element* getComponentAt(unsigned index);

		/** Returns a pointer to the wrapper component corresponding to the given WinBase */
		WinBaseWrapper* getWrapperComponent(WinBase* base);

		/** Remove the component at the specified index (but does not delete or hides it from the window) */
		void removeComponentAt(unsigned index);
		/** Remove the specified component (but does not free or hides it from the window) */
		void removeComponent(WinBase& component);
		/** Remove the specified component (but does not free or hides it from the window) */
		void removeComponent(WinBase* component);
		/** Remove the specified component (but does not free or hides it from the window) */
		void removeComponent(Element& component);
		/** Remove the specified component (but does not free or hides it from the window) */
		void removeComponent(Element* component);

		/** Returns the number of components on this layout. */
		unsigned getComponentCount() const;

		/** Organizes the position of the components on this layout */
		virtual void pack() abstract;

		/** Hides all WinBase components on this layout by calling their hide() method. */
		virtual void hideAll();

		/** Show all WinBase components on this layout by calling their show() method. This should be called after a hideAll() call to make its components visible again. */
		virtual void showAll();

		protected:
		std::vector<Element*> components; //all components on this layout
		std::vector<WinBaseWrapper*> innerWrappers; //a vector containing all WinBaseWrappers created by this layout.
	};
}

#endif /* WIDGETS_ABSTRACT_LAYOUT_HPP_ */
