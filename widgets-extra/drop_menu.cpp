/*
 * drop_menu.cpp
 *
 *  Created on: 18 de abr de 2016
 *      Author: carlosfaruolo
 */

#include "drop_menu.hpp"

#include <algorithm>

using std::vector;

using WidgetsExtra::LabelWin;
using WidgetsExtra::DropDownMenu;
using WidgetsExtra::DropDownMenuFactory;

#define max std::max

// DropDownMenu ===========================================================================================

DropDownMenu::~DropDownMenu()
{
	//fixme needs a way to delete the buttons inside
	delete cmdMenu;
}

Point DropDownMenu::getPosition() const
{
	return cmdMenu->src->area;
}

void DropDownMenu::setPosition(Point position)
{
	WidgetsExtra::Layout::WinBaseWrapper::setWinBasePosition(this->label, position);

	//apply offset due to label
	if(this->isLabelOnTop) position.y += this->label->tw_area.h;
	else position.x += this->label->tw_area.w;

	WidgetsExtra::Layout::WinBaseWrapper::setWinBasePosition(this->cmdMenu->src, position);
}

Rect DropDownMenu::getSize() const
{
	Rect rt;

	if(this->isLabelOnTop)
	{
		rt.w = max(cmdMenu->src->tw_area.w, this->label->tw_area.w);
		rt.h = cmdMenu->src->tw_area.h + this->label->tw_area.h;
	}
	else
	{
		rt.w = cmdMenu->src->tw_area.w + this->label->tw_area.w;
		rt.h = max(cmdMenu->src->tw_area.h, this->label->tw_area.h);
	}

	return cmdMenu->src->tw_area;
}

void DropDownMenu::setSize(Rect size)
{
	this->cmdMenu->src->widen(size.w - this->cmdMenu->src->tw_area.w, size.h - this->cmdMenu->src->tw_area.h);
}

bool DropDownMenu::isStretched() const
{
	return false; //maybe true afterwards as setSize is ready, but untested
}

bool DropDownMenu::operator == (const Element& b) const
{
	return &b == this;
}

// DropDownMenuButton =========================================================================================================

namespace WidgetsExtra
{
	struct DropDownMenuButton extends Button
	{
		DropDownMenu* menu;
		DropDownMenuFactory::Implementation* factory;
		DropDownMenuButton(WinBase *pw, Style st, Rect rt, Label lab, void (*someCmd)(Button*), Id id=0)
		: Button(pw, st, rt, lab, someCmd, id), menu(null), factory(null) {}
		virtual ~DropDownMenuButton(); //defined at the end of the file
	};
}

// DropDownMenuFactory (encapsulation) ==========================================================================================

struct DropDownMenuFactory::Implementation
{
	Implementation()
	: pw(null), apre(), rt(), lb(""), onTop(false), id(0), items(), menuItemCmd(null) {}

	WinBase* pw;
	Appearance apre;
	Rect rt;
	Label lb; bool onTop;
	Id id;
	vector<Label> items;
	void (*menuItemCmd)(RButWin*,int nr,int fire);

	//do the steps needed to initialize the CmdMenu wrapped inside the DropDownMenu
	static void buildCallback(Button* btn)
	{
		DropDownMenu* menu = ((DropDownMenuButton*) btn)->menu; //deference the menu
		DropDownMenuFactory::Implementation* factory = ((DropDownMenuButton*) btn)->factory; //deference the menu

		menu->cmdMenu->init(factory->rt.w, factory->items.size(), factory->menuItemCmd);
		for(unsigned i = 0; i < factory->items.size(); i++)
			menu->cmdMenu->add_mbut(factory->items[i]);
	}

	Style toStyle(Appearance ap)
	{
		switch(ap)
		{
			default:
			case BUTTON: return Style(0,0);
			case COMBOBOX: return Style(4,0);
			case MENU: return Style(2,1,20);
		}
	}

	//crafts a button that initializes a CmdMenu with this' parameters
	DropDownMenuButton* createButton(DropDownMenu* owner)
	{
		DropDownMenuButton* ddmb = new DropDownMenuButton(pw, toStyle(apre), rt, items[0], buildCallback, id);
		ddmb->menu = owner;
		ddmb->factory = this;
		return ddmb;
	}
};

// DropDownMenuFactory =============================================================================================================

DropDownMenuFactory::DropDownMenuFactory()
: implementation(new Implementation()) {}

DropDownMenuFactory::~DropDownMenuFactory()
{
	delete implementation;
}

void DropDownMenuFactory::setLabel(Label lbl, bool onTop)
{
	this->implementation->lb = lbl;
	this->implementation->onTop = onTop;
}

void DropDownMenuFactory::setCallback(void (*menu_cmd)(RButWin*,int nr,int fire))
{
	this->implementation->menuItemCmd = menu_cmd;
}

void DropDownMenuFactory::setAppearance(Appearance apre)
{
	this->implementation->apre = apre;
}

void DropDownMenuFactory::setSize(Rect rt)
{
	this->implementation->rt = rt;
}

void DropDownMenuFactory::addItem(Label label, short index)
{
	if(index == -1)
		this->implementation->items.push_back(label);
	else
		this->implementation->items.insert(this->implementation->items.begin() + index, label);
}

Label DropDownMenuFactory::itemAt(unsigned index)
{
	return this->implementation->items[index];
}

void DropDownMenuFactory::removeItem(short index)
{
	this->implementation->items.erase(this->implementation->items.begin() + index);
}

void DropDownMenuFactory::removeAllItems()
{
	this->implementation->items.clear();
}

DropDownMenu* DropDownMenuFactory::createAt(WinBase* pw, Point pt, Id id)
{
	this->implementation->rt.x = pt.x;
	this->implementation->rt.y = pt.y;
	return this->createAt(pw, id);
}

DropDownMenu* DropDownMenuFactory::createAt(WinBase* pw, Id id)
{
	this->implementation->pw = pw;
	this->implementation->id = id;

	DropDownMenu* ddmenu = new DropDownMenu(); //create instance

	//label dimensions
	Rect labelArea(
		this->implementation->rt.x,
		this->implementation->rt.y,
		draw_ttf->text_width(this->implementation->lb.str),
		TTF_FontHeight(draw_ttf->ttf_font));

	//var to decide where to put the label (top or left)
	ddmenu->isLabelOnTop = this->implementation->onTop;

	//adjust 'this->implementation.rt' according to the label orientation (top or left). createButton() will use it.
	if(ddmenu->isLabelOnTop)
		this->implementation->rt.y += labelArea.h;
	else
		this->implementation->rt.x += labelArea.w;

	//create CmdMenu with specially crafted button.
	ddmenu->cmdMenu = new CmdMenu(static_cast<Button*>(this->implementation->createButton(ddmenu)));

	//create the label
	ddmenu->label = new LabelWin(pw, labelArea, this->implementation->lb.str);

	/* Before return, create a new implementation instance, copy of the previous,
	   as the previous instance is being referenced by the DropDownMenu */
	this->implementation = new Implementation(*(this->implementation));
	return ddmenu;
}

// ===========
WidgetsExtra::DropDownMenuButton::~DropDownMenuButton(){ delete factory; }
