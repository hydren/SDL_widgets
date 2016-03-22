/*  
    Demo program SDL-widgets-1.0
    Copyright 2011-2013 W.Boeke

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program.
*/

#include <SDL_widgets.h>

TopWin *top_win;
Button *but;

void topw_disp() {
  top_win->clear();
  draw_title_ttf->draw_string(top_win->win,"Hello world!",Point(20,40));
}

void button_cmd(Button* b) {
  top_win->clear(&b->tw_area,cBackground,true);
  static int y=10;
  y= y==10 ? 70 : 10;
  b->area.y=b->tw_area.y=y;
}

int main(int,char**) {
  top_win=new TopWin("Hello",Rect(100,100,120,100),SDL_INIT_VIDEO,0,topw_disp);
  but=new Button(top_win,0,Rect(5,10,60,0),"catch me!",button_cmd);
  get_events();
  return 0;
}
