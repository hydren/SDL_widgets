/*
 * Simple example code for SDL_widgets 1.0
 *
 * Copyright (c) 2015 Carlos F. M. Faruolo <5carlosfelipe5@gmail.com>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgement in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#include "example.hpp"

TopWin *top_win;
Button *but;

void topw_disp()
{
  top_win->clear();
  draw_title_ttf->draw_string(top_win->win,"Hello world!",Point(20,40));
}

void button_cmd(Button* b)
{
  top_win->clear(&b->tw_area,cBackground,true);
  static int y=10;
  y= y==10 ? 70 : 10;
  b->area.y=b->tw_area.y=y;
}

void Widgets_RunExample()
{
	top_win=new TopWin("Hello",Rect(100,100,120,100),SDL_INIT_VIDEO,0,topw_disp);
	but=new Button(top_win,0,Rect(5,10,60,0),"catch me!",button_cmd);
	get_events();
}
