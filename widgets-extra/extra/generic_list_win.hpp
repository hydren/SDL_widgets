/*
 * generic_list_win.hpp
 *
 *  Created on: 5 de jul de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_GENERIC_LIST_WIN_HPP_
#define WIDGETS_GENERIC_LIST_WIN_HPP_

#include "../list_win.hpp"
#include "list_model.hpp"

namespace WidgetsExtra
{
	struct GenericListWin extends AbstractListWin
	{
		UIListModel* model;

		/// Optional function to be called to adjust this model's selection when changes occur in the list data related to this selection model.
		/// If it is null (default), the selection will be cleared everytime the inner data changes.
		void (*selectionAdjustmentFunction)(ListSelectionModel& selection, const void* oldDataPtr, const void* newDataPtr);

		//can be called only after a TopWin has been constructed
		GenericListWin(WinBase *parent, Style, Rect rect, Id id=0);

		virtual ~GenericListWin();

		virtual unsigned size();
		virtual std::string getAsString(unsigned index);

		virtual void setListData(const std::vector<std::string>& data, bool redrawImmediately=false);
		virtual void setListModel(UIListModel* model, bool redrawImmediately=false, bool deletePrevious=true);
		virtual void updateListData(void* data, bool redrawImmediately=false);
	};
}



#endif /* WIDGETS_GENERIC_LIST_WIN_HPP_ */
