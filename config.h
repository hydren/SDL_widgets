#ifndef SDL_WIDGETS_CONFIG_H_
#define SDL_WIDGETS_CONFIG_H_

//Replace this for a proper configuration file (or remove it altogether)

#ifdef _WIN32
	const char* FONTPATH="C:\\Windows\\Fonts\\Arial.ttf";
	const char* FONTPATH_BOLD="C:\\Windows\\Fonts\\ArialBold.ttf";
	const char* FONTPATH_MONO="C:\\Windows\\Fonts\\COUR.ttf";
#elif __APPLE__
	const char* FONTPATH="C:\\Windows\\Fonts\\Arial.ttf";
	const char* FONTPATH_BOLD="C:\\Windows\\Fonts\\ArialBold.ttf";
	const char* FONTPATH_MONO="C:\\Windows\\Fonts\\COUR.ttf";
#else
	const char* FONTPATH="/usr/share/fonts/truetype/freefont/FreeSans.ttf";
	const char* FONTPATH_BOLD="/usr/share/fonts/truetype/freefont/FreeSansBold.ttf";
	const char* FONTPATH_MONO="/usr/share/fonts/truetype/freefont/FreeMono.ttf";
#endif

#endif /* SDL_WIDGETS_CONFIG_H_ */
