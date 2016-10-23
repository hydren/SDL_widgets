/*
 * generic_spinner.cpp
 *
 *  Created on: 7 de jul de 2016
 *      Author: carlosfaruolo
 */

#include "generic_spinner.hpp"

#include "../widgets_util.hpp"

#include <map>

using std::string;
using WidgetsExtra::GenericSpinner;
using WidgetsExtra::SpinnerDataModel;

// helpers
namespace GenericSpinnerAux
{
	int nextId = 0; // used to maintain consistency. not thread safe.
	std::map<int, GenericSpinner*> references; //xxx kludge to map references to Spinner's addresses by id
}

GenericSpinner::GenericSpinner(WinBase *pw, Rect area, const char* label, SpinnerDataModel* model)
: WinBase(pw, null, area.x, area.y, area.w, area.h, pw->bgcol, ++GenericSpinnerAux::nextId),
  model(model),
  dlwTextField(this, Rect(0, 0, area.w - BUTTON_SIZE, area.h)),
  btnInc (this, 0, Rect(area.w - BUTTON_SIZE, TDIST, BUTTON_SIZE, BUTTON_SIZE/2), "+", changeValue),
  btnDec (this, 0, Rect(area.w - BUTTON_SIZE, BUTTON_SIZE/2 + TDIST, BUTTON_SIZE, BUTTON_SIZE/2), "-", changeValue),
  needsRefresh(false)
{
	if(label != null) this->dlwTextField.dialog_label(label);
	this->dlwTextField.cmd = validateField;
	this->dlwTextField.cmd_id = this->id.id1;
	GenericSpinnerAux::references[this->id.id1] = this; //register kludge-type reference to this spinner
}

GenericSpinner::~GenericSpinner()
{}

void GenericSpinner::setModel(SpinnerDataModel* model)
{
	this->model = model;
	model->listeners.addListener(this);
}

void GenericSpinner::draw()
{
	this->init_gui();

	if(this->needsRefresh)
		this->refresh();
	else
		SDL_FillRect(this->win, null, this->bgcol); //clears the screen
}

void GenericSpinner::widen(int dx, int dy)
{
	dlwTextField.widen(dx, dy);
	WidgetsExtra::setComponentPosition(&btnInc, dlwTextField.tw_area.w, TDIST);
	WidgetsExtra::setComponentPosition(&btnDec, dlwTextField.tw_area.w, BUTTON_SIZE/2 + TDIST);
}

void GenericSpinner::setLabel(const char* lbl)
{
	this->dlwTextField.dialog_label(lbl);
}

void GenericSpinner::refresh()
{
	string strValue = this->model->valueToString();
	this->dlwTextField.dialog_def(strValue.c_str(), this->dlwTextField.cmd, this->dlwTextField.cmd_id);
	this->dlwTextField.unset_cursor();
	this->btnInc.draw_blit_upd();
	this->btnDec.draw_blit_upd();
	this->needsRefresh = false;
}

void GenericSpinner::onValueChange(SpinnerDataModel*)
{
	this->needsRefresh = true;
}

void GenericSpinner::onButtonPressed(Button* btn)
{
	if(string(btn->label.str) == string("+")) //if '+' do increment
		this->model->incrementValue();

	if(string(btn->label.str) == string("-")) //if '-' do decrement
		this->model->decrementValue();

	this->refresh();
}

void GenericSpinner::onFieldInputConfirmation(const char* input)
{
	this->model->assign(input);
	this->refresh();
}

// ################# static methods to be used when SDL_widgets requests plain functions pointers  #################
void GenericSpinner::changeValue(Button* btn) { static_cast<GenericSpinner*>(btn->parent)->onButtonPressed(btn); }
void GenericSpinner::validateField(const char* text, int cmd_id) { GenericSpinnerAux::references[cmd_id]->onFieldInputConfirmation(text); } //kludged reference to the button's spinner
