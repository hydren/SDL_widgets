/*
 * spinner.hpp
 *
 *  Created on: 12 de abr de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_SPINNER_HPP_
#define WIDGETS_SPINNER_HPP_
#include <ciso646>

#include "SDL_widgets.h"

#include <stdexcept>
#include <string>

#include "util.hpp"
#include "abstract_layout.hpp"

namespace WidgetsExtra
{
	/// Generic widget that serves as a skeleton to the template struct Spinner.
	struct AbstractSpinner extends WinBase
	{
		static const unsigned BUTTON_SIZE = 16;

		DialogWin dlwTextField;
		Button btnInc, btnDec;

		AbstractSpinner(WinBase *pw, Style style, Rect area, const char* label=null);
		virtual ~AbstractSpinner();

		//overrides WinBase's
		virtual void draw();

		//overrides WinBase's (caution: doesn't work if called from a WinBase pointer; widen() is not virtual in WinBase)
		virtual void widen(int dx, int dy);

		/// Sets the spinner label
		virtual void setLabel(const char* lbl);

		/// Refreshes the text according to this spinner's current value.
		virtual void refresh();

		//pure virtual. needs to be implemented:

		/// Should increment the spinner value
		virtual void incrementValue() abstract;

		/// Should decrement the spinner value
		virtual void decrementValue() abstract;

		/// Should, if the text in the field is valid, update the spinner value. Otherwise refresh the field text with the spinner value (overriding any text in the field)
		virtual void assignValue(const char* txtVal) abstract;

		/// Returns a string representation of the spinner's current value;
		virtual std::string valueToString() abstract;

		protected:

		void onButtonPressed(Button* btn);
		void onEnterKey(const char* currentTxt);

		private:
		//redirects SDL_widgets callbacks to methods
		static void handleAbstractSpinnerButtonPress(Button* btn);
		static void handleAbstractSpinnerEnterKeyPress(const char* text,int cmd_id);
	};

	/** A widget similar to Swing's JSpinner. It is a template, which means the value on the spinner field can be (theorectically) of any type.
	 * Currently it has been tested (and passed) with integer types (int, long and alike) and floating points types (float, double and alike).
	 * The template type must have operator + and -, and be able to cast from literal values 1 and 9999.
	 * Default bounds are 0 (min) and 9999 (max). Set these as needed. Default step value is literal value 1.*/
	template<typename Type>
	struct Spinner extends AbstractSpinner
	{
		Type* value, step, minValue, maxValue;
		bool needsRefresh;

		Spinner(WinBase *pw, Style style, Rect area, const char* label=null, Type* value=null)
		: AbstractSpinner(pw, style, area, label),
		  value(value==null? new Type(1) : value), step(1), minValue(0), maxValue(9999),
		  needsRefresh(false)
		{}

		/// Returns a pointer to the spinner current value.
		virtual Type* getValue()
		{
			return this->value;
		}

		/// Replaces the spinner value with the pointer 'val'. If 'deleteOldValue' is true, calls the previous value's destructor.
		virtual void setValue(Type* val, bool deleteOldValue=false)
		{
			Type* old = this->value;

			this->value = val;
			this->needsRefresh = true;

			if(deleteOldValue) delete old;
		}

		/// Updates the spinner value, assigning 'newValue' to it.
		virtual void setValue(const Type& newValue)
		{
			*(this->value) = newValue;
			this->needsRefresh = true;
		}

		/// Returns the value by which the spinner's value is incremented or decremented when calling incrementValue() or decrementValue()
		virtual Type getStepValue()
		{
			return this->step;
		}

		/// Sets the value by which the spinner's value is incremented or decremented to 'v'. 'v' must be non-negative.
		virtual void setStepValue(Type v)
		{
			if(v < 0) //step value must be non-negative
				throw std::invalid_argument("Specified step value in Spinner is negative");

			this->step = v;
		}

		/// Return true if 'val' is in the valid range of this spinner's value.
		virtual bool isValidValue(Type val)
		{
			return (val >= this->minValue) && (val <= this->maxValue);
		}

		//overrides AbstractSpinner's
		virtual void refresh()
		{
			AbstractSpinner::refresh();
			this->needsRefresh = false;
		}

		//overrides AbstractSpinner's
		virtual void draw()
		{
			if(this->needsRefresh)
				this->refresh();
			else
				this->AbstractSpinner::draw();
		}

		//implements as declared by AbstractSpinner
		/// Increments the spinner value by 'step'.
		virtual void incrementValue()
		{
			if(this->maxValue - *(this->value) > this->step)  //if there is room for 'step'-sized increment, do it
				*(this->value) += this->step;
			else
				*(this->value) = this->maxValue;  // if there is no room for a 'step'-size increment, just set value as max.

			this->refresh();
		}

		//implements as declared by AbstractSpinner
		/// Decrements the spinner value by 'step'.
		virtual void decrementValue()
		{
			if(*(this->value) - this->minValue > this->step)  // if there is room for 'step'-sized decrement, do it
				*(this->value) -= this->step;
			else
				*(this->value) = this->minValue;  // if there is no room for a 'step'-size decrement, just set value as min.

			this->refresh();
		}

		//implements as declared by AbstractSpinner
		virtual void assignValue(const char* txtVal)
		{
			if(parseable<Type>(std::string(txtVal))) //if we can figure out something from the field
			{
				Type val = parse<Type>(std::string(txtVal));
				if(this->isValidValue(val)) //if value type is inside bounds
					*(this->value) = val;
			}
		}

		//implements as declared by AbstractSpinner
		/// Returns a string representation of the spinner's current value. The value type must be compatible with operator + and string
		virtual std::string valueToString()
		{
			return std::string() + *(this->value);
		}
	};
}

#endif /* WIDGETS_SPINNER_HPP_ */
