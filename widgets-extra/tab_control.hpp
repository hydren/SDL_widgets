/*
 * tab_control.hpp
 *
 *  Created on: 28 de abr de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_TAB_CONTROL_HPP_
#define WIDGETS_TAB_CONTROL_HPP_

#include <vector>

#include "SDL_widgets.h"

namespace WidgetsExtra
{
	struct TabController
	{
		ExtRButCtrl* tabButtonsController;
		std::vector<RExtButton*> tabButtons;
		std::vector<BgrWin*> tabsPanels;

		TabController();

		/// The safe way to add tabs with this controller.
		/// Creates a button with the given parameters and register it in the controller.
		/// The button is associated to the given BgrWin* and will make it visible when clicked.
		RExtButton* addTab(WinBase* parent, Rect tabButtonRect, Label lab, BgrWin* content);

		/// Sets the active (visible) tab.
		void setActiveTab(unsigned index);
		void setActiveTab(BgrWin* tabContent);

		void widenAll(int dx, int dy);

		private:static void onTabButtonClicked(RExtButton* rbTab, bool is_act);
	};
}



#endif /* WIDGETS_TAB_CONTROL_HPP_ */
