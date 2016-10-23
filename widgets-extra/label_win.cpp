
#include "label_win.hpp"

#include "widgets_util.hpp"

using std::string;
using std::vector;

using WidgetsExtra::LabelWin;
using WidgetsExtra::MultiLineLabelWin;

LabelWin::LabelWin(WinBase* pw, Rect bounds, string str, Id id)
: WinBase(pw, null, bounds.x, bounds.y, (bounds.w > 0? bounds.w : draw_ttf->text_width(str.c_str())), (bounds.h > 0? bounds.h : TTF_FontHeight(draw_ttf->ttf_font)), 0, id),
  txt(str), textRenderer(draw_ttf)
{}

LabelWin::~LabelWin(){}

void LabelWin::draw()
{
	this->init_gui();
	SDL_FillRect(this->win, 0, this->parent->bgcol);
	this->textRenderer->draw_string(this->win, this->txt.c_str(), Point(0, 0));
}

void LabelWin::setTextRenderer(RenderText* txtrnd)
{
	this->textRenderer = txtrnd;
	this->widen(this->tw_area.w - txtrnd->text_width(this->txt.c_str()), this->tw_area.h - TTF_FontHeight(txtrnd->ttf_font)); //fixme this overrides user-specified size
}

// ============================================ Multi line label win =====================================================================

static Rect& getRectWithHeight(Rect&, unsigned, std::string&, RenderText*);

MultiLineLabelWin::MultiLineLabelWin(WinBase* pw, Rect bounds, string str, unsigned padding, Id id)
: LabelWin(pw, getRectWithHeight(bounds, padding, str, draw_ttf), str, id),
  multiRender(draw_ttf, null, Point(), padding)
{
	multiRender.setText(str, bounds.w);
}

MultiLineLabelWin::~MultiLineLabelWin(){}

void MultiLineLabelWin::draw()
{
	this->init_gui();
	SDL_FillRect(this->win, 0, this->parent->bgcol);
	multiRender.draw(this->win);
}

void MultiLineLabelWin::widen(int dx, int dy)
{
	this->widen(dx, dy);
	this->multiRender.setText(this->txt, this->tw_area.w);
}

void MultiLineLabelWin::setTextRenderer(RenderText* txtrnd)
{
	LabelWin::setTextRenderer(txtrnd);
	this->multiRender.renderText = txtrnd;
	this->multiRender.setText(this->txt, this->tw_area.w);
}

Rect& getRectWithHeight(Rect& bounds, unsigned padding, std::string& str, RenderText* render)
{
	if(bounds.h == 0)
	{
		vector<string> tmp;
		bounds.h = WidgetsExtra::getLineWrappedText(str, render, bounds.w - 2*padding, &tmp)->size() * TTF_FontHeight(render->ttf_font) + padding;
	}
	return bounds;
}
