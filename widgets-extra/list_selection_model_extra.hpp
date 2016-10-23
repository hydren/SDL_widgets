/*
 * list_selection_model_extra.hpp
 *
 *  Created on: 29 de jun de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_LIST_SELECTION_MODEL_EXTRA_HPP_
#define WIDGETS_LIST_SELECTION_MODEL_EXTRA_HPP_

#include "list_selection_model_extra.hpp"

#include <algorithm>
//#include <iostream>

namespace WidgetsExtra
{
	struct GenericSelectionAdjustment
	{
		/// Adjusts this selection according to the changes in the given data.
		/// The parameter 'oldData' should be the list from which this model is the selection model.
		/// The parameter 'newData' should be the new state of the list, from which the given selection should be compatible after a call to this method.
		/// Note: 'oldData' must have the same size as this' model range size.
		template<typename Type> static
		void function(ListSelectionModel& self, const std::vector<Type>* oldData, const std::vector<Type>* newData)
		{
			std::vector<bool> oldSelection = self.selection; //copy the old selection
			self.selection.assign(newData->size(), false); //clean new selection
			for(unsigned i = 0; i < self.selection.size(); i++) //try to recover previous selection
			{
				typename std::vector<Type>::const_iterator position = std::find(oldData->begin(), oldData->end(), (*newData)[i]);
				if(position != oldData->end())
				{
					const unsigned oldIndex = position - oldData->begin();
					self.selection[i] = oldSelection[oldIndex];
//					std::cout << "Found equal: " << i << "->" << oldIndex << std::endl; // debug
				}
			}
		}

		/// Adjusts this selection according to the changes in the given data.
		/// The parameter 'oldData' should be the list from which this model is the selection model.
		/// The parameter 'newData' should be the new state of the list, from which the given selection should be compatible after a call to this method.
		/// Note: 'oldData' must have the same size as this' model range size.
		template<typename Type> static
		void function(ListSelectionModel& self, const void* oldDataPtr, const void* newDataPtr)
		{
			function(self, static_cast<const std::vector<Type>*>(oldDataPtr), static_cast<const std::vector<Type>*>(newDataPtr));
		}

		//Lock instantiation
		private: GenericSelectionAdjustment(){}
		virtual ~GenericSelectionAdjustment()=0;
	};
}



#endif /* WIDGETS_LIST_SELECTION_MODEL_EXTRA_HPP_ */
