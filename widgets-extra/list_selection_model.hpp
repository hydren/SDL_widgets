/*
 * list_selection_model.hpp
 *
 *  Created on: 27 de jun de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_LIST_SELECTION_MODEL_HPP_
#define WIDGETS_LIST_SELECTION_MODEL_HPP_
#include <ciso646>

#include <vector>

namespace WidgetsExtra
{
	struct ListSelectionModel
	{
		ListSelectionModel();

		/// Makes the given index selected. Any previous selection will also remain.
		void select(unsigned index);
		/// Makes the given index interval selected (startIndex, endIndex). Any previous selection will also remain.
		void select(unsigned startIndex, unsigned endIndex);

		/// Modifies the given index range: set selected if 'selected' is true and false otherwise. Any previous selection will also remain. If silent is true, no event is triggered.
		void modify(unsigned index, unsigned endIndex, bool selected, bool silent=false);

		/// Makes the given index deselected. Any previous selection will also remain.
		void unselect(unsigned index);
		/// Makes the given index interval deselected (startIndex, endIndex). Any previous selection will also remain.
		void unselect(unsigned index, unsigned endIndex);

		/// Toogle the given index to be selected/deselected. Any previous selection will also remain.
		void toogle(unsigned index);
		/// Toogle the given index interval to be selected/deselected. Any previous selection will also remain.
		void toogle(unsigned index, unsigned endIndex);

		/// Returns false if no index is selected. Otherwise returns true.
		bool isSelected() const;
		/// Returns true if the given index is selected.
		bool isSelected(unsigned index) const;

		/// Returns the selected index. Returns 0 if no index is selected. (use isSelected() if this causes ambiguity)
		unsigned getSelected() const;
		/// Returns the selected indexes, in order. The given 'n' variable will hold the ammount of selected indexes after this call. PS: Remember to delete the returned array!
		unsigned* getSelected(unsigned& n) const;
		/// Returns a vector containing the selected indexes, in order.
		std::vector<unsigned> getSelectedIndexes() const;

		/// Makes the given index the selected one. All other indexes are deselected.
		void setSelected(unsigned index);
		/// Makes the given index interval the be the only selected indexes. All other indexes are deselected.
		void setSelected(unsigned startIndex, unsigned endIndex);
		/// Makes the given indexes the be the only selected ones. The number of indexes to select (in order) must be passed throught 'n'. All other indexes are deselected.
		void setSelected(const unsigned* indexes, unsigned n);
		/// Makes the indexes in the given vector the only selected ones. All other indexes are deselected.
		void setSelectedIndexes(const std::vector<unsigned>& indexes);

		/// Makes all indexes to be unselected. After this call, isSelected() should return false.
		void clear();

		/// Shifts the selection (up if 'up' is true and down otherwise) after the selected index.
		/// If a down-shift is performed, the selection range is increased by 1. If a up-shift is performed, the selection range is decreased by 1. Use fit() to maintain range size.
		/// PS: The given index will be not selected if a down-shift is performed.
		/// @param up
		/// @param index
		void shift(bool up, unsigned index=0);

		/// Reduce or increase the selection possible range to match the given size.
		/// If reduced, all indexes before 'size'-1 will keep their state. If increased, all indexes after 'size'-1 will be deselected.
		/// @param size the size to fit the selection model
		void fit(unsigned size);

		/// Optional callback to be called when a change in the selection is made.
		/// If a selection or a deselection is made on a single index ('index'='endIndex') or a contiguous interval, this function is called.
		/// If more than one interval is selected, multiple calls to this function is made - each call for each contiguous interval.
		void (*onChange)(unsigned index, unsigned endIndex);

		/// A simple listener for ListSelectionModel. Subclass this struct and register to the listeners list to get called on selection changes.
		struct Listener
		{
			/// Method to be called in the listener when a change in the selection is made.
			/// If a selection or a deselection is made on a single index ('index'='endIndex') or a contiguous interval, this function is called.
			/// If more than one interval is selected, multiple calls to this function is made - each call for each contiguous interval.
			virtual void onChange(unsigned index, unsigned endIndex)=0;
		};

		/// Adds a Listener to this ListSelectionModel's list of listeners. If the given listener is already added, nothing happens.
		void listenerAdd(Listener* listener);
		/// Removes a Listener from this ListSelectionModel's list of listeners. If the given listener wasn't in the list, nothing happens.
		void listenerRemove(Listener* listener);

		protected:
		std::vector<bool> selection;
		std::vector<Listener*>* listeners;

		void notify();
		void notify(unsigned index);
		void notify(unsigned index, unsigned endIndex);

		void clear(unsigned except0, unsigned except1);
		friend class GenericSelectionAdjustment;
	};
}



#endif /* WIDGETS_LIST_SELECTION_MODEL_HPP_ */
