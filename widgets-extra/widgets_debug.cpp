/*
 * widgets_debug.cpp
 *
 *  Created on: 21 de jun de 2016
 *      Author: carlosfaruolo
 */

#include "widgets_debug.hpp"

#include <iostream>
#include <cstdio>

#include "toogle_button.hpp"
#include "abstract_layout.hpp"
#include "spinner.hpp"
#include "label_win.hpp"
#include "dialog_bgrwin.hpp"
#include "file_dialog.hpp"
#include "scrollable_pane.hpp"
#include "list_win.hpp"
#include "icon_button.hpp"
#include "tabs.hpp"

namespace WidgetsExtra
{
	static const char *title_string(WinBase *child) {
		const char *typ=0;
		static char name[50];
		if (child->title_str)
			snprintf(name,50,"\"%s\"",child->title_str);
		else if (child->id.id1)
			sprintf(name,"id=%s",id2s(child->id.id1));
		else name[0]=0;

		//extra widgets
		if (!typ) {
			TabbedPane *wb=dynamic_cast<TabbedPane*>(child);
			if (wb) typ="(TabbedPane)";
		}
		if (!typ) {
			IconButton *wb=dynamic_cast<IconButton*>(child);
			if (wb) typ="(IconButton)";
		}
		if (!typ) {
			IconToogleButton *wb=dynamic_cast<IconToogleButton*>(child);
			if (wb) typ="(IconToogleButton)";
		}
		if (!typ) {
			ToogleButton *wb=dynamic_cast<ToogleButton*>(child);
			if (wb) typ="(ToogleButton)";
		}
		if (!typ) {
			Layout::Separator *wb=dynamic_cast<Layout::Separator*>(child);
			if (wb) typ="(Layout::Separator)";
		}
		if (!typ) {
			AbstractSpinner *wb=dynamic_cast<AbstractSpinner*>(child);
			if (wb) typ="(AbstractSpinner)";
		}
		if (!typ) {
			MultiLineLabelWin *wb=dynamic_cast<MultiLineLabelWin*>(child);
			if (wb) typ="(MultiLineLabelWin)";
		}
		if (!typ) {
			LabelWin *wb=dynamic_cast<LabelWin*>(child);
			if (wb) typ="(LabelWin)";
		}
		if (!typ) {
			FileDialog *wb=dynamic_cast<FileDialog*>(child);
			if (wb) typ="(FileDialog)";
		}
		if (!typ) {
			DialogBgrWin *wb=dynamic_cast<DialogBgrWin*>(child);
			if (wb) typ="(DialogBgrWin)";
		}
		if (!typ) {
			ScrollablePane *wb=dynamic_cast<ScrollablePane*>(child);
			if (wb) typ="(ScrollablePane)";
		}
		if (!typ) {
			AbstractListWin *wb=dynamic_cast<AbstractListWin*>(child);
			if (wb) typ="(AbstractListWin)";
		}

		// original widgets
		if (!typ) {
			Button *wb=dynamic_cast<Button*>(child);
			if (wb) { typ="(Button)"; if (!name[0] && wb->label.str) sprintf(name,"label=\"%s\"",wb->label.str); }
		}
		if (!typ) {
			BgrWin *wb=dynamic_cast<BgrWin*>(child);
			if (wb) typ="(BgrWin)";
		}
		if (!typ) {
			HSlider *wb=dynamic_cast<HSlider*>(child);
			if (wb) typ="(HSlider)";
		}
		if (!typ) {
			VSlider *wb=dynamic_cast<VSlider*>(child);
			if (wb) typ="(VSlider)";
		}
		if (!typ) {
			HVSlider *wb=dynamic_cast<HVSlider*>(child);
			if (wb) typ="(HVSlider)";
		}
		if (!typ) {
			TextWin *wb=dynamic_cast<TextWin*>(child);
			if (wb) typ="(TextWin)";
		}
		if (!typ) {
			RButWin *wb=dynamic_cast<RButWin*>(child);
			if (wb) typ="(RButWin)";
		}
		if (!typ) {
			RExtButton *wb=dynamic_cast<RExtButton*>(child);
			if (wb) { typ="(RExtButton)"; if (!name[0] && wb->label.str) sprintf(name,"label=\"%s\"",wb->label.str); }
		}
		if (!typ) {
			CheckBox *wb=dynamic_cast<CheckBox*>(child);
			if (wb) { typ="(CheckBox)"; if (!name[0] && wb->label.str) sprintf(name,"label=\"%s\"",wb->label.str); }
		}
		if (!typ) {
			HScrollbar *wb=dynamic_cast<HScrollbar*>(child);
			if (wb) typ="(HScrollbar)";
		}
		if (!typ) {
			VScrollbar *wb=dynamic_cast<VScrollbar*>(child);
			if (wb) typ="(VScrollbar)";
		}
		if (!typ) {
			EditWin *wb=dynamic_cast<EditWin*>(child);
			if (wb) typ="(EditWin)";
		}
		if (!typ) {
			Dial *wb=dynamic_cast<Dial*>(child);
			if (wb) typ="(Dial)";
		}
		if (!typ) {
			DialogWin *wb=dynamic_cast<DialogWin*>(child);
			if (wb) typ="(DialogWin)";
		}

		if (!typ)
			typ="WinBase or unknown subclass";
		static char ret[100];
		snprintf(ret,100,"%s %s",typ,name);
		return ret;
	}

	static void print_widget_hcy(int indent, WinBase *top) {
	  for (int i=0;i<=top->lst_child;++i) {
	    for (int n=0;n<indent;++n) fputs("   ",stdout);
	    WinBase *child=top->children[i];
	    fputs(title_string(child),stdout);
	    if (child->ontopw) fputs(" (on-top)",stdout);
	    if (child->hidden) fputs(" (hidden)",stdout);
	    putchar('\n');
	    print_widget_hcy(indent+1,child);
	  }
	}

	void print_hierarchy(TopWin* topWindow)
	{
		puts("top-win");
		print_widget_hcy(1, topWindow);
		puts("can't query on-top windows");
		fflush(stdout);
	}
}

