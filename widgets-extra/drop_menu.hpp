/*
 * drop_menu.hpp
 *
 *  Created on: 18 de abr de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_DROP_MENU_HPP_
#define WIDGETS_DROP_MENU_HPP_

#include "abstract_layout.hpp"
#include "label_win.hpp"

namespace WidgetsExtra
{
	struct DropDownMenu extends WidgetsExtra::Layout::Element
	{
		CmdMenu* cmdMenu;
		WidgetsExtra::LabelWin* label;
		bool isLabelOnTop;

		virtual ~DropDownMenu();

		virtual Point getPosition() const;
		virtual void setPosition(Point position);
		virtual Rect getSize() const;
		virtual void setSize(Rect size);
		virtual bool isStretched() const;
		virtual bool operator == (const Element& b) const;
	};

	struct DropDownMenuFactory
	{
		DropDownMenuFactory();
		~DropDownMenuFactory();
		enum Appearance { BUTTON, COMBOBOX, MENU };

		void setLabel(Label label, bool onTop=false);
		void setCallback(void (*menu_cmd)(RButWin*,int nr,int fire));
		void setAppearance(Appearance apre);
		void setSize(Rect rt);

		void addItem(Label label, short index=-1);
		Label itemAt(unsigned index);
		void removeItem(short index);
		void removeAllItems();

		DropDownMenu* createAt(WinBase* pw, Point pt, Id id=0);
		DropDownMenu* createAt(WinBase* pw, Id id=0);
		friend class DropDownMenuButton;

		private:
		struct Implementation;
		Implementation* implementation;
	};
}




#endif /* WIDGETS_DROP_MENU_HPP_ */
