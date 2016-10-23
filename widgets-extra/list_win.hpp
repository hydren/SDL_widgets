/*
 * list_win.hpp
 *
 *  Created on: 14 de jun de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_LIST_WIN_HPP_
#define WIDGETS_LIST_WIN_HPP_

#include "SDL_widgets.h"
#include "list_selection_model.hpp"
#include "util.hpp"

namespace WidgetsExtra
{
	struct AbstractListWin extends BgrWin
	{
		Style style;
		Point padding;
		unsigned spacing;
		RenderText* textRenderer, *textRendererCaseSelected;

		ListSelectionModel selection;

		/// Advanced flag used to tweak the behavior of clicking on the list.
		/// If true, the list won't be re-drawed when clicked.
		bool preventRedrawOnClick;

		/// Optional flag to control whether this list is allowed to scroll through mouse wheel.
		/// There is a marginal performance gain when setting this flag false.
		/// If this ListWin isn't child of a ScrollablePane or you just want to disable mouse wheel scrolling, you should set this flag false.
		bool enableScrollingIfScrollablePaneParent;

		//can be called only after a TopWin has been constructed
		AbstractListWin(WinBase *parent, Style, Rect rect, Id id=0);
		virtual ~AbstractListWin();

		virtual void draw();
		virtual unsigned getListHeight();
		virtual unsigned getListWidth() =0;
		virtual void clickList(const Point& point);

		// Pure virtuals, needs implementation from child class
		virtual unsigned size() =0;
		virtual std::string getAsString(unsigned index) =0;

		protected:
		unsigned lastClickedIndex;
		virtual void onMouseDown(Point point, int buttonNumber);
		virtual void onMouseMove(Point point, int buttonNumber); // by default does nothing
		virtual void onMouseUp(Point point, int buttonNumber); // by default does nothing
		RenderText* getResolvedTextRenderer();
		RenderText* getResolvedTextRendererCaseSelected();
		static RenderText* draw_white_ttf;

		private:
		//compatibility function (with BgrWin pointers)
		static void drawBgrWinAsListWin(BgrWin* bgrWin);
		static void onMouseDown(BgrWin* bgr,int x,int y,int but);
		static void onMouseMove(BgrWin* bgr,int x,int y,int but);
		static void onMouseUp(BgrWin* bgr,int x,int y,int but);
	};

	/// An improved alternative of TextWin.
	template<typename Type>
	struct ListWin extends AbstractListWin
	{
		ListWin(WinBase *parent, Style style, Rect rect, Id id=0)
		: AbstractListWin(parent, style, rect, id),
		  listData(new std::vector<Type>()), ownData(true),

		  //advanced usage
		  stringfyFunction(stringfy_by_cast<Type>),
		  selectionAdjustmentFunction(null)
		{}

		ListWin(WinBase *parent, Style style, Rect rect, std::string (*stringfyFunction)(Type value), Id id=0)
		: AbstractListWin(parent, style, rect, id),
		  listData(new std::vector<Type>()), ownData(true),

		  //advanced usage
		  stringfyFunction(stringfyFunction),
		  selectionAdjustmentFunction(null)
		{}

		virtual ~ListWin() { if(ownData) delete listData; }

		virtual unsigned size() { return listData->size(); }
		virtual std::string getAsString(unsigned index)
		{
			if(stringfyFunction != null)
				return stringfyFunction(listData->at(index)); // if a stringfy function was specified, use it
			else
				return "<unstringable value>";
		}

		virtual unsigned getListWidth()
		{
			RenderText* render = this->getResolvedTextRendererCaseSelected();
			unsigned max = 0;
			for(unsigned i = 0, currentSize; i < listData->size(); i++)
			{
				currentSize = render->text_width(getAsString(i).c_str());
				if(i == 0 or currentSize > max) max = currentSize;
			}
			return max;
		}

		virtual std::vector<Type>* getListData() { return listData; }
		virtual void setListData(std::vector<Type>* newData, bool alsoDeletePrevious=false)
		{
			//adjust selection to new data
			this->updateSelection(newData);

			std::vector<Type>* previous = listData;
			this->listData = newData;

			if(alsoDeletePrevious or ownData) delete previous;
			ownData = false;
		}

		protected:
		std::vector<Type>* listData;
		bool ownData; // tells whether the listData was created by this instance. if true, then it will be deleted when this list is deleted or its listData is replaced.

		/// A function pointer to a procedure to get a string value of the specified value.
		/// By default, the action is to try to cast to std::string, but other actions can be specified.
		/// See String::Callbacks for a useful set of templates of functions.
		std::string (*stringfyFunction)(Type value);

		void updateSelection(std::vector<Type>* newData)
		{
			if(this->selectionAdjustmentFunction == null)  //no special selection adjuster specified
			{
				//default procedure, clear selection
				this->selection.fit(newData->size());
				this->selection.clear();
			}
			else this->selectionAdjustmentFunction(this->selection, this->listData, newData);
		}

		public:

		/// Optional function to be called to adjust this model's selection when changes occur in the list data related to this selection model.
		/// If it is null (default), the selection will be cleared everytime the inner data changes.
		void (*selectionAdjustmentFunction)(ListSelectionModel& selection, const std::vector<Type>* oldDataPtr, const std::vector<Type>* newDataPtr);
	};
}

#endif /* WIDGETS_LIST_WIN_HPP_ */
