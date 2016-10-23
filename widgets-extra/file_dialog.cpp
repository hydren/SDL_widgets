/*
 * file_dialog.cpp
 *
 *  Created on: 11 de mai de 2016
 *      Author: carlosfaruolo
 */

#include "file_dialog.hpp"

#include <map>
#include <unistd.h>

#include "widgets_util.hpp"

using WidgetsExtra::FileDialog;
using std::map;
using std::vector;
using std::string;

static const char *home_btn_xpm[] = {
"13 12 3 1",
"  c None",
"x c #101010",
"- c #e0e0e0",
"      x      ",
"     xxx     ",
"    xx xxxx  ",
"   xx x xxx  ",
"  xx xxx xx  ",
" xx xxxxx xx ",
"xx xxxxxxx xx",
"   xxxxxxx   ",
"   xxxxxxx   ",
"   xx   xx   ",
"   xx   xx   ",
"   xx   xx   "};

char buffer[1024];

//xxx kludges to maintain references when calling FileDialog's static callbacks
namespace FileDialog_static
{
	int lastId = 0;
	map<int, FileDialog*> references;

	const char* modeTitle(FileDialog::FileDialogMode mode)
	{
		switch(mode)
		{
			case FileDialog::SELECT_FILE: 	return "Choose file...";
			case FileDialog::SAVE_FILE: 	return "Save file...";
			case FileDialog::SELECT_FOLDER: return "Choose folder...";
			default: 						return "File dialog...";
		}
	}

	const char* modeCurrentDirectory(FileDialog::FileDialogMode mode)
	{
		switch(mode)
		{
			case FileDialog::SELECT_FILE: 	return "Look in:  ";
			case FileDialog::SAVE_FILE: 	return "Save in:  ";
			case FileDialog::SELECT_FOLDER:	return "Folder:   ";
			default: 						return "Path:     ";
		}
	}

	const char* modeFilename(FileDialog::FileDialogMode mode)
	{
		switch(mode)
		{
			case FileDialog::SELECT_FILE:	return "Filename: ";
			case FileDialog::SAVE_FILE:		return "Name:     ";
			default:						return "<name>:   ";
		}
	}
}

FileDialog::FileDialog(FileDialogMode mode, void (*onFinished)(FileDialog*), const vector<string>& fileTypes, Style dialogStyle, Style bntStyle)
: DialogBgrWin(Rect(0, 0, 400, mode==SELECT_FOLDER? 96 : 160), FileDialog_static::modeTitle(mode), null, dialogStyle),
  onFinishedCallback(onFinished), selectedFilename(), mode(mode),
  lblCurrentDirectory(this, Rect(), FileDialog_static::modeCurrentDirectory(mode)),
  cmdmCurrentDirectoryField(new Button(this, Style(4,0), Rect(0, 0, titleBarArea.w * 0.75, 1.5 * TTF_FontHeight(draw_ttf->ttf_font)), getcwd(buffer, 1024), FileDialog::triggerNavigation)),
  btnGoHome(this, bntStyle, Rect(0, 0, cmdmCurrentDirectoryField.src->tw_area.h, cmdmCurrentDirectoryField.src->tw_area.h+1), Label(""), create_pixmap(home_btn_xpm), FileDialog::navigateToHome),
  lblFilename(this, Rect(), FileDialog_static::modeFilename(mode)),
  dlgwFilenameField(this, Rect(0, 0, titleBarArea.w * 0.75, 1.25 * TTF_FontHeight(draw_ttf->ttf_font))),
  ddmFileType(null), availableFileTypes(fileTypes),
  layoutSouthButtons(0, 0, 400),
  btnOk(this, bntStyle, Rect(), "  Ok  ", FileDialog::confirmation),
  btnCancel(this, bntStyle, Rect(), "Cancel", FileDialog::cancellation)
{
	this->id = FileDialog_static::lastId++; //getting new (presumedly unique) id
	FileDialog_static::references[id.id1] = this; //binding this id to this address
	dlgwFilenameField.cmd = FileDialog::getFieldText; //setting callback for dok()
	dlgwFilenameField.cmd_id = id.id1; //setting id of this FileDialog of reference purposes
	this->btnClose.cmd = FileDialog::cancellation; //replacing callback when closing

	layoutSouthButtons.spacing_h = 8;
	layoutSouthButtons.addComponent(new WidgetsExtra::Layout::Spacer(Rect()));
	layoutSouthButtons.addComponent(btnOk); packLabeledComponent(&btnOk);
	layoutSouthButtons.addComponent(btnCancel); packLabeledComponent(&btnCancel);

	if(mode == SELECT_FOLDER)
	{
		lblFilename.hidden = true;
		dlgwFilenameField.hidden = true;
	}
	else
	{
		DropDownMenuFactory ddmf;
		ddmf.setLabel("File type:  ");
		ddmf.setAppearance(DropDownMenuFactory::COMBOBOX);
		ddmf.setSize(Rect(0, 0, dlgwFilenameField.tw_area.w * 0.75, 1.25 * TTF_FontHeight(draw_ttf->ttf_font)));
		ddmf.setCallback(FileDialog::selectFileType);

		ddmf.addItem("All files"); selectedType = "All files";
		const_foreach(const string&, str, vector<string>, availableFileTypes)
		{
			ddmf.addItem(Label(str.c_str()));
		}

		ddmFileType = ddmf.createAt(this, id);
	}

	this->validate();
}

FileDialog::~FileDialog()
{
	delete ddmFileType;
}

void FileDialog::bind()
{
	this->keep_on_top(); //binds to main window
	this->bgcol = parent->bgcol; //inherit background color
	this->dlgwFilenameField.bgcol = parent->bgcol; //black prior to this call
	this->draw_blit_recur();
	this->upd();
}

void FileDialog::validate()
{
	//shift properly all components inside it
	DialogBgrWin::validate();
	setComponentPosition(&lblCurrentDirectory, 8, titleBarArea.y + titleBarArea.h + 12, true);
	setComponentPosition(cmdmCurrentDirectoryField.src, lblCurrentDirectory.area.x + lblCurrentDirectory.tw_area.w + 6, lblCurrentDirectory.area.y - 2, true);
	setComponentPosition(&btnGoHome, cmdmCurrentDirectoryField.src->area.x + cmdmCurrentDirectoryField.src->tw_area.w + 6, lblCurrentDirectory.area.y - 3, true);
	setComponentPosition(&lblFilename, 8, lblCurrentDirectory.area.y + cmdmCurrentDirectoryField.src->tw_area.h + 16);
	setComponentPosition(&dlgwFilenameField, cmdmCurrentDirectoryField.src->area.x, lblFilename.area.y - TTF_FontHeight(draw_ttf->ttf_font), true);
	if(ddmFileType != null) ddmFileType->setPosition(Point(8, dlgwFilenameField.area.y + dlgwFilenameField.tw_area.h + 24));

	layoutSouthButtons.position.y = this->tw_area.h - btnOk.tw_area.h - 6;
	layoutSouthButtons.pack();
}

void FileDialog::replaceSelectedFilename(const char* path, const char* filename)
{
	//todo review usage of / character as file separator here.
	selectedFilename = string(path) + '/' + filename;
}

//================================= callbacks / static functions ===========================================

//to be called by dlgwFilenameField when we call dok() on it in the FileDialog::confirmation() call
void FileDialog::getFieldText(const char* txt, int id1)
{
	FileDialog* self = FileDialog_static::references[id1];
	self->replaceSelectedFilename(self->cmdmCurrentDirectoryField.src->label.str, txt);
}

//called when clicking on the navigation field
void FileDialog::triggerNavigation(Button* navButton)
{
	FileDialog* self = static_cast<FileDialog*>(navButton->parent);
	Point fchooserPos(navButton->tw_area.x + navButton->tw_area.w * 0.5, navButton->tw_area.y + navButton->tw_area.h * 0.5);

	if(self->mode == SELECT_FILE)
		file_chooser(FileDialog::fileSelected, self->id, fchooserPos.x, fchooserPos.y);
	else if(self->mode == SAVE_FILE || self->mode == SELECT_FOLDER)
		working_dir(FileDialog::folderOpened, self->id, fchooserPos.x, fchooserPos.y);
}

//called when selected a file when browsing
void FileDialog::fileSelected(const char* path, Id id)
{
	FileDialog* self = FileDialog_static::references[id.id1];

	self->cmdmCurrentDirectoryField.src->label = getcwd(buffer, 1024);
	self->cmdmCurrentDirectoryField.src->draw_blit_upd();
	self->dlgwFilenameField.dialog_def(path, self->dlgwFilenameField.cmd, self->dlgwFilenameField.cmd_id);
	self->replaceSelectedFilename(getcwd(buffer, 1024), path); //retrieve filename
	free((void*) path);
}

//called when changed folder when browsing
void FileDialog::folderOpened(const char* path, Id id)
{
	FileDialog* self = FileDialog_static::references[id.id1];

	self->cmdmCurrentDirectoryField.src->label = getcwd(buffer, 1024);
	self->cmdmCurrentDirectoryField.src->draw_blit_upd();
	self->replaceSelectedFilename(getcwd(buffer, 1024)); //retrieve folder name
	free((void*) path);
}

//called when the user press the "Home" button
void FileDialog::navigateToHome(Button* navButton) //fixme FileDialog::navigateToHome doesn't update file_chooser() bgr
{
	FileDialog* self = static_cast<FileDialog*>(navButton->parent);

	chdir(getenv("HOME")!=null? getenv("HOME") : getenv("USERPROFILE"));
	self->cmdmCurrentDirectoryField.src->label = getcwd(buffer, 1024);
	self->cmdmCurrentDirectoryField.src->draw_blit_upd();
}

void FileDialog::selectFileType(RButWin* selection, int nr, int fire)
{
	if(fire)
	{
		FileDialog* self = FileDialog_static::references[selection->id.id1];
		self->ddmFileType->cmdMenu->src->label = selection->act_button()->label.str;
		self->ddmFileType->cmdMenu->src->draw_blit_upd();
		self->selectedType = selection->act_button()->label.str;
	}
}

//called when we click Ok button
void FileDialog::confirmation(Button* okBtn)
{
	FileDialog* self = static_cast<FileDialog*>(okBtn->parent);
	if(self->mode == SAVE_FILE) //filename was not retrieved
		self->dlgwFilenameField.dok(); //retrieve filename (typed by user)
	if(self->onFinishedCallback != null) self->onFinishedCallback(self);
	self->close();
	close_file_chooser();
}

//called when we click Cancel or 'X' button
void FileDialog::cancellation(Button* cancelBtn)
{
	FileDialog* self = static_cast<FileDialog*>(cancelBtn->parent);
	self->selectedFilename.clear();
	if(self->onFinishedCallback != null) self->onFinishedCallback(self);
	self->close();
	close_file_chooser();
}

