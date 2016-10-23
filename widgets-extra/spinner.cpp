/*
 * spinner.cpp
 *
 *  Created on: 24 de mai de 2016
 *      Author: carlosfaruolo
 */

#include "spinner.hpp"
#include <map>

#include "widgets_util.hpp"

using std::string;
using WidgetsExtra::AbstractSpinner;
using WidgetsExtra::Spinner;

// helpers
namespace AbstractSpinnerAux
{
	int nextId = 0; // used to maintain consistency. not thread safe.
	std::map<int, AbstractSpinner*> references; //xxx kludge to map references to Spinner's addresses by id
}

AbstractSpinner::AbstractSpinner(WinBase *pw, Style style, Rect area, const char* label)
: WinBase(pw, null, area.x, area.y, area.w, area.h, pw->bgcol, ++AbstractSpinnerAux::nextId),
  dlwTextField(this, Rect(0, 0, area.w - BUTTON_SIZE, area.h)),
  btnInc (this, style, Rect(area.w - BUTTON_SIZE, TDIST, BUTTON_SIZE, BUTTON_SIZE/2), "+", handleAbstractSpinnerButtonPress),
  btnDec (this, style, Rect(area.w - BUTTON_SIZE, BUTTON_SIZE/2 + TDIST, BUTTON_SIZE, BUTTON_SIZE/2), "-", handleAbstractSpinnerButtonPress)
{
	if(label != null) this->dlwTextField.dialog_label(label);
	if(style.param2 != 0) dlwTextField.bgcol = style.param2;
	this->dlwTextField.cmd = handleAbstractSpinnerEnterKeyPress;
	this->dlwTextField.cmd_id = this->id.id1;
	AbstractSpinnerAux::references[this->id.id1] = this; //register kludge-type reference to this spinner
}

AbstractSpinner::~AbstractSpinner()
{}

void AbstractSpinner::draw()
{
	this->init_gui();
	SDL_FillRect(this->win, null, this->bgcol); //clears the screen
}

void AbstractSpinner::widen(int dx, int dy)
{
	dlwTextField.widen(dx, dy);
	WidgetsExtra::setComponentPosition(&btnInc, dlwTextField.tw_area.w, TDIST);
	WidgetsExtra::setComponentPosition(&btnDec, dlwTextField.tw_area.w, BUTTON_SIZE/2 + TDIST);
}

void AbstractSpinner::setLabel(const char* lbl)
{
	this->dlwTextField.dialog_label(lbl);
}

void AbstractSpinner::refresh()
{
	string strValue = this->valueToString();
	this->dlwTextField.dialog_def(strValue.c_str(), this->dlwTextField.cmd, this->dlwTextField.cmd_id);
	this->dlwTextField.unset_cursor();
	this->btnInc.draw_blit_upd();
	this->btnDec.draw_blit_upd();
}

void AbstractSpinner::onButtonPressed(Button* btn)
{
	if(btn == &btnInc) this->incrementValue();
	if(btn == &btnDec) this->decrementValue();
}

void AbstractSpinner::onEnterKey(const char* currentTxt)
{
	this->assignValue(currentTxt);
	this->refresh();
}

// static callback methods
void AbstractSpinner::handleAbstractSpinnerButtonPress(Button* btn) { static_cast<AbstractSpinner*>(btn->parent)->onButtonPressed(btn); }
void AbstractSpinner::handleAbstractSpinnerEnterKeyPress(const char* text, int cmd_id) { AbstractSpinnerAux::references[cmd_id]->onEnterKey(text); /*kludged reference to the button's spinner*/ }
