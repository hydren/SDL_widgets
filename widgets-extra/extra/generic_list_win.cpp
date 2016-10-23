/*
 * generic_list_win.cpp
 *
 *  Created on: 5 de jul de 2016
 *      Author: carlosfaruolo
 */

#include "generic_list_win.hpp"
#include "../widgets_util.hpp"

using WidgetsExtra::GenericListWin;
using WidgetsExtra::DefaultUIListModel;
using WidgetsExtra::UIListModel;

using std::vector;
using std::string;

GenericListWin::GenericListWin(WinBase *parent, Style style, Rect rect, Id id)
: AbstractListWin(parent, style, rect, id),
  model(new DefaultUIListModel()),
  selectionAdjustmentFunction(null)
{}

GenericListWin::~GenericListWin()
{
	delete model;
}

void GenericListWin::setListData(const std::vector<std::string>& data, bool redrawImmediately)
{
	this->setListModel(new DefaultUIListModel(data), redrawImmediately);
}

void GenericListWin::setListModel(UIListModel* model, bool redrawImmediately, bool deletePrevious)
{
	UIListModel* previousModel = this->model;

	this->model = model;
	this->selection.clear();
	this->selection.fit(model->size());

	if(sdl_running and redrawImmediately) this->draw_blit_upd();
	if(deletePrevious and previousModel != null) delete previousModel;
}

void GenericListWin::updateListData(void* data, bool redrawImmediately)
{
	if(this->selectionAdjustmentFunction == null)  //no special selection adjuster specified
	{
		//default procedure, clear selection
		this->selection.fit(model->size());
		this->selection.clear();
	}
	else this->selectionAdjustmentFunction(selection, this->model->getData(), data); // special selection adjuster
	this->model->updateData(data);
	if(sdl_running and redrawImmediately) this->draw_blit_upd();
}

unsigned GenericListWin::size()
{
	return this->model->size();
}

string GenericListWin::getAsString(unsigned index)
{
	return this->model->getStringfiedElementAt(index);
}
