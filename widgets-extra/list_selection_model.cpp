/*
 * list_selection_model.cpp
 *
 *  Created on: 27 de jun de 2016
 *      Author: carlosfaruolo
 */

#include "list_selection_model.hpp"

#include <cstdlib>
#include <algorithm>

using WidgetsExtra::ListSelectionModel;
using std::vector;

ListSelectionModel::ListSelectionModel()
: onChange(NULL), selection(), listeners(NULL)
{}

void ListSelectionModel::select(unsigned index)
{
	selection.at(index) = true;
	notify(index);
}

void ListSelectionModel::select(unsigned startIndex, unsigned endIndex)
{
	for(unsigned i = startIndex; i <= endIndex; i++)
		selection.at(i) = true;
	notify(startIndex, endIndex);
}

void ListSelectionModel::modify(unsigned index, unsigned endIndex, bool selected, bool silent)
{
	for(unsigned i = index; i <= endIndex; i++)
		selection.at(i) = selected;
	if(not silent) notify(index, endIndex);
}

void ListSelectionModel::unselect(unsigned index)
{
	selection.at(index) = false;
	notify(index);
}

void ListSelectionModel::unselect(unsigned startIndex, unsigned endIndex)
{
	for(unsigned i = startIndex; i <= endIndex; i++)
		selection.at(i) = false;
	notify(startIndex, endIndex);
}

void ListSelectionModel::toogle(unsigned index)
{
	selection.at(index).flip();
	notify(index);
}

void ListSelectionModel::toogle(unsigned startIndex, unsigned endIndex)
{
	for(unsigned i = startIndex; i <= endIndex; i++)
		selection.at(i).flip();
	notify(startIndex, endIndex);
}

bool ListSelectionModel::isSelected() const
{
	for(unsigned i = 0; i < selection.size(); i++)
		if(selection[i] == true)
			return true;
	return false;
}

bool ListSelectionModel::isSelected(unsigned index) const
{
	return selection.at(index);
}

unsigned ListSelectionModel::getSelected() const
{
	for(unsigned i = 0; i < selection.size(); i++)
		if(selection[i] == true)
			return i;
	return 0;
}

unsigned *ListSelectionModel::getSelected(unsigned & n) const
{
	vector<unsigned> selectedOnes = getSelectedIndexes();
	n = selectedOnes.size();
	unsigned* arr = new unsigned[n];
	for(unsigned i = 0; i < n; i++)
		arr[i] = selectedOnes[i];
	return arr;
}

vector<unsigned> ListSelectionModel::getSelectedIndexes() const
{
	vector<unsigned> selectedOnes;

	for(unsigned i = 0; i < selection.size(); i++)
		if(selection[i] == true)
			selectedOnes.push_back(i);

	return selectedOnes;
}

void ListSelectionModel::setSelected(unsigned index)
{
	if(not isSelected()) //nothing selected before
		select(index);
	else
	{
		unsigned n, *indexes = getSelected(n);
		if(n == 1 and indexes[0] == index) return; // only one selected and the same

		clear();
		select(index);
	}
}

void ListSelectionModel::setSelected(unsigned startIndex, unsigned endIndex)
{
	if(not isSelected())
		select(startIndex, endIndex);
	else
	{
		for(unsigned i = startIndex; i <= endIndex; i++)
			selection.at(i) = true;
		clear(startIndex, endIndex);
	}
}

void ListSelectionModel::setSelected(const unsigned * indexes, unsigned n)
{
	selection.assign(selection.size(), false);
	for(unsigned i = 0; i < n; i++)
		selection.at(indexes[i]) = true;
	notify();
}

void ListSelectionModel::setSelectedIndexes(const std::vector<unsigned>& indexes)
{
	selection.assign(selection.size(), false);
	for(unsigned i = 0; i < indexes.size(); i++)
		selection.at(indexes.at(i)) = true;
	notify();
}

void ListSelectionModel::clear()
{
	if(onChange == NULL and (listeners == NULL or listeners->size() == 0))
		selection.assign(selection.size(), false); //doesn't need to notify anyone, use raw function

	else //notify along assignment
	{
		long currentChangeStartIndex = -1; // start of current changing interval
		for(unsigned i = 0; i < selection.size(); i++)
		{
			bool wasTrue = (selection[i] == true); // a change
			selection[i] = false;
			if(wasTrue and currentChangeStartIndex == -1)
			{
				currentChangeStartIndex = i;
			}
			else if(not wasTrue and currentChangeStartIndex != -1)
			{
				notify(currentChangeStartIndex, i-1);
				currentChangeStartIndex = -1;
			}
		}
		if(currentChangeStartIndex != -1)
			notify(currentChangeStartIndex, selection.size()-1);
	}
}

void ListSelectionModel::clear(unsigned exceptStart, unsigned exceptEnd)
{
	long currentChangeStartIndex = -1; // start of current changing interval
	for(unsigned i = 0; i < selection.size(); i++)
	{
		bool changed = (selection[i] == true) or (i <= exceptEnd and i >= exceptStart); // a change
		if(i > exceptEnd or i < exceptStart) selection[i] = false;
		if(changed and currentChangeStartIndex == -1)
		{
			currentChangeStartIndex = i;
		}
		else if(not changed and currentChangeStartIndex != -1)
		{
			notify(currentChangeStartIndex, i-1);
			currentChangeStartIndex = -1;
		}
	}
	if(currentChangeStartIndex != -1)
		notify(currentChangeStartIndex, selection.size()-1);
}

void ListSelectionModel::shift(bool up, unsigned index)
{
	if(up)
		selection.erase(selection.begin() + index);
	else
		selection.insert(selection.begin() + index, false);

	if(index < selection.size())
		notify(index, selection.size()-1);
//	else
//		notify(); //xxx should we notify this?
}

void ListSelectionModel::fit(unsigned size)
{
	while(size != selection.size())
		if(size > selection.size())
			selection.push_back(false);
		else
			selection.pop_back();

	//xxx should we notify this?
}

void ListSelectionModel::listenerAdd(Listener* listener)
{
	if(listeners == NULL) listeners = new vector<Listener*>();
	if(std::find(listeners->begin(), listeners->end(), listener) == listeners->end())
		listeners->push_back(listener);
}

void ListSelectionModel::listenerRemove(Listener* listener)
{
	if(listeners == NULL) return;
	vector<Listener*>::iterator it = std::find(listeners->begin(), listeners->end(), listener);
	if(it != listeners->end())
		listeners->erase(it);
}

void ListSelectionModel::notify()
{
	notify(0, selection.size()-1);
}

void ListSelectionModel::notify(unsigned index)
{
	notify(index, index);
}

void ListSelectionModel::notify(unsigned index, unsigned endIndex)
{
	if(onChange != NULL)
		onChange(index, endIndex);

	if(listeners != NULL)
		for(unsigned i = 0; i < listeners->size(); i++)
			listeners->at(i)->onChange(index, endIndex);
}
