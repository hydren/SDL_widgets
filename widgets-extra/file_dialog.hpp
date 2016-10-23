/*
 * file_dialog.hpp
 *
 *  Created on: 11 de mai de 2016
 *      Author: carlosfaruolo
 */

#ifndef WIDGETS_FILE_DIALOG_HPP_
#define WIDGETS_FILE_DIALOG_HPP_

#include <string>
#include <vector>

#include "util.hpp"

#include "dialog_bgrwin.hpp"
#include "label_win.hpp"
#include "flow_layout.hpp"
#include "drop_menu.hpp"
#include "icon_button.hpp"

namespace WidgetsExtra
{
	struct FileDialog extends WidgetsExtra::DialogBgrWin
	{
		void (*onFinishedCallback)(FileDialog* dialog);
		std::string selectedFilename, selectedType;

		enum FileDialogMode { SELECT_FILE, SAVE_FILE, SELECT_FOLDER };
		FileDialog(FileDialogMode mode=SELECT_FILE, void (*onFinished)(FileDialog* dialog)=null,
				const std::vector<std::string>& fileTypes=std::vector<std::string>(),
				Style dialogStyle=0, Style bntStyle=0);
		virtual ~FileDialog();

		protected:
		FileDialogMode mode;

		LabelWin lblCurrentDirectory;
		CmdMenu cmdmCurrentDirectoryField;
		IconButton btnGoHome;

		LabelWin lblFilename;
		DialogWin dlgwFilenameField;

		DropDownMenu* ddmFileType;
		const std::vector<std::string> availableFileTypes;

		FlowLayout layoutSouthButtons;
		Button btnOk, btnCancel;

		virtual void bind();
		virtual void validate();

		void replaceSelectedFilename(const char* path, const char* filename="");

		//auxiliary callback
		static void getFieldText(const char* txt,int);

		//navigation callbacks
		static void triggerNavigation(Button*);
		static void fileSelected(const char* path,Id);
		static void folderOpened(const char* path,Id);
		static void navigateToHome(Button*);

		//filetype selection callback
		static void selectFileType(RButWin*,int nr,int fire);

		//finishing callbacks
		static void confirmation(Button*);
		static void cancellation(Button*);
	};
}

#endif /* WIDGETS_FILE_DIALOG_HPP_ */
