#ifndef _SDL_WIDGETS_H
#define _SDL_WIDGETS_H

/*  Copyright 2011 W.Boeke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_ttf.h>

extern Uint32 cWhite, cBlack, cGrey, cRed, cBlue,
              cForeground, cBackground, cBorder, cPointer, cScrollbar;

const int
  TDIST=14;  // distance text lines

struct Int2 {
  int x,y;
  Int2(int x,int y);
  Int2();
  void set(int x,int y);
  bool operator!=(Int2 b);
};

struct Point {
  short x, y;
  Point();
  Point(short x,short y);
  void set(short x,short y);
  bool operator==(Point b);
  bool operator!=(Point b);
};

struct Rect:SDL_Rect {
  Rect();
  Rect(Sint16 x,Sint16 y,Uint16 dx,Uint16 dy);
  void set(Sint16 x,Sint16 y,Uint16 dx,Uint16 dy);
};

static Rect *rp(int x,int y,int w,int h) { // creates pointer to Rect. Each source file gets its own instance.
  static Rect rp_rect;
  rp_rect.set(x,y,w,h);
  return &rp_rect;
}

struct Id {
  int id1,id2;
  Id(int);
  Id(int,int);
};

struct Label {
  struct RenderText *render_t;
  void (*draw_cmd)(SDL_Surface *win,int nr,int y_off);
  const char *str;  // the text
  Label(const char* txt);
  Label(void (*draw_cmd)(SDL_Surface *win,int nr,int y_off));
  Label(const char *txt,void (*dr)(SDL_Surface *win,int nr,int y_off));
  void draw(SDL_Surface *win,int nr,Point pnt);
};

struct Style {
  const int st;
  Uint32 param;
  int param2;
  Style(int st);
  Style(int st,int par);
  Style(int st,int par,int par2);
};

struct RenderText {
  static const int dim=128-' ';
  SDL_Surface *chars[dim+1];
  int ch_wid[dim];
  TTF_Font *ttf_font;
  SDL_Color text_col;
  RenderText(TTF_Font*,SDL_Color text_col);
  int draw_string(SDL_Surface *win,const char *s,Point);
  int text_width(const char *s);
  void set_char(const int ind);
};

struct WinBase {
  SDL_Surface *win,   // parent window
              *title;
  WinBase *parent;
  WinBase **children;
  struct OnTopWin *ontopw;
  const char *title_str;
  Point area;    // relative to parent window
  Rect tw_area,  // relative to top window
       title_area; // title surface
  Point title_top;
  int lst_child,end_child;
  Uint32 bgcol;
  bool hidden;
  Id id;
  WinBase(WinBase *pw,const char *t,int x,int y,int dx,int dy,Uint32 bgcol,Id id);
  ~WinBase();
  void set_parent(WinBase *pw);
  virtual void draw()=0;
  void reloc_title(int dx,int dy); // shift title area
  void set_title(const char* new_t,bool _upd=true);
  void add_child(WinBase* child);
  void remove_child(WinBase* child);
  void clear(Rect *rect=0);
  void clear(Rect*,Uint32 col,bool _upd);
  void hide();
  void show();
  void draw_raised(Rect *rect,Uint32 col,bool up);
  void draw_gradient(Rect rect,const struct Color5* col,bool vertical=false,bool hollow=false);
  void border(WinBase *child,int wid=1);
  WinBase *in_a_win(int x,int y);
  void draw_blit_recur();    // recursivily draw surface, blit own and children's surface plus title to parent
  void draw_blit_upd();      // draw surface, blit surface to parents
  void blit_upd(Rect *rect=0); // blit surface to parents
  void upd(Rect *rect=0);    // update top window
  void widen(int dx,int dy); // widen win
  void move(int dx,int dy);  // move win
  void set_pos(Sint16 x, Sint16 y); // set win position (wrapper to WinBase::move())
  bool move_if_ok(int delta_x,int delta_y); // guarded move
  Point tit_os();            // get title offset
  void keep_on_top();        // make staying visible
  void init_gui();
};

struct TopWin:WinBase {
  void (*display_cmd)();
  TopWin(const char* title,Rect rect,Uint32 init_flag,Uint32 video_flag,void (*draw)(),void (*set_icon)()=0,void (*on_sdl_init)()=0);
  void draw();
};

struct Button:WinBase {
  bool is_down;
  Style style;
  Label label;
  void (*cmd)(Button*);
  Button(WinBase *pw,Style,Rect,Label lab,void (*cmd)(Button*),Id id=0);
  void draw();
};

struct BgrWin:WinBase {  // background window
  void (*display_cmd)(BgrWin*);
  void (*down_cmd)(BgrWin*,int x,int y,int but);
  void (*moved_cmd)(BgrWin*,int x,int y,int but);
  void (*up_cmd)(BgrWin*,int x,int y,int but);
  BgrWin(WinBase *pw,
         Rect,
         const char* title,
         void (*display_cmd)(BgrWin*),
         void (*down_cmd)(BgrWin*,int,int,int),
         void (*moved_cmd)(BgrWin*,int,int,int),
         void (*up_cmd)(BgrWin*,int,int,int),
         Uint32 wcol,
         Id id=0);
  //void move_contents_h(int delta);  // move contents horizontal
  void draw();
};

// non-editable text window
struct TextWin:WinBase {
  static const int SMAX=200;    // string length
  int linenr,
      lmax;
  Style style;
  char (*textbuf)[SMAX];
  
  TextWin(WinBase *pw,Style,Rect rect,int lmax,const char* title,Id id=0);
  void draw();
  void add_text(const char*,bool do_draw);
  void reset();
};

// radio buttons
struct RButton {
  Label label;
  void (*cmd)(struct RButWin*,int nr,int fire);
  RButton();
  RButton(int nr,Label lab);
};

struct RButWinData {
  int butnr,
      rb_max;
  RButton *but,
          *act_button;
  RButWinData();
  int next();
};

struct RButWin:WinBase {
  int y_off;
  const int b_dist; // button distance
  bool maybe_z;  // unselect with second click?
  void (*rb_cmd)(RButWin*,int nr,int fire);
  RButWinData def_buttons, // default buttons
              *d;
  Style style;
  RButWin(WinBase *parent,Style,Rect,const char *title,bool mbz,void(*cmd)(RButWin*,int nr,int fire),Id id=0);
  ~RButWin();
  void draw_rbutton(RButton *rb);
  void draw();
  void set_rbut(RButton *rb,int fire=1,bool do_draw=true); // if fire, then cmd is called
  void set_rbutnr(int nr,int fire=1,bool do_draw=true);
  int act_rbutnr();
  RButton* act_button();
  void reset();
  RButton *add_rbut(Label lab);
  RButton *is_in_rbutton(SDL_MouseButtonEvent *ev);
};

// extern radio button
struct RExtButton:WinBase {
  struct ExtRButCtrl *rxb_ctr;
  Style style;
  Label label;
  RExtButton(WinBase *pw,Style,Rect,Label,Id id=0);
  void draw();
};

struct ExtRButCtrl {
  int butnr;  // total nr of buttons
  bool maybe_z;  // can all buttons be unselected?
  Style style;
  RExtButton *act_lbut;
  void (*reb_cmd)(RExtButton*,bool is_act);
  ExtRButCtrl(Style,void (*cmd)(RExtButton*,bool is_act));
  int next();
  void set_rbut(RExtButton*,int fire=1);
  void reset();
  RExtButton *add_extrbut(WinBase *pw,Rect,Label lab,Id id);
};

// horizontal slider
struct HSlider:WinBase {
  const int sdx,
        minv,maxv;
  int def_data,  // default buffer for data
      *d;
  const char *lab_left,
             *lab_right;
  char *text;
  Style style;
  void (*cmd)(HSlider*,int fire,bool rel);
  HSlider(WinBase *parent,Style,Rect rect,int minval,int maxval,const char* t,
          void (*cmd)(HSlider*,int fire,bool rel),Id id=0);
  int &value();
  void calc_hslval(int x);
  void draw();
  void set_hsval(int val,int fire=1,bool do_draw=true);
};

// vertical slider
struct VSlider:WinBase {
  const int sdy,
        minv,maxv;
  int def_data,  // default buffer for data
      *d;
  char *text;
  Style style;
  void (*cmd)(VSlider*,int fire,bool rel);
  Rect txt_rect;
  VSlider(WinBase *parent,Style,Rect rect,int minval,int maxval,const char* t,
          void (*cmd)(VSlider*,int fire,bool rel),Id id=0);
  int &value();
  void calc_vslval(int x);
  void draw();
  void set_vsval(int val,int fire=1,bool do_draw=true);
  void hide(); // = WinBase::hide + hide text
  void show(); // = WinBase::show + draw text
};

// 2-dimensional slider
struct HVSlider:WinBase {
  const int sdx, sdy; // active area
  const Int2 minv,maxv;
  Int2 def_data,  // default buffer for data
       *d;
  char *text_x,*text_y;
  Style style;
  void (*cmd)(HVSlider*,int fire,bool rel);
  Rect txt_rect;
  HVSlider(WinBase *pw,Style,Rect rect,Int2 minval,Int2 maxval,const char* t,
           void (*cmd)(HVSlider*,int fire,bool rel),Id id=0);
  void calc_hvslval(Int2 i2);
  Int2& value();
  void draw();
  void set_hvsval(Int2,int fire=1,bool do_draw=true);
  void hide(); // like VSlider::hide()
  void show();
};

// circular slider
struct Dial:WinBase {
  const int minv,maxv;
  int def_data,  // default buffer for data
      *d;
  char *text;
  Style style;
  void (*cmd)(Dial*,int fire,bool rel);
  const Point mid;
  Rect txt_rect; // text display, w.r.t. parent
  static const int pnt_max=4,
                   sdx=60; // mouse sensitivity
  int d_start;
  const Point loc[pnt_max];
  Point aloc[pnt_max]; // actual location
  float radius[pnt_max],
        angle[pnt_max],
        ang;
  Dial(WinBase *parent,Style,Rect rect,int minval,int maxval,const char* t,
       void (*cmd)(Dial*,int fire,bool rel),Id id=0);
  int &value();
  void calc_dialval(int x,bool init);
  void draw();
  void set_dialval(int val,int fire=1,bool do_draw=true);
  void hide(); // = WinBase::hide + hide text
  void show(); // = WinBase::show + draw text
  void rotate();
  Point rotate(float r,float a);
};

// check box
struct CheckBox:WinBase {
  bool def_val,
       *d;
  Label label;
  Style style;
  void (*cmd)(CheckBox*);
  CheckBox(WinBase *pw,Style,Rect,Label lab,void (*cmd)(CheckBox*),Id id=0);
  bool &value();
  void draw();
  void set_cbval(bool,int fire=1,bool do_draw=true);
};

// horizontal scrollbar
struct HScrollbar:WinBase {
  int range, p0, xpos, wid,
      value;
  Style style;
  const int ssdim; // soft scroll area
  void (*cmd)(HScrollbar*,int val,int range);
  HScrollbar(WinBase *pw,Style,Rect,int r,void (*cmd)(HScrollbar*,int val,int range),Id id=0);
  void calc_params(int r);
  void draw();
  void set_range(int range);
  void calc_xpos(int newx);
  void set_xpos(int newx);
  void inc_value(bool incr);
  bool in_ss_area(SDL_MouseButtonEvent *ev,bool *dir);
};

// vertical scrollbar
struct VScrollbar:WinBase {
  int range, p0, ypos, height,
      value;
  Style style;
  const int ssdim; // soft scroll area
  void (*cmd)(VScrollbar*,int val,int range);
  VScrollbar(WinBase *pw,Style,Rect,int r,void (*cmd)(VScrollbar*,int val,int range),Id id=0);
  void calc_params(int r);
  void draw();
  void set_range(int);
  void calc_ypos(int newy);
  void set_ypos(int newy);
  void inc_value(bool incr);
  bool in_ss_area(SDL_MouseButtonEvent *ev,bool *dir);
};

// editable text window
struct EditWin:WinBase {
  int linenr,
      lmax,
      y_off;
  struct Line **lines;
  void (*cmd)(int ctrl_key,int key,int cmd_id);
  struct {
    int x,y;
  } cursor;
  EditWin(WinBase *pw,Rect,const char *title,void (*cmd)(int cmd_id,int ctrl_key,int key),Id id=0);
  void draw();
  void draw_line(int vpos,bool upd);
  void set_offset(int yoff);
  void set_cursor(int x,int y);
  void unset_cursor();
  void set_line(int n,bool upd,const char *form,...);     // assign lines[n]
  void insert_line(int n,bool upd,const char *form,...);  // insert a line at lines[n]
  const char *get_line(int n);
  void reset();
  void get_info(bool *active,int* nr_of_lines,int* cursor_ypos,int *nr_chars,int *total_nr_chars);
  void handle_key(SDL_keysym *key);
};

struct DialogWin:WinBase {
  Rect textr;  // text area
  int cursor,
      cmd_id;
  const char *label;
  struct Line *lin;
  void (*cmd)(const char* text,int cmd_id);
  DialogWin(WinBase *pw,Rect,Id id=0);
  void draw_line();
  void set_cursor(int pos);
  void unset_cursor();
  void draw();
  void dialog_label(const char *s,Uint32 col=0);
  void dialog_def(const char *str,void(*cmd)(const char* text,int cmdid),int cmd_id);
  bool handle_key(SDL_keysym *key);
  void dok();
};

struct Lamp {
  WinBase *pwin;
  Rect rect;
  Uint32 col;
  bool hidden;
  Lamp(WinBase *pw,Rect);
  void draw();
  void set_color(Uint32 col);
  void show();
};

struct Message {
  WinBase *pwin;
  Style style;
  const char *label;
  Point lab_pt; // label Point
  Rect mes_r;   // message Rect
  Uint32 bgcol; // background color for message
  Message(WinBase *pw,Style,const char* lab,Point top);
  void draw_label(bool upd=false);
  void draw_mes(const char *form,...);
  void draw(const char *form,...);
private:
  void draw_message(const char *form,va_list ap);
};

struct CmdMenu {
  Rect mrect;
  int nr_buttons;
  bool sticky;
  Button *src;
  RButWin *buttons;
  CmdMenu(Button *src);
  bool init(int wid,int nr_buttons,void (*menu_cmd)(RButWin*,int nr,int fire));
  RButton* add_mbut(Label lab);
  void mclose();
};

extern bool sdl_running;
extern RenderText *draw_ttf,
                  *draw_title_ttf,
                  *draw_mono_ttf,
                  *draw_blue_ttf;
extern Point alert_position;
extern const char *def_fontpath;

void send_uev(int cmd,int param1=0,int param2=0);
extern void (*handle_uev)(int cmd,int param,int param2); // user event
extern void (*handle_kev)(SDL_keysym *key,bool down);    // keyboard event
extern void (*handle_rsev)(int dw,int dh);               // resize event
void set_text(char *&txt,const char *form,...);
void say(const char *form,...);
void err(const char *form,...);
void alert(const char *form,...);
const char *id2s(int id);
void get_events();
Uint32 calc_color(Uint32 c);
SDL_Surface *create_pixmap(const char* pm_data[]);
SDL_Cursor *init_system_cursor(const char *image[]);
void file_chooser(void (*cmd)(const char* f_name,Id id),Id=0,bool setpos=false,Sint16 posx=0,Sint16 posy=0);
void working_dir(void (*cmd)(const char* dir_name,Id id)=0,Id=0,bool setpos=false,Sint16 posx=0,Sint16 posy=0);
void print_h();  // print widget hierarchy
namespace mwin {
  void down(BgrWin *bgw,int x,int y,int but);
  void move(BgrWin *bgw,int x,int y,int but);
  void up(BgrWin *bgw,int x,int y,int but);
}

#endif /* _SDL_WIDGETS_H */
