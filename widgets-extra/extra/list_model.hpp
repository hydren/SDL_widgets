/*
 * list_model.hpp
 *
 *  Created on: 20 de jun de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_LIST_MODEL_HPP_
#define WIDGETS_LIST_MODEL_HPP_

#include "abstract_list_model.hpp"
#include "../util.hpp"

#include <vector>

namespace WidgetsExtra
{
	/// Default implementation of UIListModel. The list's elements are standard strings.
	struct DefaultUIListModel extends UIListModel
	{
		DefaultUIListModel();
		DefaultUIListModel(const std::vector<std::string>& data);

		virtual std::string getStringfiedElementAt(unsigned index);
		virtual unsigned size();
		virtual void updateData(const std::vector<std::string>& data);
		virtual void updateData(void* data);
		virtual void* getData();

		protected:
		std::vector<std::string> data;
	};
}

#endif /* WIDGETS_LIST_MODEL_HPP_ */
