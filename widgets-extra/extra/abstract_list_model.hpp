/*
 * abstract_list_model.hpp
 *
 *  Created on: 20 de jun de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_ABSTRACT_LIST_MODEL_HPP_
#define WIDGETS_ABSTRACT_LIST_MODEL_HPP_

#include <string>

namespace WidgetsExtra
{
	/// A model for a list of elements to be displayed by a UI element like ListWin.
	struct UIListModel
	{
		virtual ~UIListModel() {};

		/// Returns the string value of the element at the specified index.
		virtual std::string getStringfiedElementAt(unsigned index)=0;

		/// Returns the length of the list.
		virtual unsigned size()=0;

		/// (Optional) Updates the inner data of this model.
		/// Behavior and expected type and format of the given data are implementation-defined.
		virtual void updateData(void* data) {}

		/// (Optional) Returns the inner data of this model.
		/// Behavior and expected type and format of the returned data are implementation-defined.
		virtual void* getData() { return 0; }
	};
}

#endif /* WIDGETS_ABSTRACT_LIST_MODEL_HPP_ */
