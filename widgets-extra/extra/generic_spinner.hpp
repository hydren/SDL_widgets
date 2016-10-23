/*
 * generic_spinner.hpp
 *
 *  Created on: 7 de jul de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_EXTRA_GENERIC_SPINNER_HPP_
#define WIDGETS_EXTRA_GENERIC_SPINNER_HPP_

#include "../../SDL_widgets.h"
#include "../abstract_layout.hpp"

#include "../util.hpp"

namespace WidgetsExtra
{
	struct SpinnerDataModel
	{
		virtual ~SpinnerDataModel() {};

		/// Should increment the spinner value
		virtual void incrementValue() abstract;

		/// Should decrement the spinner value
		virtual void decrementValue() abstract;

		/// Should, if the text in the field is valid, update the spinner value with a interpretation of the given string.
		virtual void assign(const char* txtVal) abstract;

		/// Should return a string representation of the spinner's current value.
		virtual std::string valueToString() abstract;

		/// (Optional) A callback to be called when the data model changes;
		void (*onValueChange)(SpinnerDataModel*);

		/// A interface to a listener to this model. The callback that is called when the data model changes;
		struct Listener { virtual void onValueChange(SpinnerDataModel*) abstract; };
		ListenerManager<Listener> listeners;
		void notify()
		{
			if(onValueChange != null) onValueChange(this);
			for(unsigned i = 0; i < listeners.size(); i++) listeners[i]->onValueChange(this);
		}
	};

	template<typename Type>
	struct NumberSpinnerDataModel extends SpinnerDataModel
	{
		Type* value, step, min, max;
		virtual ~NumberSpinnerDataModel() {}

		NumberSpinnerDataModel(Type* value=null)
		: value(value==null? new Type(1) : value), step(1), min(0), max(9999) {}

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
			this->notify();

			if(deleteOldValue) delete old;
		}

		/// Updates the spinner value, assigning 'newValue' to it.
		virtual void setValue(const Type& newValue)
		{
			*(this->value) = newValue;
			this->notify();
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
			return (val >= this->min) && (val <= this->max);
		}

		/// Increments the spinner value by 'step'.
		virtual void incrementValue()
		{
			if(isValidValue(*(this->value) + this->step) && *(this->value) + this->step >= *(this->value)) //second clausule checks for overflow
			{
				*(this->value) += this->step;
			}
		}

		/// Decrements the spinner value by 'step'.
		virtual void decrementValue()
		{
			if(isValidValue(*(this->value) - this->step) && *(this->value) - this->step <= *(this->value))
			{
				*(this->value) -= this->step;
			}
		}

		/// If the text in the field is valid, update the spinner value with a interpretation of the given string.
		/// Returns true if the spinner value was changed as a result of this call.
		virtual bool assign(const char* txtVal)
		{
			if(parseable<Type>(std::string(txtVal))) //if we can figure out something from the field
			{
				Type val = parse<Type>(std::string(txtVal));
				if(this->isValidValue(val)) //if value type is inside bounds
					*(this->value) = val;

				return true;
			}
			else return false;
		}
	};

	/// Generic widget that serves as a skeleton to the template struct Spinner.
	struct GenericSpinner extends WinBase, SpinnerDataModel::Listener
	{
		static const unsigned BUTTON_SIZE = 16;

		SpinnerDataModel* model;
		DialogWin dlwTextField;
		Button btnInc, btnDec;
		bool needsRefresh;

		GenericSpinner(WinBase *pw, Rect area, const char* label=null, SpinnerDataModel* model=null);
		virtual ~GenericSpinner();

		virtual void setModel(SpinnerDataModel* model);

		/// Overrides WinBase's
		virtual void draw();

		/// Does NOT override WinBase's. Caution: doesn't work if called from a WinBase pointer as widen() is not virtual in class WinBase.
		virtual void widen(int dx, int dy);

		/// Sets the spinner label
		virtual void setLabel(const char* lbl);

		/// refresh the text according to this spinner's current value.
		virtual void refresh();

		virtual void onValueChange(SpinnerDataModel*);

		//pure virtual. needs to be implemented:

//		/// Should refresh the text according to this spinner's current value.
//		virtual void refresh() abstract;
//
//		/// Should increment the spinner value
//		virtual void incrementValue() abstract;
//
//		/// Should decrement the spinner value
//		virtual void decrementValue() abstract;
//
//		/// Should, if the text in the field is valid, update the spinner value. Otherwise refresh the field text with the spinner value (overriding any text in the field)
//		virtual void parseIfValid(const char* txtVal) abstract;

		protected:
		virtual void onButtonPressed(Button* btn);
		virtual void onFieldInputConfirmation(const char* input);

		private:
		// compatibility callback functions
		static void changeValue(Button* btn);
		static void validateField(const char* text,int cmd_id);
	};

	template <typename Type>
	struct DefaultSpinner extends GenericSpinner
	{
		DefaultSpinner(WinBase *pw, Rect area, const char* label=null, Type* value=null)
		: GenericSpinner(pw, area, label, new NumberSpinnerDataModel<Type>(value))
		{}

		NumberSpinnerDataModel<Type>* getModel() { return static_cast<NumberSpinnerDataModel<Type>*>(model); }
	};

	/** A widget similar to Swing's JSpinner. It is a template, which means the value on the spinner field can be (theorectically) of any type.
	 * Currently it has been tested (and passed) with integer types (int, long and alike) and floating points types (float, double and alike).
	 * The template type must have operator + and -, and be able to cast from literal values 1 and 9999.
	 * Default bounds are 0 (min) and 9999 (max). Set these as needed. Default step value is literal value 1.*/
	template<typename Type>
	struct Spinner2 extends GenericSpinner
	{
		Type* value, step, min, max;
		bool needsRefresh;

		Spinner2(WinBase *pw, Rect area, const char* label=null, Type* value=null)
		: GenericSpinner(pw, area, label),
		  value(value==null? new Type(1) : value), step(1), min(0), max(9999),
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
			return (val >= this->min) && (val <= this->max);
		}

		/// Updates the text field content and redraw's the Spinner.
		virtual void refresh()
		{
			std::string strValue = std::string() + *(this->value);
			this->dlwTextField.dialog_def(strValue.c_str(), this->dlwTextField.cmd, this->dlwTextField.cmd_id);
			this->dlwTextField.unset_cursor();
			this->btnInc.draw_blit_upd();
			this->btnDec.draw_blit_upd();
			this->needsRefresh = false;
		}

		//overrides AbstractSpinner's
		virtual void draw()
		{
			if(this->needsRefresh)
				this->refresh();
			else
				this->GenericSpinner::draw();
		}

		/// Increments the spinner value by 'step'.
		virtual void incrementValue()
		{
			if(isValidValue(*(this->value) + this->step) && *(this->value) + this->step >= *(this->value)) //second clausule checks for overflow
			{
				*(this->value) += this->step;
			}
			this->refresh();
		}

		/// Decrements the spinner value by 'step'.
		virtual void decrementValue()
		{
			if(isValidValue(*(this->value) - this->step) && *(this->value) - this->step <= *(this->value))
			{
				*(this->value) -= this->step;
			}
			this->refresh();
		}

		//overrides AbstractSpinner's
		virtual void parseIfValid(const char* txtVal)
		{
			if(parseable<Type>(std::string(txtVal))) //if we can figure out something from the field
			{
				Type val = parse<Type>(std::string(txtVal));
				if(this->isValidValue(val)) //if value type is inside bounds
					*(this->value) = val;
			}
		}
	};
}




#endif /* WIDGETS_EXTRA_GENERIC_SPINNER_HPP_ */
