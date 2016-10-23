/*
 * list_model_extra.hpp
 *
 *  Created on: 20 de jun de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_LIST_MODEL_EXTRA_HPP_
#define WIDGETS_LIST_MODEL_EXTRA_HPP_

#include "abstract_list_model.hpp"
#include "../util.hpp"

namespace WidgetsExtra
{
	/// A more customizable implementation of UIListModel.
	/// StringableType should be a typename that is possible to be cast to a std::string or else a pointer to a function that peforms the operation needs to be specified.
	/// If provided, the function provided must receive a StringableType instance and return a std::string (most likely a string representation of the instance).
	/// This string returning operation is referred as a 'stringfy' operation, similiar to Java's toString() method.
	/// There are pre-baked functions defined in futil's String::Callbacks namespace: stringfy_by_cast, stringfy_by_method, and others.
	template <typename StringableType>
	struct StringableTypeUIListModel extends UIListModel
	{
		StringableTypeUIListModel()
		: data(), stringfyFunction(stringfy_by_cast<StringableType>) {}

		StringableTypeUIListModel(const std::vector<StringableType>& data)
		: data(data), stringfyFunction(stringfy_by_cast<StringableType>) {}

		StringableTypeUIListModel(std::string (*stringfyFunction)(StringableType))
		: data(), stringfyFunction(stringfyFunction) {}

		StringableTypeUIListModel(const std::vector<StringableType>& data, std::string (*stringfyFunction)(StringableType))
		: data(data), stringfyFunction(stringfyFunction) {}

		virtual std::string getStringfiedElementAt(unsigned index) { return stringfyFunction(data.at(index)); }

		virtual unsigned size() { return data.size(); }

		virtual void updateData(const std::vector<StringableType>& data) { this->data = data; }

		virtual void updateData(void* data) { this->updateData(*static_cast<std::vector<StringableType>*>(data)); }

		virtual void* getData() { return &this->data; }

		protected:
		std::vector<StringableType> data;
		std::string (*stringfyFunction)(StringableType value);
	};
}

#endif /* WIDGETS_LIST_MODEL_EXTRA_HPP_ */
