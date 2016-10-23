/*
 * widgets_util.hpp
 *
 *  Created on: 27 de abr de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_WIDGETS_UTIL_HPP_
#define WIDGETS_WIDGETS_UTIL_HPP_
#include <ciso646>

#include "SDL_widgets.h"

#include <vector>
#include <string>

///Utilities for SDL_widgets
namespace WidgetsExtra
{
	/// Properly sets the position of the given component (better than modifying .area and .tw_area directly). Use raw=true when it doesn't work otherwise.
	void setComponentPosition(WinBase* component, int x, int y, bool raw=false);

	/// Adjusts the given button size according to its label
	void packLabeledComponent(Button* btn, unsigned height=0, int padding=4);

	/// Adjusts the given checkbox size according to its label
	void packLabeledComponent(CheckBox* chck, unsigned height=0, int padding=4);

	/// Adjusts the given external radio button size according to its label
	void packLabeledComponent(RExtButton* rebt, unsigned height=0, int padding=4);

	/// Adjusts the given component size according to the given label
	void packLabeledComponent(WinBase* wb, Label& lbl, unsigned height=0, int padding=4);

	/// Helper class to hold parameters. Purely optional.
	struct LabeledComponentPacker
	{
		unsigned height; int padding;
		LabeledComponentPacker(unsigned height=0, int padding=4) : height(height), padding(padding) {}
		void pack(Button* btn) const { packLabeledComponent(btn, height, padding); }
		void pack(CheckBox* chck) const { packLabeledComponent(chck, height, padding); }
		void pack(RExtButton* rebt) const { packLabeledComponent(rebt, height, padding); }
		void pack(WinBase* wb, Label& lbl) const { packLabeledComponent(wb, lbl, height, padding); }
	};

	/// A (proposed) default draw() function for BgrWin's.
	/// Fills the given BgrWin area with its background color (bgcol).
	void drawBgrWin(BgrWin* bgrWin);

	/// Holds a gradient of 5 colors (copied from SDL_widgets.cpp)
	struct Color5 {
		Uint32 c[5];
		Color5(Uint32 fst,Uint32 lst);
		void set_col(Uint32 fst,Uint32 lst);  // input like: 0x102030ff
		void draw_gradient(WinBase* wb,Rect rect,bool vertical=false,bool hollow=false) const;

		static const Color5
			GradientBlue, GradientDarkBlue, GradientRose, GradientWheat, GradientGreen,
			GradientGrey, GradientDarkGrey;

		protected:
		bool ready;
		void setup();
	};

	/// Returns a vector of strings containing, each string corresponding to a line of a line-wrapped version of the given text.
	/// The line-wrapping is computed as if the text was to be drawn with the given RenderText and a maximum width.
	/// If 'vstr' is not null, it is used to store the result and returned. Otherwise, a new vector is instantiated and returned for that purpose.
	///
	/// @param fullText The text to be split in a vector of lines
	/// @param drawer The RenderText intened to be used when drawing the lines
	/// @param maxWidth The maximum intended width of the lines
	/// @param vstr A pointer to a vector of strings to be used as the container for the result.
	/// @return A vector of std::string's containing the computed lines.
	std::vector<std::string>* getLineWrappedText(std::string fullText, RenderText* drawer, Uint16 maxWidth, std::vector<std::string>* vstr=NULL);
}

#endif /* WIDGETS_WIDGETS_UTIL_HPP_ */
