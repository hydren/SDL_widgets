/*
 * widgets_util.cpp
 *
 *  Created on: 27 de abr de 2016
 *      Author: carlosfaruolo
 */

#include "widgets_util.hpp"

#include "util.hpp"

using std::vector;
using std::string;

// xxx This function is experimental, but proved to work in many cases. Still, it should be avoided when possible.
void setComponentRawPosition(WinBase* wb, int x, int y)
{
	wb->area.x = x;			wb->area.y = y;
	wb->tw_area.x = x;		wb->tw_area.y = y;
	wb->title_area.x = x;	wb->title_area.y = y-17;
	wb->title_top.x = x;	wb->title_top.y = y-17;
	if (wb->parent)
	{
		wb->tw_area.x=wb->area.x+wb->parent->tw_area.x;
		wb->tw_area.y=wb->area.y+wb->parent->tw_area.y;
	}
}

void WidgetsExtra::setComponentPosition(WinBase* component, int x, int y, bool raw)
{
	if(raw) setComponentRawPosition(component, x, y);
	else component->move(x - component->area.x, y - component->area.y);
}

void WidgetsExtra::packLabeledComponent(WinBase* wb, Label& lbl, unsigned height, int padding)
{
	int properWidth = lbl.render_t->text_width(lbl.str) + 2*padding;
	int properHeight = height > 0? height : TTF_FontHeight(lbl.render_t->ttf_font) + 2*padding;  //if height is 0, use font height plus padding instead
	wb->widen(properWidth - wb->tw_area.w, properHeight - wb->tw_area.h);
}

void WidgetsExtra::packLabeledComponent(Button* btn, unsigned height, int padding)
{
	packLabeledComponent(btn, btn->label, height, padding);
}

void WidgetsExtra::packLabeledComponent(CheckBox* btn, unsigned height, int padding)
{
	int properHeight = height > 0? height : 14;  //if height is 0, use size 14 (check symbol size defined as pixmap)
	int properWidth = btn->label.render_t->text_width(btn->label.str) + 2*padding + properHeight;
	btn->widen(properWidth - btn->tw_area.w, properHeight - btn->tw_area.h);
}

void WidgetsExtra::packLabeledComponent(RExtButton* btn, unsigned height, int padding)
{
	packLabeledComponent(btn, btn->label, height, padding);
}

void WidgetsExtra::drawBgrWin(BgrWin* bgrWin)
{
	bgrWin->init_gui();
	SDL_FillRect(bgrWin->win, null, bgrWin->bgcol); //clears the screen
}

WidgetsExtra::Color5::Color5(Uint32 fst, Uint32 lst)
: ready(false)
{
	this->c[0] = fst;
	this->c[4] = lst;
}

void WidgetsExtra::Color5::set_col(Uint32 fst,Uint32 lst) { // fst, lst e.g.: 0x102030ff, output c[i] e.g: 0x102030
  for (int i=0;i<5;++i) {
    Uint32 cc=0;
    for (int cn=0;cn<3;++cn) {  // cn = red,green,blue
      Uint8 col=((fst>>(cn+1)*8 & 0xff) * (4-i) + (lst>>(cn+1)*8 & 0xff)*i) / 4;
      cc += col<<cn*8;
    }
    c[i]=calc_color(cc);
  }
  ready = true;
}

void WidgetsExtra::Color5::draw_gradient(WinBase* self, Rect rect, bool vertical, bool hollow) const
{
	const_cast<Color5*>(this)->setup();
	const int x[6]={ 0,rect.w/5,rect.w*2/5,rect.w*3/5,rect.w*4/5,rect.w },
			y[6]={ 0,rect.h/5,rect.h*2/5,rect.h*3/5,rect.h*4/5,rect.h };
	for (int i=0;i<5;++i) {
		Uint32 c= hollow ? this->c[4-i] : this->c[i];
		if (vertical)
			SDL_FillRect(self->win,rp(rect.x+x[i],rect.y,x[i+1]-x[i],rect.h),c);
		else
			SDL_FillRect(self->win,rp(rect.x,rect.y+y[i],rect.w,y[i+1]-y[i]),c);
	}
	rectangleColor(self->win,rect.x,rect.y,rect.x+rect.w-1,rect.y+rect.h-1,0x707070ff);
}

void WidgetsExtra::Color5::setup()
{
	if(not ready)
		this->set_col(this->c[0], this->c[4]);
}

const WidgetsExtra::Color5
	WidgetsExtra::Color5::GradientBlue(0xf0ffffff, 0xa0d0e0ff),
	WidgetsExtra::Color5::GradientDarkBlue(0xa0f0ffff, 0x3094d0ff),
	WidgetsExtra::Color5::GradientWheat(0xfff0d7ff, 0xc0c0a0ff),
	WidgetsExtra::Color5::GradientGrey(0xffffffff, 0xc7c7c7ff),
	WidgetsExtra::Color5::GradientDarkGrey(0xf0f0f0ff, 0x707070ff),
	WidgetsExtra::Color5::GradientGreen(0xe0ffe0ff, 0x77c077ff),
	WidgetsExtra::Color5::GradientRose(0xffd0d0ff, 0xd07070ff);


vector<string>* WidgetsExtra::getLineWrappedText(std::string fullText, RenderText* drawer, Uint16 maxWidth, vector<string>* lines)
{
	if(lines == null)
		lines = new vector<string>();

	unsigned lf = 0, cr = 0, ln = 0;

	while( lf + cr < fullText.size() )
	{
		bool exceed = false;

		if(fullText.c_str()[lf+cr] == '\n' ||  lf + cr == fullText.size()-1) // line feed or last draw
			exceed = true;
		else
		{
			string substr = replace_all(fullText.substr(lf, cr+1), "\n", " ");
			if((unsigned) drawer->text_width(substr.c_str()) > maxWidth)
				exceed = true;
		}

		if(exceed)
		{
			//separate from text pointer, 'cr' caracters
			string substr = replace_all(fullText.substr(lf, cr), "\n", " ");
			lines->push_back(substr);
			lf += cr; //increase text pointer
			cr = 0; //"caret" position is reset
			ln++; //line number incremented
		}

		cr++; //increase caret position
	}

	return lines;
}

