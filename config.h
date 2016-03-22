#ifndef SDL_WIDGETS_CONFIG_H_
#define SDL_WIDGETS_CONFIG_H_

//Replace this for a proper configuration file (or remove it altogether)

#ifdef _WIN32
	const char* FONTPATH="C:\\Windows\\Fonts\\FreeSans.ttf";
	const char* FONTPATH_BOLD="C:\\Windows\\Fonts\\FreeSansBold.ttf";
	const char* FONTPATH_MONO="C:\\Windows\\Fonts\\FreeMono.ttf";
#else
	const char* FONTPATH="/usr/share/fonts/truetype/freefont/FreeSans.ttf";
	const char* FONTPATH_BOLD="/usr/share/fonts/truetype/freefont/FreeSansBold.ttf";
	const char* FONTPATH_MONO="/usr/share/fonts/truetype/freefont/FreeMono.ttf";
#endif

#endif /* SDL_WIDGETS_CONFIG_H_ */
