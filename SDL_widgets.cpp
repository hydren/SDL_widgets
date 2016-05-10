/*  
    SDL-widgets-1.0
    Copyright 2011-2013 W.Boeke

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.
*/

#if defined(_WIN32) && defined(__STRICT_ANSI__)
	#define off64_t _off64_t
#endif

#include <stdio.h>
#include <stdarg.h>
#ifndef _WIN32
	#include <unistd.h> // for chdir()
#endif
#include <dirent.h> // for DIR, dirent, opendir()
#include "config.h"
#include "SDL_widgets.h"
#include "sw-pixmaps.h"

#ifdef _WIN32
	#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

	#ifndef __STRICT_ANSI__
		#define strcasecmp _stricmp
	#else
		int strcasecmp( const char * str1, const char * str2)
		{
			unsigned int str1len = strlen(str1);
			unsigned int str2len = strlen(str2);
			if(str1len < str2len) return -str2[str1len];
			if(str1len > str2len) return  str1[str2len];
			unsigned int i;
			for(i = 0; i < str1len; i++)
				if (tolower(str1[i]) != tolower(str2[i]))
					return tolower(str1[i]) - tolower(str2[i]);
			return 0;
		}
		char *strdup (const char *s)
		{
			char *d = (char*) malloc (strlen (s) + 1);  // Allocate memory (Space for length plus nul)
			if (d != NULL) strcpy (d,s);        		// Copy string if okay
			return d;                            		// Return the new string
		}
	#endif
#endif

Uint32 cWhite,cBlack, cGrey, cRed, cBlue,
       cBorder, cBackground, cForeground, cPointer, cScrollbar;

const Uint32
  pixdepth=16,
  foreground= 0xcdba96ff,
  background= 0xdcd8c0ff,
  pointer1  = 0x03abffff,
  red       = 0xff0000ff,
  dark      = 0x303030ff,
  grey0     = 0x606060ff,
  grey1     = 0x707070ff,
  grey2     = 0x909090ff,
  grey3     = 0xa0a0a0ff,
  grey4     = 0xb0b0b0ff,
  grey8     = 0xe9e9e9ff,
  white     = 0xffffffff;
const float rad2deg=180/M_PI; // = 57.3

const char *def_fontpath=FONTPATH;

static int
   char_wid=0,   // -> 7
   ontop_nr=-1,  // index 
   max_ontopwin=5;

RenderText *draw_ttf,
           *draw_title_ttf,
           *draw_mono_ttf,
           *draw_blue_ttf;

void (*handle_uev)(int cmd,int param,int param2);
void (*handle_kev)(SDL_keysym *key,bool down);
void (*handle_rsev)(int dw,int dh);

static TopWin *topw;
static CmdMenu *the_menu,     // set at mousebutton up
               *the_menu_tmp; // set by CmdMenu::init()

struct Color5 {
  Uint32 c[5];
  void set_col(Uint32 fst,Uint32 lst);  // input like: 0x102030ff
};

static Uint32 cSelCmdMenu, cSlBackgr, cSlBar, cPointer1, cPointer2, cSelRBut, cButBground, cMenuBground,
              cGrey0,cGrey1,cGrey2,cGrey3,cGrey4,cGrey5,cGrey6,cGrey7,cGrey8,
              video_flag,
              go_ticks; // increased by keep_alive thread
static const int
  nominal_font_size=12,
  LBMAX=20;     // labels etc.
static Color5
  cGradientBlue, cGradientDarkBlue, cGradientRose, cGradientWheat, cGradientGreen,
  cGradientGrey, cGradientDarkGrey;
bool sdl_running; // set true by get_events()
static bool quit=false;
static TTF_Font
  *def_font,
  *title_font,
  *mono_font;
static const SDL_Color def_text_col={ 0,0,0 },
                       blue_col={ 0,0,0xf0 },
                       dark_blue_col={ 0,0,0xa0 };

static SDL_mutex *mtx=SDL_CreateMutex();
static SDL_cond *cond=SDL_CreateCond();

static struct {
  SDL_Surface *rbut_pm,
              *checkbox_pm,
              *lamp_pm,
              *cross_pm;
} pixm;

void err(const char *form,...) {
  va_list ap;
  va_start(ap,form);
  printf("Error: ");
  vprintf(form,ap);
  va_end(ap);
  putchar('\n'); fflush(stdout);
  exit(1);
}

void say(const char *form,...) {   // for debugging
  va_list ap;
  va_start(ap,form);
  printf("say: "); vprintf(form,ap); putchar('\n');
  va_end(ap);
  fflush(stdout);
}

template<class T> T* new_nulled(T* t,int len) {
  t=new T[len];
  bzero(t,sizeof(T)*len);
  return t;
}

template<class T> T* re_alloc(T* arr,int& len) { // arr only used for type inference
  T* new_arr=new T[len*2];
  bzero(new_arr+len,sizeof(T)*len);
  for (int i=0;i<len;++i) new_arr[i]=arr[i];
  delete[] arr;
  len*=2;
  return new_arr;
}

const char *id2s(int id) { // from 'abc' to "abc"
  static char buf[10];
  if (id<'0') {
    sprintf(buf,"%d",id);
    return buf;
  }
  buf[1]=(id>>24) & 0xff; buf[2]=(id>>16) & 0xff; buf[3]=(id>>8) & 0xff; buf[4]=id & 0xff; buf[5]='\''; buf[6]=0;
  int i; for (i=1;!buf[i];++i); --i;
  buf[i]='\'';
  return buf+i;
}

Point alert_position(4,4);

static int min(int a, int b) { return a<=b ? a : b; }
static int max(int a, int b) { return a>=b ? a : b; }
static int minmax(int a, int x, int b) { return x>=b ? b : x<=a ? a : x; }
static int idiv(int a,int b) { return (2 * a + b)/(b*2); }

static const Color5 *int2col5(int id,int i) {
  switch (id) {
    case 'but': {
      static const Color5 *grad_colors[5]={
        &cGradientBlue,&cGradientGrey,&cGradientGreen,&cGradientWheat,&cGradientRose
      };
      return i<5 && i>0 ? grad_colors[i] : grad_colors[0];
    }
    case 'scrl': {
      static const Color5 *grad_colors[2]={ &cGradientDarkBlue,&cGradientDarkGrey };
      return i<2 && i>0 ? grad_colors[i] : grad_colors[0];
    }
    default: return 0;
  }
}

static void cross(SDL_Surface *win,int,int) {
  if (!pixm.cross_pm) pixm.cross_pm=create_pixmap(cross_xpm);
  SDL_BlitSurface(pixm.cross_pm,0,win,rp(3,3,0,0));
}

static Rect* calc_overlap(Rect *a,Rect *b) {
  static Rect clip;
  clip.x=max(a->x,b->x);
  clip.y=max(a->y,b->y);
  int w=min(a->x+a->w,b->x+b->w)-clip.x;
  int h=min(a->y+a->h,b->y+b->h)-clip.y;
  if (w<=0 || h<=0) return 0;
  clip.w=w; clip.h=h;
  return &clip;
}

struct OnTopWin {
  WinBase *wb;         // set by keep_on_top()
  SDL_Surface *backup;
  void remove();
};

static OnTopWin *ontop_win=new OnTopWin[max_ontopwin];

void OnTopWin::remove() {
  SDL_FreeSurface(backup);
  for (int i=this-ontop_win;i<ontop_nr;++i) {
    ontop_win[i]=ontop_win[i+1];
    --ontop_win[i].wb->ontopw; // update ontopw's
  }
  --ontop_nr;
}

namespace mwin {  // to move BgrWin
  static Point mouse_point;

  static void draw_outline(BgrWin *bgr) {
    Rect *r=&bgr->tw_area;
    rectangleColor(topw->win,r->x,r->y,r->x + r->w - 1,r->y + r->h - 1,red);
  }

  void down(BgrWin* bgr,int x,int y,int but) { 
    if (but==SDL_BUTTON_LEFT) {
      if (!bgr->ontopw) { alert("widget not on-top"); return; }
      SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
      mouse_point.set(x,y);
      draw_outline(bgr);
    }
  }

  void move(BgrWin *bgr,int x,int y,int but) {
    if (abs(x-mouse_point.x)<=4 && abs(y-mouse_point.y)<=4)
      return;
    if (!bgr->ontopw) { alert("widget not on-top"); return; }
    Rect old(bgr->tw_area);
    if (!bgr->hidden) {
      SDL_BlitSurface(bgr->ontopw->backup,0,topw->win,&old);
      for (int n=bgr->ontopw-ontop_win+1;n<=ontop_nr;++n) {
        OnTopWin *otw=ontop_win+n;
        if (!otw->wb->hidden) {
          Rect *r1=&otw->wb->tw_area,
               *clip=calc_overlap(&old,r1);
          if (clip) {
            Rect c1(clip->x-r1->x,clip->y-r1->y,clip->w,clip->h);
            SDL_BlitSurface(topw->win,clip,otw->backup,&c1);
            SDL_BlitSurface(otw->wb->win,&c1,topw->win,clip);
          }
        }
      }
      SDL_UpdateRects(topw->win,1,&old);
    }
    if (bgr->move_if_ok(x - mouse_point.x,y - mouse_point.y)) {
      if (bgr->hidden)
        SDL_BlitSurface(bgr->ontopw->backup,0,topw->win,&old);
      else bgr->hidden=true;
      SDL_BlitSurface(topw->win,&bgr->tw_area,bgr->ontopw->backup,0);
      SDL_UpdateRects(topw->win,1,&old);
      draw_outline(bgr);
      SDL_UpdateRects(topw->win,1,&bgr->tw_area);
    }
  }

  void up(BgrWin *bgw,int x,int y,int but) {
    if (!bgw->ontopw) { alert("widget not on-top"); return; }
    bgw->hidden=false;
    SDL_BlitSurface(bgw->win,0,topw->win,&bgw->tw_area);
    int n=bgw->ontopw - ontop_win +1;  // 1 past own entry
    for (;n<=ontop_nr;++n) {
      OnTopWin *otw=ontop_win+n,
               *prev=otw-1;
      if (!otw->wb->hidden) {
        Rect *clip=calc_overlap(&bgw->tw_area,&otw->wb->tw_area);
        if (clip) {
          Rect r1(clip->x-otw->wb->tw_area.x,clip->y-otw->wb->tw_area.y,clip->w,clip->h),
               r2(clip->x-prev->wb->tw_area.x,clip->y-prev->wb->tw_area.y,clip->w,clip->h);
          SDL_BlitSurface(otw->backup,&r1,prev->backup,&r2);  // update backup
          SDL_BlitSurface(otw->wb->win,&r1,topw->win,clip);      // restore top window
        }
      }
    }
    SDL_UpdateRects(topw->win,1,&bgw->tw_area);
  }
}

namespace awin {
  BgrWin *bgr;
  TextWin *text;
  bool ok=false,
       alert_given=false;
  static void close_cmd(Button*) { bgr->hide(); }
  static void disp_cmd(BgrWin *bgwin) { bgwin->draw_raised(0,calc_color(0xffa0a0),true); }
  void init() {
    int wid=300,hight=150,
        w=topw->tw_area.w,
        h=topw->tw_area.h;
    Rect rect(alert_position.x,alert_position.y,wid,hight);
    if (rect.x+wid>w) rect.x=w-wid;     // will it fit?
    if (rect.y+hight>h) rect.y=h-hight;
    if (rect.x<0) { rect.x=0; rect.w=min(w,wid); }
    if (rect.y<0) { rect.y=0; rect.h=min(h,hight); }
    bgr=new BgrWin(0,rect,0,disp_cmd,mwin::down,mwin::move,mwin::up,cWhite);
    bgr->keep_on_top();
    bgr->hidden=true;
    text=new TextWin(bgr,0,Rect(3,22,rect.w-7,rect.h-26),(rect.h-26)/TDIST,0);
    new Button(bgr,Style(0,1),Rect(rect.w-20,5,14,13),cross,close_cmd);
    ok=true;
  }
  void check_alert() {
    if (alert_given) {
      bgr->hidden=false;
      SDL_BlitSurface(topw->win,&bgr->tw_area,bgr->ontopw->backup,0);
      SDL_BlitSurface(bgr->win,0,topw->win,&bgr->tw_area);
      SDL_UpdateRects(topw->win,1,&bgr->tw_area);
    }
  }
  void alert(char *buf) {
    if (sdl_running) {
      if (bgr->hidden) {
        bgr->hidden=false;
        SDL_BlitSurface(topw->win,&bgr->tw_area,bgr->ontopw->backup,0);
        text->reset();
      }
      text->add_text(buf,false);
      bgr->draw_blit_recur();
      SDL_BlitSurface(bgr->win,0,topw->win,&bgr->tw_area);
      SDL_UpdateRects(topw->win,1,&bgr->tw_area);
    }
    else {
      bgr->hidden=true;
      text->add_text(buf,false);
      alert_given=true;
    }
  }
};

void alert(const char *form,...) {
  char buf[200];
  va_list ap;
  va_start(ap,form);
  vsnprintf(buf,200,form,ap);
  va_end(ap);
  if (awin::ok)  // then awin::init() has been called
    awin::alert(buf);
  else
    puts(buf);
}

static void sdl_quit(int n) {
  SDL_Quit();
  puts("Goodbye!");
  exit(n);
}

Rect::Rect() { x=y=w=h=0; }
Rect::Rect(Sint16 _x,Sint16 _y,Uint16 _dx,Uint16 _dy) { x=_x; y=_y; w=_dx; h=_dy; }
void Rect::set(Sint16 _x,Sint16 _y,Uint16 _dx,Uint16 _dy) { x=_x; y=_y; w=_dx; h=_dy; }

Point::Point(short x1,short y1):x(x1),y(y1) { }
Point::Point():x(0),y(0) { }
void Point::set(short x1,short y1) { x=x1; y=y1; }
bool Point::operator==(Point b) { return x==b.x && y==b.y; }
bool Point::operator!=(Point b) { return x!=b.x || y!=b.y; }

void Int2::set(int x1,int y1) { x=x1; y=y1; }
Int2::Int2():x(0),y(0) { }
Int2::Int2(int x1,int y1):x(x1),y(y1) { }
bool Int2::operator!=(Int2 b) { return x!=b.x || y!=b.y; }

Id::Id(int _id1):id1(_id1),id2(0) { }
Id::Id(int _id1,int _id2):id1(_id1),id2(_id2) { }

Label::Label(const char* t):
  render_t(draw_ttf),
  draw_cmd(0),
  str(t) {
}
Label::Label(void (*dr)(SDL_Surface *win,int nr,int y_off)):
  render_t(draw_ttf),
  draw_cmd(dr),
  str(0) {
}
Label::Label(const char *t,void (*dr)(SDL_Surface *win,int nr,int y_off)):
  render_t(draw_ttf),
  draw_cmd(dr),
  str(t) {
}

void Label::draw(SDL_Surface *win,int nr,Point pnt) {
  if (str) render_t->draw_string(win,str,pnt);
  if (draw_cmd) draw_cmd(win,nr,pnt.y);
}

Style::Style(int _st):st(_st),param(0),param2(0) { }
Style::Style(int _st,int par):st(_st),param(par),param2(0) { }
Style::Style(int _st,int par,int par2):st(_st),param(par),param2(par2) { }

Uint32 calc_color(Uint32 c) {
  return SDL_MapRGB(topw->win->format,c>>16 & 0xff,c>>8 & 0xff,c & 0xff);
  //return SDL_MapRGBA(topw->win->format,c>>16 & 0xff,c>>8 & 0xff,c & 0xff,SDL_ALPHA_OPAQUE);
}

static Uint32 calc_col(Uint32 c) {
  return SDL_MapRGB(topw->win->format,c>>24 & 0xff,c>>16 & 0xff,c>>8 & 0xff);
}

void Color5::set_col(Uint32 fst,Uint32 lst) { // fst, lst e.g.: 0x102030ff, output c[i] e.g: 0x102030
  for (int i=0;i<5;++i) {
    Uint32 cc=0;
    for (int cn=0;cn<3;++cn) {  // cn = red,green,blue
      Uint8 col=((fst>>(cn+1)*8 & 0xff) * (4-i) + (lst>>(cn+1)*8 & 0xff)*i) / 4;
      cc += col<<cn*8;
    }
    c[i]=calc_color(cc);
  }
}

WinBase::WinBase(WinBase *pw,const char *t,int x,int y,int dx,int dy,Uint32 _bgcol,Id _id):
    win(0),
    title(0),
    parent(pw),
    children(0),
    ontopw(0),
    title_str(t),
    area(x,y),
    tw_area(x,y,dx,dy),
    title_area(x,y-17,0,0),
    title_top(x,y-17),
    lst_child(-1),
    end_child(5),
    bgcol(_bgcol),
    hidden(false),
    id(_id) {
  if (parent) { // parent = 0 if this = topw, or if keep_on_top() will be called
    tw_area.x=area.x+parent->tw_area.x;
    tw_area.y=area.y+parent->tw_area.y;
    parent->add_child(this);
  }
};

WinBase::~WinBase() {
  for (;lst_child>=0;--lst_child) delete children[lst_child];
  delete[] children;
  if (parent)
    parent->remove_child(this);
  if (ontopw)
    ontopw->remove();
  SDL_FreeSurface(win);
  SDL_FreeSurface(title);
}

void WinBase::init_gui() {
  if (!win) {
    win=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w,tw_area.h,pixdepth,0,0,0,0);
    if (title_str) {
      title=TTF_RenderText_Blended(title_font,title_str,dark_blue_col);
      title_area.w=title->w; title_area.h=title->h;
    }
  }
  if (ontopw && !sdl_running && !hidden) SDL_BlitSurface(topw->win,&tw_area,ontopw->backup,0); // else hide() won't work
}

void WinBase::reloc_title(int dx,int dy) {
  title_area.x=title_top.x+dx; title_area.y=title_top.y+dy;
}

void WinBase::set_title(const char* new_t,bool _upd) {
  if (title) {
    SDL_FreeSurface(title);
    parent->clear(&title_area,parent->bgcol,_upd);
    title=0;
  }
  if (new_t) {
    title=TTF_RenderText_Blended(title_font,new_t,def_text_col);
    title_area.h=title->h;
    title_area.w=title->w;
    if (_upd && !parent->hidden) {
      SDL_BlitSurface(title,0,parent->win,&title_area);
      parent->upd(&title_area);
    }
  }
}

void WinBase::add_child(WinBase *child) {
  if (!children)
    children=new WinBase*[end_child];
  else if (lst_child==end_child-1)
    children=re_alloc(children,end_child);
  if (child->ontopw) {  // then this = topw
    if (awin::ok && children[lst_child]==awin::bgr) { // awin::bgr always last
      children[lst_child]=child;
      children[++lst_child]=awin::bgr;
    }
    else
      children[++lst_child]=child;
  }
  else {
    if (this==topw && awin::ok && children[lst_child]==awin::bgr) { // awin::bgr and ontop-windows always last
      int i;
      for (i=lst_child;i>0 && children[i-1]->ontopw;--i);
      for (int j=lst_child;j>=i;--j) {
        children[j+1]=children[j];
        if (j==i) { children[j]=child; break; }
      }
      children[++lst_child]=awin::bgr;
    }
    else
      children[++lst_child]=child;
  }
}

void WinBase::remove_child(WinBase *child) {
  for (int i=lst_child;i>=0;--i)
    if (children[i]==child) {
      for (;i<lst_child;++i) children[i]=children[i+1];
      --lst_child;
      return;
    }
  alert("remove_child: child %p not found",child);
}

void WinBase::clear(Rect *rect) {
  init_gui();
  SDL_FillRect(win,rect,bgcol);
}

void WinBase::clear(Rect *rect,Uint32 col,bool _upd) {
  init_gui();
  SDL_FillRect(win,rect,col);
  if (_upd) blit_upd(rect);
}

static void move_tw_area(WinBase *wb,int delta_x,int delta_y) { // recursive
  wb->tw_area.x+=delta_x;
  wb->tw_area.y+=delta_y;
  for (int i=0;i<=wb->lst_child;++i)
    move_tw_area(wb->children[i],delta_x,delta_y);
}

bool WinBase::move_if_ok(int delta_x,int delta_y) {
  bool x_not_ok=tw_area.x+delta_x < 0 || tw_area.x+tw_area.w+delta_x > topw->tw_area.w,
       y_not_ok=tw_area.y+delta_y < 0 || tw_area.y+tw_area.h+delta_y > topw->tw_area.h;
  if (x_not_ok && y_not_ok)
    return false;
  if (x_not_ok)
    delta_x=0;
  else if (y_not_ok)
    delta_y=0;
  move_tw_area(this,delta_x,delta_y);
  area.x+=delta_x;
  area.y+=delta_y;
  return true;
}
  
void WinBase::hide() {
  if (hidden) return;
  hidden=true;
  if (!sdl_running) return;
  WinBase *wb,*nxt;
  Rect r2(0,0,tw_area.w,tw_area.h),
       tr2(tit_os().x,tit_os().y,title_area.w,title_area.h);
  for (wb=this,nxt=parent;nxt;wb=nxt,nxt=nxt->parent) {
    r2.x+=wb->area.x; r2.y+=wb->area.y;    // win
    nxt->clear(&r2,parent->bgcol,false);
    if (title) {
      tr2.x+=wb->area.x; tr2.y+=wb->area.y;  // title
      nxt->clear(&tr2,parent->bgcol,false);
    }
    if (nxt->hidden) return;
    if (nxt==topw) {
      if (ontopw) {
        SDL_BlitSurface(ontopw->backup,0,topw->win,&r2); // restore the screen
      }
      wb->upd(&r2);
      if (title) wb->upd(&tr2);
      return;
    }
  }
}

Point WinBase::tit_os() { return Point(title_area.x-area.x,title_area.y-area.y); }

void WinBase::show() {
  if (!hidden) return;
  hidden=false;
  if (!sdl_running) return;
  // blit_upd() not useable: does'nt blit the title
  WinBase *wb,*nxt;
  Rect r1(0,0,tw_area.w,tw_area.h),
       tr1(tit_os().x,tit_os().y,title_area.w,title_area.h);
  for (wb=this,nxt=parent;nxt;wb=nxt,nxt=nxt->parent) {
    if (wb->hidden) return;
    r1.x+=wb->area.x; r1.y+=wb->area.y;    // win
    SDL_BlitSurface(win,0,nxt->win,&r1);
    if (title) {
      tr1.x+=wb->area.x; tr1.y+=wb->area.y;  // title
      SDL_BlitSurface(title,0,nxt->win,&tr1);
    }
    if (nxt==topw) {
      wb->upd(&r1);
      if (title) wb->upd(&tr1);
      return;
    }
  }
}

void WinBase::draw_raised(Rect *rect,Uint32 col,bool up) {
  SDL_FillRect(win,rect,col);
  if (!rect) rect=rp(0,0,tw_area.w,tw_area.h);
  if (up) {
    hlineColor(win,rect->x,rect->x+rect->w-1,rect->y,white); // upper
    vlineColor(win,rect->x,rect->y,rect->y+rect->h-1,white);  // left
    hlineColor(win,rect->x,rect->x+rect->w-1,rect->y+rect->h-1,grey0); // bottom
    vlineColor(win,rect->x+rect->w-1,rect->y,rect->y+rect->h-1,grey0); // right
  }
  else {
    hlineColor(win,rect->x,rect->x+rect->w-1,rect->y,grey0);  // upper
    hlineColor(win,rect->x,rect->x+rect->w-1,rect->y+rect->h-1,white); // bottom
    vlineColor(win,rect->x,rect->y,rect->y+rect->h-1,grey0);  // left
    vlineColor(win,rect->x+rect->w-1,rect->y,rect->y+rect->h-1,white); // right
  }
}

void WinBase::draw_gradient(Rect rect,const Color5 *col,bool vertical,bool hollow) {
  const int x[6]={ 0,rect.w/5,rect.w*2/5,rect.w*3/5,rect.w*4/5,rect.w },
            y[6]={ 0,rect.h/5,rect.h*2/5,rect.h*3/5,rect.h*4/5,rect.h };
  for (int i=0;i<5;++i) {
    Uint32 c= hollow ? col->c[4-i] : col->c[i];
    if (vertical)
      SDL_FillRect(win,rp(rect.x+x[i],rect.y,x[i+1]-x[i],rect.h),c);
    else
      SDL_FillRect(win,rp(rect.x,rect.y+y[i],rect.w,y[i+1]-y[i]),c);
  }
  rectangleColor(win,rect.x,rect.y,rect.x+rect.w-1,rect.y+rect.h-1,grey1);
}

void set_text(char *&txt,const char *form,...) {
  if (!txt) txt=new char[LBMAX];
  va_list ap;
  va_start(ap,form);
  vsnprintf(txt,LBMAX,form,ap);
  va_end(ap);
}

SDL_Surface *create_pixmap(const char* pm_data[]) {
  int i,dx,dy,nr_col;
  char ch;
  sscanf(pm_data[0],"%d %d %d",&dx,&dy,&nr_col);
  SDL_Surface *pm=SDL_CreateRGBSurface(SDL_SWSURFACE,dx,dy,pixdepth,0,0,0,0);
  SDL_SetColorKey(pm,SDL_SRCCOLORKEY,0);
  //printf("dx=%d dy=%d nr_col=%d\n",dx,dy,nr_col);
  SDL_FillRect(pm,0,0); // default = None
  struct ColSym {
    char ch;
    Uint32 col;
    bool none;
  };
  ColSym *colsym=new ColSym[nr_col];
  for (i=0;i<nr_col;++i) {
    Uint32 col;
    bool none=false;
    char s[10];
    sscanf(pm_data[i+1],"%c c %c%6s",&colsym[i].ch,&ch,s); // expected "* c #606060" or "* c None"
    if (ch=='#')
      sscanf(s,"%x",&col);
    else if (ch=='N' && !strcmp(s,"one")) {
      col=0; none=true;
    }
    else {
      printf("unexpected char '%c' in xpm\n",ch); col=0;
    }
    if ((col & 0xff) < 8 && (col>>8 & 0xff) < 8 && (col>>16 & 0xff) < 8)  // to prevent transparent color
      col |= 8;
    colsym[i].col= col<<8 | 0xff;
    colsym[i].none=none;
  }
  for (int col=0;col<nr_col;++col) {
    ch=colsym[col].ch;
    Uint32 pixcol=colsym[col].col;
    bool none=colsym[col].none;
    for (int y=0;y<dy;++y)
      for (int x=0;x<dx;++x)
        if (pm_data[y+nr_col+1][x]==ch && !none)
          pixelColor(pm,x,y,pixcol);
  }
  delete[] colsym;
  return pm;
}

SDL_Cursor *init_system_cursor(const char *image[]) {
  int i, row, col,
      dx,dy,nr_color;
  Uint8 data[4*32];
  Uint8 mask[4*32];
  int hot_x=-1, hot_y=-1;
  sscanf(image[0],"%d %d %d",&dx,&dy,&nr_color); // expected: "20 20 3 1"
  if (nr_color!=3) {
    alert("%d colors in cursor data, expected 3",nr_color);
    return 0;
  }
  if (dx>32 || dy>32) {
    alert("dx=%d, dy=%d in cursor data, must be <= 32",dx,dy);
    return 0;
  }
  i = -1;
  for ( row=0; row<32; ++row ) {
    for ( col=0; col<32; ++col ) {
      if ( col % 8 ) {
        data[i] <<= 1;
        mask[i] <<= 1;
      } else {
        ++i;
        data[i] = mask[i] = 0;
      }
      char ch= col<dx && row<dy ? image[4+row][col] : ' ';
      switch (ch) {
        case 'X':
          data[i] |= 0x01;
          mask[i] |= 0x01;
          break;
        case '.':
          mask[i] |= 0x01;
          break;
        case ' ':
          break;
        default:
          alert("char '%c' in cursor data, expected 'X','.' or ' '",ch);
          return 0;
      }
    }
  }
  if (2!=sscanf(image[4+dy], "%d,%d", &hot_x, &hot_y) ||
      hot_x<0 || hot_y<0) {
    alert("hot-x and hot-y missing in cursor data");
    return 0;
  }
  return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}

TopWin::TopWin(const char* wm_title,Rect rect,Uint32 init_flag,Uint32 vflag,void (*draw_cmd)(),void (*set_icon)(),void (*on_sdl_init)()):
    WinBase(0,0,0,0,rect.w,rect.h,cBackground,0),
    display_cmd(draw_cmd) {
  if (SDL_Init(init_flag) < 0) err("SDL init problem");
  if(on_sdl_init!=0) on_sdl_init();
  if (TTF_Init() < 0) err("SDL ttf init problem");
  if (!(title_font=TTF_OpenFont(FONTPATH,nominal_font_size+1)))  // fontpath from config.h
    err("font-spec %s not found",FONTPATH);
  if (!(def_font=TTF_OpenFont(FONTPATH,nominal_font_size)))
    err("font-spec %s not found",FONTPATH);
  if (!(mono_font=TTF_OpenFont(FONTPATH_MONO,nominal_font_size)))
    err("font-spec %s not found",FONTPATH_MONO);
  TTF_SizeText(mono_font,"1",&char_wid,0); // minimum mono char width
  video_flag=vflag ? vflag : SDL_SWSURFACE;
  if (set_icon) set_icon();
  const char* cur_wdir=getcwd(0,0);
  if (wm_title) SDL_WM_SetCaption(wm_title,wm_title);
  else SDL_WM_SetCaption(cur_wdir,0);
  free((void*)cur_wdir);
  win=SDL_SetVideoMode(rect.w,rect.h,pixdepth,video_flag);
  SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  draw_ttf=new RenderText(def_font,def_text_col);
  draw_blue_ttf=new RenderText(def_font,blue_col);
  draw_title_ttf=new RenderText(def_font,dark_blue_col);
  draw_mono_ttf=new RenderText(mono_font,def_text_col);
  SDL_SetAlpha(win, 0,SDL_ALPHA_OPAQUE);
  topw=this; // needed for calc_color()

  cWhite       = calc_color(white);
  cBlack       = calc_color(0);
  cGrey0       = calc_col(grey0),
  cGrey1       = calc_color(0x808080),
  cGrey2       = calc_col(grey2),
  cGrey3       = calc_col(grey3),
  cGrey4       = calc_col(grey4),
  cGrey5       = calc_color(0xc0c0c0),
  cGrey6       = calc_color(0xd0d0d0),
  cGrey7       = calc_color(0xe0e0e0),
  cGrey8       = calc_col(grey8);
  cBorder      = cGrey0;
  cSlBackgr    = cGrey4; // slider and scrollbar background
  cSlBar       = cGrey2; // slider value bar
  cGrey        = cGrey5;
  cMenuBground = cGrey7;
  cButBground  = cGrey7;
  cBlue        = calc_color(0xff);
  cRed         = calc_col(red);
  cForeground  = calc_col(foreground);
  cBackground  = calc_col(background);
  cSelRBut     = calc_color(0xe7e760); // selected radiobutton
  cSelCmdMenu  = calc_color(0xb0b0ff); // selected cmd menu item
  cPointer     = calc_color(0xff8080);
  cPointer1    = calc_col(pointer1);
  cPointer2    = cPointer1;
  cGradientBlue.set_col    (0xf0ffffff,0xa0d0e0ff);
  cGradientDarkBlue.set_col(0xa0f0ffff,0x3094d0ff);
  cGradientWheat.set_col   (0xfff0d7ff,0xc0c0a0ff);
  cGradientGrey.set_col    (0xffffffff,0xc7c7c7ff);
  cGradientDarkGrey.set_col(0xf0f0f0ff,0x707070ff);
  cGradientGreen.set_col   (0xe0ffe0ff,0x77c077ff);
  cGradientRose.set_col    (0xffd0d0ff,0xd07070ff);
  bgcol=cBackground;
  awin::init();
  awin::bgr->hidden=true;
  SDL_Cursor *curs=init_system_cursor(arrow);
  if (curs) SDL_SetCursor(curs);
}

void TopWin::draw() { if (display_cmd) display_cmd(); }

BgrWin::BgrWin(WinBase *pw,
               Rect rect,
               const char* tit,
               void (*dis_cmd)(BgrWin*),
               void (*do_cmd)(BgrWin*,int,int,int),
               void (*mo_cmd)(BgrWin*,int,int,int),
               void (*u_cmd)(BgrWin*,int,int,int),
               Uint32 _bgcol,
               Id _id):
  WinBase(pw,tit,rect.x,rect.y,rect.w,rect.h,_bgcol,_id),
          display_cmd(dis_cmd),
          down_cmd(do_cmd),
          moved_cmd(mo_cmd),
          up_cmd(u_cmd) {
}

void BgrWin::draw() {
  init_gui();
  if (display_cmd) display_cmd(this);
}

HSlider::HSlider(WinBase *pw,Style st,Rect rect,int minval,int maxval,
        const char* tit,void (*_cmd)(HSlider*,int fire,bool rel),Id _id):
    WinBase(pw,tit,rect.x,rect.y,rect.w,9+TDIST,0,_id),
    sdx(rect.w-8),
    minv(minval),maxv(max(minval+1,maxval)),
    def_data(minval),
    d(&def_data),
    lab_left(0),
    lab_right(0),
    text(0),
    style(st),
    cmd(_cmd) {
}

static void draw_text(Rect *txt_rect,const char *text,WinBase *parent) {
  if (txt_rect->w) parent->clear(txt_rect,parent->bgcol,true); 
  txt_rect->w=draw_ttf->text_width(text);
  draw_ttf->draw_string(parent->win,text,Point(txt_rect->x,txt_rect->y));
  parent->blit_upd(txt_rect);
}

int &HSlider::value() { return *d; }

void HSlider::draw() {
  init_gui();
  SDL_FillRect(win,0,parent->bgcol);
  int range=maxv-minv,
      xpos;
  draw_raised(rp(0,0,tw_area.w,12),cSlBackgr,false); 
  switch (style.st) {
    case 0:
      xpos=(*d - minv) * sdx / range;
      SDL_FillRect(win,rp(1,1,xpos+4,10),cSlBar);
      if (style.param==0) 
        for (int i=*d+1;i<=maxv;++i) {
          vlineColor(win,3 + sdx * (i-minv) / range,3,7,grey8);
        }
      draw_raised(rp(xpos+1,1,6,10),cPointer2,true);
      break;
    case 1:
      xpos=(*d - minv) * sdx / range;
      if (style.param==0) 
        for (int i=minv;i<=maxv;++i)
          vlineColor(win,3 + sdx * (i-minv) / range,3,7,grey8);
      draw_raised(rp(xpos+1,1,6,10),cPointer1,true);
      break;
  }
  if (text)
    draw_ttf->draw_string(win,text,Point((tw_area.w-draw_ttf->text_width(text))/2,tw_area.h-14));
  if (lab_left)
    draw_ttf->draw_string(win,lab_left,Point(1,tw_area.h-14));
  if (lab_right) {
    int x1=tw_area.w - draw_ttf->text_width(lab_right) - 1;
    draw_ttf->draw_string(win,lab_right,Point(x1,tw_area.h-14));
  }
}

void HSlider::set_hsval(int val,int fire,bool do_draw) {
  *d=val;
  if (fire && cmd) cmd(this,fire,false);
  if (do_draw)
    draw_blit_upd();
}

void HSlider::calc_hslval(int x1) {
  *d=minmax(minv,minv + idiv((x1-4) * (maxv - minv),sdx),maxv);
}

VSlider::VSlider(WinBase *pw,Style st,Rect rect,int minval,int maxval,
        const char* tit,void (*_cmd)(VSlider*,int fire,bool rel),Id _id):
    WinBase(pw,tit,rect.x,rect.y,10,rect.h,0,_id),
    sdy(rect.h-8),
    minv(minval),maxv(max(minval+1,maxval)),
    def_data(minval),
    d(&def_data),
    text(0),
    style(st),
    cmd(_cmd),
    txt_rect(rect.x+14,rect.y+tw_area.h/2-5,0,TDIST) {
}

int &VSlider::value() { return *d; }

void VSlider::draw() {
  init_gui();
  SDL_FillRect(win,0,parent->bgcol);
  int range=maxv-minv,
      ypos=sdy - (*d - minv) * sdy / range;
  draw_raised(rp(0,0,12,tw_area.h),cSlBackgr,false);
  switch (style.st) {
    case 0:
      SDL_FillRect(win,rp(1,ypos+4,9,tw_area.h-ypos-4),cSlBar);
      if (style.param==0)
        for (int i=*d+1;i<=maxv;++i)
          hlineColor(win,2,7,3 + sdy - sdy * (i-minv) / range,grey8);
      draw_raised(rp(1,ypos+1,10,6),cPointer2,true);
      break;
    case 1:
      if (style.param==0)
        for (int i=minv;i<=maxv;++i) {
          hlineColor(win,2,7,3 + sdy - sdy * (i-minv) / range,grey8);
        }
      draw_raised(rp(1,ypos+1,10,6),cPointer1,true);
      break;
  }
  if (!hidden && text)
    draw_text(&txt_rect,text,parent);
}

void VSlider::set_vsval(int val,int fire,bool do_draw) {
  *d=val;
  if (fire && cmd) cmd(this,fire,false);
  if (do_draw)
    draw_blit_upd();
}

void VSlider::calc_vslval(int y1) {
  *d=minmax(minv,minv + idiv((sdy+4-y1) * (maxv - minv),sdy),maxv);
}

HVSlider::HVSlider(WinBase *pw,Style st,Rect rect,Int2 minval,Int2 maxval,
                   const char* tit,void (*_cmd)(HVSlider*,int fire,bool rel),Id _id):
    WinBase(pw,tit,rect.x,rect.y,rect.w,rect.h,0,_id),
    sdx(rect.w-10),sdy(rect.h-10-TDIST),
    minv(minval),maxv(maxval),
    def_data(minval),
    d(&def_data),
    text_x(0),text_y(0),
    style(st),
    cmd(_cmd),
    txt_rect(rect.x+tw_area.w+2,rect.y+tw_area.h/2-10,0,TDIST)  {
}

Int2 &HVSlider::value() { return *d; }

void HVSlider::draw() {
  init_gui();
  SDL_FillRect(win,0,parent->bgcol);
  Int2 range(maxv.x - minv.x,maxv.y - minv.y),
       pos((d->x - minv.x) * sdx / range.x,
           sdy - (d->y - minv.y) * sdy / range.y);
  int i;
  const int dx=tw_area.w,
            dy=tw_area.h-TDIST;
  draw_raised(rp(0,0,dx,dy),cSlBackgr,false);
  switch (style.st) {
    case 0:
      SDL_FillRect(win,rp(1,pos.y+1,pos.x+8,dy-pos.y-2),cSlBar);
      if (style.param==0) {
        for (i=d->x+1;i<=maxv.x;++i) {
          int x1=4 + sdx * (i-minv.x) / range.x;
          vlineColor(win,x1,dy-6,dy-2,grey8);
        }
        for (i=d->y+1;i<=maxv.y;++i) {
          int y1=4 + sdy - sdy * (i-minv.y) / range.y;
          if (y1<pos.y) hlineColor(win,1,5,y1,grey8);
        }
      }
      draw_raised(rp(pos.x+1,pos.y+1,8,8),cPointer2,true);
      break;
    case 1:
      if (style.param==0) {
        for (i=minv.x;i<=maxv.x;++i) {
          int x1=4 + sdx * (i-minv.x) / range.x;
          vlineColor(win,x1,dy-6,dy-2,grey8);
        }
        for (i=minv.y;i<=maxv.y;++i) {
          int y1=4 + sdy - sdy * (i-minv.y) / range.y;
          hlineColor(win,1,5,y1,grey8);
        }
      }
      draw_raised(rp(pos.x+1,pos.y+1,8,8),cPointer1,true);
      break;
  }

  if (text_x) {
    int x1=max(0,8+(sdx-draw_ttf->text_width(text_x))/2);
    draw_ttf->draw_string(win,text_x,Point(x1,tw_area.h-14));
  }
  if (!hidden && text_y)
    draw_text(&txt_rect,text_y,parent);
}

void HVSlider::set_hvsval(Int2 val,int fire,bool do_draw) {
  *d=val;
  if (fire && cmd) cmd(this,fire,false);
  if (do_draw)
    draw_blit_upd();
}

void HVSlider::calc_hvslval(Int2 i2) {
  Int2 range(maxv.x - minv.x,maxv.y - minv.y),
       val1(minv.x + idiv((i2.x - 5) * range.x,sdx),
            minv.y + idiv((sdy + 5 - i2.y) * range.y,sdy));
  d->set(minmax(minv.x,val1.x,maxv.x),minmax(minv.y,val1.y,maxv.y));
}

void VSlider::hide() {
  WinBase::hide();
  if (txt_rect.w) { 
    parent->clear(&txt_rect);
    parent->blit_upd(&txt_rect);
  }
}

void VSlider::show() {
  WinBase::show();
  if (txt_rect.w) { 
    draw_ttf->draw_string(parent->win,text,Point(txt_rect.x,txt_rect.y));
    parent->blit_upd(&txt_rect);
  }
}

void HVSlider::hide() {
  WinBase::hide();
  if (txt_rect.w) { 
    parent->clear(&txt_rect);
    parent->blit_upd(&txt_rect);
  }
}

void HVSlider::show() {
  WinBase::show();
  if (txt_rect.w) { 
    draw_ttf->draw_string(parent->win,text_y,Point(txt_rect.x,txt_rect.y));
    parent->blit_upd(&txt_rect);
  }
}

Dial::Dial(WinBase *pw,Style st,Rect rect,int minval,int maxval,
        const char* tit,void (*_cmd)(Dial*,int fire,bool rel),Id _id):
    WinBase(pw,tit,rect.x,rect.y,rect.w,rect.w,0,_id),
    minv(minval),maxv(max(minval+1,maxval)),
    def_data(minval),
    d(&def_data),
    text(0),
    style(st),
    cmd(_cmd),
    mid(rect.w/2,rect.w/2),
    txt_rect(rect.x+rect.w+3, rect.y+mid.y, 0, 15),
    d_start(0),
    loc((Point[]){ Point(-2,4), Point(2,4), Point(2,mid.y-1), Point(-2,mid.y-1) }), // pointer
    ang(0) {
  for (int i=0;i<pnt_max;++i) {
    float x=loc[i].x,
          y=loc[i].y;
    radius[i]=hypotf(x,y);
    angle[i]=atan2f(x,y);
  }
  title_area.x=rect.x+rect.w+3;
  title_area.y=rect.y+mid.y-TDIST-2;
}

void Dial::rotate() {
  for (int i=0;i<pnt_max;++i) {
    float a=angle[i]+ang;
    aloc[i].x=lrint(cosf(a) * radius[i] + mid.x);
    aloc[i].y=lrint(sinf(a) * radius[i] + mid.y);
  }
}
Point Dial::rotate(float r,float a) { // one point
  return Point(lrint(cosf(a) * r + mid.x),
               lrint(sinf(a) * r + mid.y));
}

int &Dial::value() { return *d; }

static void circle3d(SDL_Surface *win,Point mid,int radius,bool up) {
  int start=110;
  Uint32 col[4]= { grey1,
                   up ? white : dark,
                   grey1,
                   up ? dark : white };
  for (int i=0;i<4;++i) {
    arcColor(win,mid.x,mid.y,radius,start,start+90,col[i]);
    start+=90;
  }
}

void Dial::draw() {
  init_gui();
  SDL_FillRect(win,0,parent->bgcol);
  ang=(120+(*d - minv) * (360-60)/(maxv - minv)) / rad2deg;
  rotate();
  const Sint16 x[pnt_max]={ aloc[0].x, aloc[1].x, aloc[2].x, aloc[3].x },
               y[pnt_max]={ aloc[0].y, aloc[1].y, aloc[2].y, aloc[3].y };
  int end=lrint(ang*rad2deg);
  const int hwid=tw_area.w/2;

  filledCircleColor(win,mid.x,mid.y,hwid,grey4);  // background
  filledPieColor(win,mid.x,mid.y,hwid,120,end,grey2);

  filledPolygonColor(win,x,y,pnt_max,0x505050ff); // pointer

  filledCircleColor(win,mid.x,mid.y,4,pointer1);  // centre
  circle3d(win,mid,4,true);

  float step=300./rad2deg/(maxv-minv);
  for (int i=1;i<=maxv - *d;++i) {
    float a=ang+step*i,
          r=radius[2];
    Point pt0=rotate(r-1,a),
          pt1=rotate(r-3,a);
    lineColor(win,pt0.x,pt0.y,pt1.x,pt1.y,white);  // small scale-lines
  }
  circle3d(win,mid,hwid-1,false); // border

  if (text) draw_text(&txt_rect,text,parent);
}

void Dial::set_dialval(int val,int fire,bool do_draw) {
  *d=val;
  if (fire && cmd) cmd(this,fire,false);
  if (do_draw)
    draw_blit_upd();
}

void Dial::calc_dialval(int x1,bool init) {  // x1 = distance from area.x, sdx = 30
  int val=minv + idiv(x1 * (maxv - minv),sdx);
  if (init) d_start= *d - val;
  else *d=minmax(minv,d_start+val,maxv);
}

void Dial::hide() {
  WinBase::hide();
  if (txt_rect.w) { 
    parent->clear(&txt_rect);
    parent->blit_upd(&txt_rect);
  }
}

void Dial::show() {
  WinBase::show();
  if (txt_rect.w) { 
    draw_ttf->draw_string(parent->win,text,Point(txt_rect.x,txt_rect.y));
    parent->blit_upd(&txt_rect);
  }
}

CheckBox::CheckBox(WinBase *pw,Style st,Rect rect,Label lab,void (*_cmd)(CheckBox*),Id _id):
    WinBase(pw,0,rect.x,rect.y,rect.w ? rect.w : 14,rect.h ? rect.h : 14,0,_id),
    def_val(false),
    d(&def_val),
    label(lab),
    style(st),
    cmd(_cmd) {
}

bool &CheckBox::value() { return *d; }

void CheckBox::draw() {
  if (!win) {
    switch (style.st) {
      case 0:
      case 1:
        win=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w,tw_area.h,pixdepth,0,0,0,0);
        if (label.str) { // label used as title
          title=TTF_RenderText_Blended(def_font,label.str,def_text_col);
          title_area.set(area.x+tw_area.w+2,area.y,title->w,title->h);
          label.str=0;
        }
        break;
    }
  }
  Rect os;
  switch (style.st) {
    case 0:  // square 3D
      SDL_FillRect(win,0,parent->bgcol);
      draw_raised(rp(0,1,tw_area.h-2,tw_area.h-2),cGrey7,false);
      if (!pixm.checkbox_pm) pixm.checkbox_pm=create_pixmap(check_xpm);
      if (*d) SDL_BlitSurface(pixm.checkbox_pm,0,win,rp(1,3,0,0));
      label.draw(win,id.id2,Point(tw_area.h+2,tw_area.h/2-8));
      break;
    case 1:  // round 3D
      SDL_FillRect(win,0,parent->bgcol);
      if (!pixm.rbut_pm) pixm.rbut_pm=create_pixmap(rbut_pm);
      SDL_BlitSurface(pixm.rbut_pm,0,win,rp(0,1,0,0));
      if (*d)
        filledCircleColor(win,6,7,2,0xff);
      label.draw(win,id.id2,Point(tw_area.h+2,tw_area.h/2-8));
      break;
  }
}

void CheckBox::set_cbval(bool val,int fire,bool do_draw) {
  *d=val;
  if (do_draw) draw_blit_upd();
  if (fire && cmd) cmd(this);
}

Button::Button(WinBase *pw,Style st,Rect rect,Label lab,void (*_cmd)(Button*),Id _id):
    WinBase(pw,0,rect.x,rect.y,rect.w?rect.w:16,rect.h?rect.h:16,0,_id),
    is_down(false),
    style(st),
    label(lab),
    cmd(_cmd) {
}

void Button::draw() {
  if (!win) {
    switch (style.st) {
      case 0:  // normal
        win=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w,tw_area.h,pixdepth,0,0,0,0);
        break;
      case 1:  // square, label used as title at rightside
        win=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.h,tw_area.h,pixdepth,0,0,0,0);
        if (label.str) {
          title=TTF_RenderText_Blended(def_font,label.str,def_text_col);
          title_area.set(area.x+tw_area.w+2,area.y+1,title->w,title->h);
          label.str=0;
        }
        break;
      case 2:  // for menu
      case 3:  // 2D, no redraw if is_down
        win=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w,tw_area.h,pixdepth,0,0,0,0);
        bgcol=parent->bgcol;
        break;
      case 4:  // for menu, with variable label
        win=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w,tw_area.h,pixdepth,0,0,0,0);
        bgcol=cWhite;
        break;
    }
  }
  const Color5 *bcol=int2col5('but',style.param);
  int y=tw_area.h/2-8;
  switch (style.st) {
    case 0:
      if (is_down)
        draw_raised(0,bcol->c[2],false);
      else
        draw_gradient(Rect(0,0,tw_area.w,tw_area.h),bcol);
      label.draw(win,id.id2,Point(3,y));
      break;
    case 1:
      if (is_down)
        draw_raised(rp(0,0,tw_area.h,tw_area.h),bcol->c[2],false);
      else
        draw_gradient(Rect(0,0,tw_area.h,tw_area.h),bcol);
      label.draw(win,id.id2,Point(3,y));
      break;
    case 2: // for menu
      if (is_down) break;
      clear();
      label.draw(win,id.id2,Point(3,y));
      if (style.param==1)
        rectangleColor(win,0,0,tw_area.w-1,tw_area.h-1,grey0);
      break;
    case 3: // 2D, no drawing if is_down
      if (is_down) break;
      clear();
      label.draw(win,id.id2,Point(3,y));
      rectangleColor(win,0,0,tw_area.w-1,tw_area.h-1,grey0);
      break;
    case 4: // for menu, with variable label
      if (is_down) break;
      bgcol=cWhite; // restore bgcol
      clear();
      label.draw(win,id.id2,Point(3,y));
      SDL_FillRect(win,rp(tw_area.w-12,0,12,tw_area.h),parent->bgcol);
      rectangleColor(win,0,0,tw_area.w-1,tw_area.h-1,grey0);
      vlineColor(win,tw_area.w-13,0,tw_area.h-1,grey0);
      filledTrigonColor(win,tw_area.w-10,y+5,tw_area.w-4,y+5,tw_area.w-7,y+10,0xff);
      break;
  }
}

RenderText::RenderText(TTF_Font *font,SDL_Color tcol):
    ttf_font(font),
    text_col(tcol) {
  for (int i=0;i<dim;++i) { chars[i]=0; ch_wid[i]=0; }
}

int RenderText::draw_string(SDL_Surface *win,const char *s,Point pt) {
  Rect offset(pt.x,pt.y,0,0);
  int ind;
  for (int i=0;s[i] && offset.x < win->w;++i) {
    if (s[i]=='\n') {
      offset.y+=TDIST; offset.x=pt.x;
      continue;
    }
    if (s[i]<' ') {
      alert("draw_string: unexpected char (nr: %d)",s[i]);
      return 0;
    }
    ind=s[i]-' ';
    if (!chars[ind]) set_char(ind);
    Rect off(offset);
    SDL_BlitSurface(chars[ind],0,win,&off); // off.w end off.h might be modified!
    offset.x+=ch_wid[ind];
  }
  return offset.y + TDIST;
}

void RenderText::set_char(const int ind) {
  static char buf[2];
  buf[0]=ind+' ';
  chars[ind]=TTF_RenderText_Blended(ttf_font,buf,text_col);
  ch_wid[ind]=chars[ind]->w;
}

int RenderText::text_width(const char *s) {
  int wid=0;
  if (!s) return char_wid;
  for (int i=0;s[i];++i) {
    if (s[i]<' ') {
      alert("text_width: bad char in string %s",s);
      return 0;
    }
    const int ind=s[i]-' ';
    if (!chars[ind]) set_char(ind);
    wid+=ch_wid[ind];
  }
  return wid;
}

struct UevQueue {  // user-event queue
  int queue_len,
    (*buffer)[3],
    ptr1,
    ptr2;
  UevQueue():
      queue_len(50),
      buffer(new int[queue_len][3]),
      ptr1(0),
      ptr2(0) {
  }
  void push(int param0,int param1,int param2) {
    int n=(ptr2+1)%queue_len;
    if (n==ptr1) {
      printf("push: inc queue-buffer len to: %d\n",2*queue_len);
      int (*prev)[3]=buffer,
          i1,i2,i3;
      buffer=new int[2*queue_len][3];
      for (i1=ptr1,i2=0;i1!=ptr2;i1=(i1+1)%queue_len,++i2)
        for (i3=0;i3<3;++i3)
          buffer[i2][i3]=prev[i1][i3];
      ptr1=0;
      n=queue_len;
      ptr2=n-1;
      queue_len*=2;
      delete[] prev;
    }
    buffer[ptr2][0]=param0;
    buffer[ptr2][1]=param1;
    buffer[ptr2][2]=param2;
    ptr2=n;
  }
  bool is_empty() { return ptr1==ptr2; }
  void pop(int *dat) {
    for (int i=0;i<3;++i)
      dat[i]=buffer[ptr1][i];
    ptr1=(ptr1+1)%queue_len;
  }
} uev_queue;

struct Repeat {
  bool on,
       direction;
  SDL_MouseButtonEvent ev;
} repeat;

void send_uev(int cmd,int param1,int param2) {
  SDL_mutexP(mtx);
  uev_queue.push(cmd,param1,param2);
  SDL_CondSignal(cond);
  SDL_mutexV(mtx);
}

WinBase *WinBase::in_a_win(int x,int y) {
  if (hidden) return 0;
  int x1=tw_area.x, y1=tw_area.y;
  if (x >= x1 && x < x1+tw_area.w && y >= y1 && y < y1+tw_area.h) {
    for (int i=lst_child;i>=0;--i) {      // last child first
      WinBase *wb=children[i]->in_a_win(x,y);
      if (wb)
        return wb;
    }
    return this;
  }
  return 0;
}

void WinBase::draw_blit_recur() {  // from this downward, also the title is blitted
  draw();
  for (int i=0;i<=lst_child;++i) {
    WinBase *child=children[i];
    if (!child->hidden || !child->win) {
      child->draw_blit_recur();
      if (!child->hidden && child->title)
        SDL_BlitSurface(child->title,0,win,&child->title_area);
    }
  }
  if (parent && !hidden) {
    SDL_BlitSurface(win,0,parent->win,rp(area.x,area.y,0,0));
  }
}

void WinBase::upd(Rect *rect) { // rect w.r.t. topw, win blitted to topw
  if (!rect) rect=&tw_area;
  int n=0;
  if (this!=topw) {
    if (parent!=topw) { alert("upd: parent must be the topwindow %p %p",topw,parent); return; }
    if (ontopw) {
      n=ontopw - ontop_win +1;  // 1 past own entry
    }
  }
  for (;n<=ontop_nr;++n) {
    OnTopWin *otw=ontop_win+n;
    WinBase *wb=otw->wb;
    if (!wb->hidden) {
      Rect *r1=&wb->tw_area,
           *clip=calc_overlap(rect,r1);
      if (clip) {
        Rect c1(clip->x-r1->x,clip->y-r1->y,clip->w,clip->h);
        SDL_BlitSurface(topw->win,clip,otw->backup,&c1);  // update backup
        SDL_BlitSurface(wb->win,&c1,topw->win,clip);      // restore top window
      }
      if (wb->title) {
        Rect tc1(r1->x+wb->tit_os().x,r1->y+wb->tit_os().y,wb->title_area.w,wb->title_area.h);
        clip=calc_overlap(&tw_area,&tc1);
        if (clip)
          SDL_BlitSurface(wb->title,0,topw->win,&tc1);
      }
    }
  }
  SDL_UpdateRects(topw->win,1,rect);
}

RButWin::RButWin(WinBase *pw,Style st,Rect rect,const char *tit,bool mbz,void(*cmd)(RButWin*,int nr,int fire),Id _id):
  WinBase(pw,tit,rect.x,rect.y,rect.w,rect.h,cWhite,_id),
    y_off(0),
    b_dist(st.param2 ? st.param2 : TDIST),
    maybe_z(mbz),
    rb_cmd(cmd),
    d(&def_buttons),
    style(st) {
}

RButWin::~RButWin() {
  delete[] d->but; d->but=0;
}

RButWinData::RButWinData():
    butnr(-1),
    rb_max(20),
    but(new RButton[rb_max]),
    act_button(0) {
}

void RButWin::draw() {
  init_gui();
  switch (style.st) {
    case 0: // normal
      draw_raised(0,cButBground,false);
      break;
    case 1: // checkboxes
      clear();
      break;
    case 2: // for menu
      clear();
      rectangleColor(win,0,0,tw_area.w-1,tw_area.h-1,grey0);
      break;
  }
  for (int i=0;i<=d->butnr;++i) draw_rbutton(d->but+i);
};

RButton* RButWin::is_in_rbutton(SDL_MouseButtonEvent *ev) {
  int nr=(ev->y-tw_area.y + y_off)/b_dist;
  if (nr>d->butnr) return 0;
  return d->but+max(0,nr);
}

void RButWin::draw_rbutton(RButton *rb) {
  int nr=rb-d->but,
      y1=nr*b_dist-y_off;
  if (y1<0 || y1>=tw_area.h) return;
  switch (style.st) {
    case 0: // normal
      SDL_FillRect(win,rp(1,y1+1,tw_area.w-2,b_dist-2),rb==d->act_button ? cSelRBut : cButBground);
      rb->label.draw(win,nr,Point(4,y1));
      break;
    case 1: // with checkboxes
      SDL_FillRect(win,rp(0,y1,tw_area.w,b_dist),parent->bgcol);
      switch(style.param) {
        case 1:  // square
          draw_raised(rp(0,y1,12,12),cButBground,false);
          break;
        default: // round
          if (!pixm.rbut_pm) pixm.rbut_pm=create_pixmap(rbut_pm);
          SDL_BlitSurface(pixm.rbut_pm,0,win,rp(0,y1,0,0));
      }
      if (rb==d->act_button)
        filledCircleColor(win,6,y1+6,2,0xff);
      rb->label.draw(win,nr,Point(15,y1));
      break;
    case 2: // for menu
      SDL_FillRect(win,rp(1,y1+1,tw_area.w-2,b_dist-2),rb==d->act_button ? cSelCmdMenu : cMenuBground);
      rb->label.draw(win,nr,Point(4,y1));
      break;
  }
}

int RButWin::act_rbutnr() {
  if (!d->act_button) return -1;
  return d->act_button-d->but;
}

RButton* RButWin::act_button()
{
	return d->act_button;
}

void RButWin::reset() {
  d->butnr=-1; y_off=0;
  d->act_button=0;
}
void RButWin::set_rbut(RButton *rb,int fire,bool do_draw) {
  d->act_button=rb;
  if (fire && rb->cmd) rb->cmd(this,rb-d->but,fire);
  if (do_draw) draw_blit_upd();
}
void RButWin::set_rbutnr(int nr,int fire,bool do_draw) {
  if (nr>d->butnr)
    alert("set_rbutnr: bad index %d",nr);
  else {
    if (nr<0)
      d->act_button=0;
    else
      d->act_button=d->but+nr;
    if (fire && d->act_button && d->act_button->cmd) d->act_button->cmd(this,d->act_button-d->but,fire);
    if (do_draw) draw_blit_upd();
  }
}

RButton* RButWin::add_rbut(Label lab) {
  init_gui();
  RButton *rb;
  int nr=d->next();
  d->but[nr].label=lab;
  rb=d->but+nr;
  if (nr==0 && !maybe_z) d->act_button=rb;
  rb->cmd=rb_cmd;
  if (sdl_running) draw_rbutton(rb);
  return rb;
}
int RButWinData::next() {
  if (butnr==rb_max-1) {
    but=re_alloc(but,rb_max);
  }
  return ++butnr;
}
RButton::RButton(int nr1,Label lab):
  label(lab) {
}
RButton::RButton():label(0,0) {
}

ExtRButCtrl::ExtRButCtrl(Style st,void(*cmd)(RExtButton*,bool)):
    butnr(-1),
    maybe_z(true),
    style(st),
    act_lbut(0),
    reb_cmd(cmd) {
}

int ExtRButCtrl::next() {
  return ++butnr;
}

RExtButton::RExtButton(WinBase *pw,Style st,Rect rect,Label lab,Id _id):
    WinBase(pw,0,rect.x,rect.y,rect.w?rect.w:16,rect.h?rect.h:16,0,_id),
    style(st),
    label(lab) {
}

RExtButton *ExtRButCtrl::add_extrbut(WinBase *pw,Rect rect,Label lab,Id id) {
  RExtButton *rb;
  ++butnr;
  rb=new RExtButton(pw,style,rect,lab,id);
  rb->rxb_ctr=this;
  return rb;
}

void ExtRButCtrl::set_rbut(RExtButton* rb,int fire) {
  RExtButton *act_old=act_lbut;
  act_lbut=rb;
  if (act_old) act_old->draw_blit_upd();
  rb->draw_blit_upd();
  if (fire && reb_cmd) reb_cmd(rb,true);
}

void ExtRButCtrl::reset()
{
	RExtButton *act_old=act_lbut;
	act_lbut=0;
	if (act_old) act_old->draw_blit_upd();
}

void RExtButton::draw() {
  init_gui();
  bool is_act = rxb_ctr->act_lbut==this;
  switch (style.st) {
    case 0:   // especially for tabbed windows
    case 3:
      if (is_act)   // like draw_raised(), no lowest/left line
        SDL_FillRect(win,0,style.param);
      else {
        SDL_FillRect(win,0,cGrey);
        if (style.st==0) // for top tabs
          hlineColor(win,0,tw_area.w-1,tw_area.h-1,white);
        else             // for rightside tabs
          vlineColor(win,0,0,tw_area.h-1,grey0); 
      }
      hlineColor(win,1,tw_area.w-1,0,white);
      vlineColor(win,tw_area.w-1,0,tw_area.h-2,grey0); 
      if (style.st==0)
        vlineColor(win,0,0,tw_area.h-1,white);
      else
        hlineColor(win,0,tw_area.w-1,tw_area.h-1,grey0);
      label.draw(win,id.id2,Point(3,tw_area.h/2-7));
      break;
    case 1:
    case 2:
      if (style.st==1)
        draw_gradient(Rect(0,0,tw_area.w,tw_area.h),int2col5('but',style.param),false,true);
      else
        SDL_FillRect(win,rp(0,0,tw_area.w,tw_area.h),style.param);
      if (is_act) {
        rectangleColor(win,0,0,tw_area.w-1,tw_area.h-1,red);
      }
      else {
        rectangleColor(win,0,0,tw_area.w-2,tw_area.h-2,white);
        rectangleColor(win,1,1,tw_area.w-1,tw_area.h-1,grey0);
      }
      label.draw(win,id.id2,Point(3,tw_area.h/2-8));
      break;
  }
}

TextWin::TextWin(WinBase *pw,Style st,Rect rect,int lm,const char *t,Id _id):
    WinBase(pw,t,rect.x,rect.y,rect.w,rect.h?rect.h:lm*TDIST+4,cWhite,_id),
    linenr(-1),
    lmax(lm),
    style(st),
    textbuf(new char[lmax][SMAX]) {
}

void TextWin::add_text(const char *s,bool do_draw) {
  const char *p,
             *prev=s;
  for (p=s;p<s+SMAX;++p) {
    if (!*p || *p=='\n') {
      ++linenr;
      char *txt=textbuf[linenr % lmax];
      strncpy(txt,prev,p-prev);
      txt[p-prev]=0;
      if (!*p) break;
      prev=p+1;
    }
  }
  if (do_draw) draw_blit_upd();
}

void TextWin::reset() {
  linenr=-1;
}

void TextWin::draw() {
  init_gui();
  int n,n1;
  char *p;
  switch (style.st) {
    case 0: clear(); break;
    case 1: draw_raised(0,bgcol,style.param); break;
  }
  n= linenr<lmax ? 0 : linenr-lmax+1;
  for (n1=n;n1<n+lmax && n1<=linenr;++n1) {
    int ind=n1%lmax;
    p=textbuf[ind];
    draw_ttf->draw_string(win,p,Point(4,TDIST*(n1-n+1)-12));
  }
}

HScrollbar::HScrollbar(WinBase *pw,Style st,Rect rect,int r,void (*_cmd)(HScrollbar*,int,int),Id _id):
    WinBase(pw,0,rect.x,rect.y,rect.w,rect.h ? rect.h : 12,cSlBackgr,_id),
    p0(0),xpos(0),value(0),
    style(st),
    ssdim(style.st==1 ? 12 : 0),
    cmd(_cmd) {
  if (!style.param2) style.param2=5; // step if repeat
  calc_params(r);
}

void HScrollbar::calc_params(int r) {
  range=max(tw_area.w-2*ssdim,r-2*ssdim);
  wid=max(2,(tw_area.w-2*ssdim) * (tw_area.w-2*ssdim) / range);
}

void HScrollbar::set_range(int r) {
  calc_params(r);
  int xp=minmax(0,value*(tw_area.w-2*ssdim)/range,tw_area.w-2*ssdim-wid);
  p0-=xpos-xp;
  xpos=xp;
  draw_blit_upd();
}

void HScrollbar::draw() {
  init_gui();
  draw_raised(0,bgcol,false);
  const Color5 *col=int2col5('scrl',style.param);
  draw_gradient(Rect(xpos+ssdim,0,wid,tw_area.h),col);
  if (style.st==1) {
    draw_gradient(Rect(0,0,ssdim,tw_area.h),col);
    draw_gradient(Rect(tw_area.w-ssdim,0,ssdim,tw_area.h),col);
    filledTrigonColor(win,6,2,6,8,2,5,0xff);
    filledTrigonColor(win,tw_area.w-7,2,tw_area.w-4,5,tw_area.w-7,8,0xff);
  }
}

void HScrollbar::inc_value(bool incr) {
  int val=value,
      xp;
  if (incr) {
    val+=style.param2;
    xp=val*(tw_area.w-2*ssdim)/range;
    if (xp>tw_area.w-2*ssdim-wid) return;
  }
  else {
    val-=style.param2;
    if (val<0) return;
    xp=val*(tw_area.w-2*ssdim)/range;
  }
  value=val;
  if (cmd) cmd(this,value,range);
  if (xp!=xpos) {
    xpos=xp;
    draw_blit_upd();
  }
}

void HScrollbar::calc_xpos(int newx) {
  int xp=minmax(0,xpos + newx - p0,tw_area.w-2*ssdim-wid);
  p0=newx;
  if (xp!=xpos) {
    xpos=xp;
    value=xpos * range / (tw_area.w-2*ssdim);
    draw_blit_upd();
    if (cmd) cmd(this,value,range);
  }
}

void HScrollbar::set_xpos(int newx) {
  int xp=minmax(0,newx*(tw_area.w-2*ssdim)/range,tw_area.w-2*ssdim-wid);
  if (xp!=xpos) {
    xpos=xp;
    draw_blit_upd();
  }
}

bool HScrollbar::in_ss_area(SDL_MouseButtonEvent *ev,bool *dir) {
  if (style.st==0) return false;
  if (ev->x >= tw_area.x + tw_area.w - ssdim) { if (dir) *dir=true; return true; }
  if (ev->x <= tw_area.x + ssdim) { if (dir) *dir=false; return true; }
  return false;
}

VScrollbar::VScrollbar(WinBase *pw,Style st,Rect rect,int r,void (*_cmd)(VScrollbar*,int,int),Id _id):
    WinBase(pw,0,rect.x,rect.y,rect.w ? rect.w : 12,rect.h,cSlBackgr,_id),
    p0(0),ypos(0),value(0),
    style(st),
    ssdim(style.st==1 ? 10 : 0),
    cmd(_cmd) {
  if (!style.param2) style.param2=5; // step if repeat
  calc_params(r);
}

void VScrollbar::set_range(int r) {
  calc_params(r);
  int yp=minmax(0,value*(tw_area.h-2*ssdim)/range,tw_area.h-2*ssdim-height);
  p0-=ypos-yp;
  ypos=yp;
  draw_blit_upd();
}

void VScrollbar::calc_params(int r) {
  range=max(tw_area.h-2*ssdim,r-2*ssdim);
  height=max((tw_area.h-2*ssdim) * (tw_area.h-2*ssdim) / range,2);
}

void VScrollbar::draw() {
  init_gui();
  draw_raised(0,bgcol,false);
  const Color5 *col=int2col5('scrl',style.param);
  draw_gradient(Rect(0,ypos+ssdim,tw_area.w,height),col,true);
  if (style.st==1) {
    draw_gradient(Rect(0,0,tw_area.w,ssdim),col,true);
    draw_gradient(Rect(0,tw_area.h-ssdim,tw_area.w,ssdim),col,true);
    filledTrigonColor(win,2,6,8,6,5,2,0xff);
    filledTrigonColor(win,2,tw_area.h-7,5,tw_area.h-3,8,tw_area.h-7,0xff);
  }
}

void VScrollbar::inc_value(bool incr) {
  int val=value,
      yp;
  if (incr) { 
    val+=style.param2;
    yp=val*(tw_area.h-2*ssdim)/range;
    if (yp>tw_area.h-2*ssdim-height) return;
  }
  else {
    val-=style.param2;
    if (val<0) return;
    yp=val*(tw_area.h-2*ssdim)/range;
  }
  value=val;
  if (cmd) cmd(this,value,range);
  if (yp!=ypos) {
    ypos=yp;
    draw_blit_upd();
  }
}

void VScrollbar::calc_ypos(int newy) {
  int yp=minmax(0,ypos + newy - p0,tw_area.h-2*ssdim-height);
  p0=newy;
  if (yp!=ypos) {
    ypos=yp;
    value=ypos * range / (tw_area.h-2*ssdim);
    draw_blit_upd();
    if (cmd) cmd(this,value,range);
  }
}

void VScrollbar::set_ypos(int newy) {
  int yp=minmax(0,newy*(tw_area.h-2*ssdim)/range,tw_area.h-2*ssdim-height);
  if (yp!=ypos) {
    ypos=yp;
    draw_blit_upd();
  }
}

bool VScrollbar::in_ss_area(SDL_MouseButtonEvent *ev,bool *dir) {
  if (style.st==0) return false;
  if (ev->y >= tw_area.y + tw_area.h - ssdim) { if (dir) *dir=true; return true; }
  if (ev->y <= tw_area.y + ssdim) { if (dir) *dir=false; return true; }
  return false;
}

Lamp::Lamp(WinBase *pw,Rect r):
    pwin(pw),
    rect(r),
    col(white),
    hidden(false) {
}

void Lamp::draw() {
  if (hidden) return;
  if (!pixm.lamp_pm) pixm.lamp_pm=create_pixmap(lamp_pm);
  SDL_BlitSurface(pixm.lamp_pm,0,pwin->win,&rect);
  filledCircleColor(pwin->win,rect.x+6,rect.y+6,4,col);
}

void Lamp::set_color(Uint32 c) {
  col=c;
  draw();
  pwin->blit_upd(&rect);
}

void Lamp::show() {
  if (!hidden) return;
  hidden=false;
  draw();
  pwin->blit_upd(&rect);
}

Message::Message(WinBase *pw,Style st,const char* lab,Point top):
    pwin(pw),
    style(st),
    label(lab),
    lab_pt(top),
    mes_r(0,top.y,0,TDIST+1),
    bgcol(style.param ? style.param : pw->bgcol) {
  mes_r.x= label ? draw_ttf->text_width(label)+top.x+2 : top.x;
}

void Message::draw_label(bool upd) {
  draw_ttf->draw_string(pwin->win,label,lab_pt);
  if (upd) pwin->blit_upd(rp(lab_pt.x,lab_pt.y,mes_r.x,14));
}

void Message::draw_mes(const char *form,...) {
  va_list ap;
  va_start(ap,form);
  draw_message(form,ap);
  va_end(ap);
  pwin->blit_upd(&mes_r);
}

void Message::draw(const char *form,...) {
  draw_label();
  va_list ap;
  va_start(ap,form);
  draw_message(form,ap);
  va_end(ap);
}

void Message::draw_message(const char *form,va_list ap) {
  char buf[100];
  vsnprintf(buf,100,form,ap);
  int twid=draw_title_ttf->text_width(buf);
  switch (style.st) {
    case 0:  // title font
      if (mes_r.w<twid) mes_r.w=twid;
      SDL_FillRect(pwin->win,&mes_r,bgcol);
      draw_title_ttf->draw_string(pwin->win,buf,Point(mes_r.x,mes_r.y));
      break;
    case 1:  // 3D
      if (mes_r.w<twid+2) mes_r.w=twid+2;
      pwin->draw_raised(&mes_r,bgcol,false);
      draw_title_ttf->draw_string(pwin->win,buf,Point(mes_r.x+1,mes_r.y));
      break;
    case 2:  // regular font
      if (mes_r.w<twid) mes_r.w=twid;
      SDL_FillRect(pwin->win,&mes_r,bgcol);
      draw_ttf->draw_string(pwin->win,buf,Point(mes_r.x,mes_r.y));
  }
}

CmdMenu::CmdMenu(Button *_src):
    nr_buttons(0),
    sticky(false),
    src(_src),
    buttons(0) {
}

bool CmdMenu::init(int wid,int nr_b,void (*menu_cmd)(RButWin*,int nr,int fire)) {
  if (the_menu) { the_menu->mclose(); the_menu=0; }
  if (buttons) return false;
  nr_buttons=nr_b;
  src->bgcol=cSelCmdMenu;
  int bdist=src->style.param2 ? src->style.param2 : TDIST,
      left=src->tw_area.x,
      top=src->tw_area.y+src->tw_area.h-1,
      height=nr_buttons*bdist+2;
  if (top+height>topw->tw_area.h) {
    left+=src->tw_area.w;
    top=max(0,src->tw_area.y-height/2);
    if (top+height>topw->tw_area.h)
      top=max(0,topw->tw_area.h-height);
  }
  else if (left+wid>topw->tw_area.w)
    left=src->area.x+src->tw_area.w-wid;
  mrect.set(left,top,wid,height);
  buttons=new RButWin(0,Style(2,0,bdist),mrect,0,!sticky,menu_cmd,src->id);
  buttons->keep_on_top();
  buttons->bgcol=cMenuBground;
  buttons->draw();
  the_menu_tmp=this;
  return true;
}

RButton *CmdMenu::add_mbut(Label lab) {
  if (buttons->d->butnr<=nr_buttons-2) {
    RButton *rb=buttons->add_rbut(lab);
    int nrb=buttons->d->butnr;
    if (nrb==nr_buttons-1) {
      buttons->blit_upd(); // draw already done
      buttons->maybe_z=!sticky;
    }
    return rb;
  }
  alert("add_mbut: > %d buttons",nr_buttons);
  return 0;
}

void CmdMenu::mclose() {
  src->bgcol=src->parent->bgcol;
  src->draw_blit_upd();
  if (buttons) { // can be 0
    buttons->hide();
    delete buttons;
    buttons=0;
  }
}

void WinBase::draw_blit_upd() {  // from child upward, title is not blitted
  if (!sdl_running) return;
  draw();
  blit_upd();
}

void WinBase::blit_upd(Rect *rect) {
  if (!sdl_running) return;
  Rect r1,r2;
  if (rect) r1=*rect;
  else r1.set(0,0,tw_area.w,tw_area.h);
  r2=r1;
  if (this==topw) upd(&r2);
  else if (parent==topw) {
    if (hidden) return;
    r2.x+=area.x; r2.y+=area.y;
    SDL_BlitSurface(win,&r1,parent->win,&r2);
    upd(&r2);
  }
  else {
    WinBase *wb,*nxt;
    for (wb=this,nxt=parent;nxt;wb=nxt,nxt=nxt->parent) {
      if (wb->hidden) return;
      r2.x+=wb->area.x; r2.y+=wb->area.y;
      SDL_BlitSurface(wb->win,&r1,nxt->win,&r2);
      if (nxt==topw) {
        wb->upd(&r2);
        return;
      }
      r1.x+=wb->area.x; r1.y+=wb->area.y;
    }
  }
}

void WinBase::border(WinBase *child,int wid) {  // default: wid=1
  Rect r(child->area.x,child->area.y,child->tw_area.w,child->tw_area.h);
  switch (wid) {
    case 1:
      rectangleColor(win,r.x-1,r.y-1,r.x+r.w,r.y+r.h,grey0);
      break;
    case 2:
      rectangleColor(win,r.x-2,r.y-2,r.x+r.w+1,r.y+r.h+1,white);
      rectangleColor(win,r.x-1,r.y-1,r.x+r.w+1,r.y+r.h+1,grey0);
      break;
    case 3:
      rectangleColor(win,r.x-3,r.y-3,r.x+r.w+1,r.y+r.h+1,white);
      rectangleColor(win,r.x-2,r.y-2,r.x+r.w+2,r.y+r.h+2,grey0);
      rectangleColor(win,r.x-1,r.y-1,r.x+r.w,r.y+r.h,grey3);
      rectangleColor(win,r.x-4,r.y-4,r.x+r.w+3,r.y+r.h+3,grey3);
      break;
    }
  child->reloc_title(0,-wid);
}

void WinBase::widen(int dx,int dy) {
  SDL_FreeSurface(win);
  win=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w+dx,tw_area.h+dy,pixdepth,0,0,0,0);
  tw_area.w += dx; tw_area.h += dy;
}

void WinBase::move(int dx,int dy) {
  area.x += dx; area.y += dy;
  title_area.x += dx; title_area.y += dy;
  move_tw_area(this,dx,dy); 
}

void WinBase::keep_on_top() {
  if (ontopw) return; // called more then once?
  if (parent) { alert("keep_on_top: parent != 0"); return; }
  if (!(parent=topw)) { alert("keep_on_top: top-window = 0"); return; }
  if (!bgcol) bgcol=cWhite;
  tw_area.x=area.x+parent->tw_area.x;
  tw_area.y=area.y+parent->tw_area.y;
  if (ontop_nr>=max_ontopwin-1) {
    int old=max_ontopwin;
    ontop_win=re_alloc(ontop_win,max_ontopwin);
    for (int i=0;i<old;++i)
      ontop_win[i].wb->ontopw=ontop_win+i; // re-assign pointers
  }
  if (awin::ok) { // then ontop_win[ontop_nr].wb == awin::bgr)
    OnTopWin *const otw=ontop_win+ontop_nr;
    ++ontop_nr;
    otw[1]=otw[0];
    otw[1].wb->ontopw=otw+1; // the awin::bgr
    otw->wb=this;
    ontopw=otw;
    otw->backup=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w,tw_area.h,pixdepth,0,0,0,0);
    SDL_BlitSurface(topw->win,&tw_area,otw->backup,0); // backup the screen
    if (!otw[1].wb->hidden) {
      Rect *r1=&tw_area,
           *r2=&otw[1].wb->tw_area,
           *clip=calc_overlap(r1,r2);
      if (clip) {
        Rect c1(clip->x-r1->x,clip->y-r1->y,0,0),
             c2(clip->x-r2->x,clip->y-r2->y,clip->w,clip->h);
        SDL_BlitSurface(otw[1].backup,&c2,otw->backup,&c1); // transfer backup
      }
    }
  }
  else {
    ++ontop_nr;
    OnTopWin *const otw=ontop_win+ontop_nr;
    otw->wb=this;
    ontopw=otw;
    otw->backup=SDL_CreateRGBSurface(SDL_SWSURFACE,tw_area.w,tw_area.h,pixdepth,0,0,0,0);
    // screen backup done by awin::show()
  }
  parent->add_child(this);
}

struct TheCursor {
  struct DialogWin *diawd;
  struct EditWin *edwd;
  bool keep;
  void unset() {
    if (diawd) { diawd->unset_cursor(); diawd=0; }
    if (edwd) { edwd->unset_cursor(); edwd=0; }
  }
} the_cursor;

struct Line {
  int dlen,     // occupied in data[]
      leftside,
      dmax;     // length of data[]
  char *data;
  Line():dlen(0),leftside(0),dmax(50),data(new char[dmax]) {
    data[0]=0;
  }
  int xpos(int nr) {
    char ch=data[nr];
    data[nr]=0;
    int pos;
    TTF_SizeText(mono_font,data+leftside,&pos,0);
    data[nr]=ch;
    return pos+3;
  }
  void reset() { data[0]=0; dlen=leftside=0; }
  void insert_char(int ks,int& n) {
    int i;
    if (dlen>=dmax-1) data=re_alloc(data,dmax);
    for (i=dlen;i>n;--i) data[i]=data[i-1];
    data[i]=ks;
    ++dlen; ++n; data[dlen]=0;
  }
  void rm_char(int& n) {
    int i;
    if (dlen==0 || n==0) return;
    for (i=n-1;i<dlen;++i) data[i]=data[i+1];
    --dlen; --n;
  }
  void cpy(const char *s) {
    dlen=strlen(s);
    while (dmax<=dlen) data=re_alloc(data,dmax);
    strcpy(data,s);
  }
  void cat(char *s) {
    dlen+=strlen(s);
    while (dmax<=dlen) data=re_alloc(data,dmax);
    strcat(data,s);
  }
};

DialogWin::DialogWin(WinBase *pw,Rect rect,Id _id):
    WinBase(pw,0,rect.x,rect.y,rect.w,2*TDIST+2,pw->bgcol,_id),
    textr(0,TDIST,rect.w,TDIST+2),
    cursor(-1),
    label(0),
    lin(new Line),
    cmd(0) {
}

void DialogWin::set_cursor(int x) {
  int wid=0,
      i=lin->leftside;
  for (;lin->data[i];++i) {
    wid+=draw_mono_ttf->ch_wid[lin->data[i]-' '];
    if (wid >= x - tw_area.x) break;
  }
  cursor=min(lin->dlen,i);
  if (the_cursor.edwd) { the_cursor.edwd->unset_cursor(); the_cursor.edwd=0; }
  the_cursor.diawd=this;
  draw_line();
}

void DialogWin::unset_cursor() {
  if (cursor>=0) {
    cursor=-1;
    lin->leftside=0;
    draw_line();
  }
}

void DialogWin::dok() {
  if (cmd) cmd(lin->data,cmd_id);
  else alert("dialog: no action specified");
}

void DialogWin::draw() {
  init_gui();
  clear(rp(0,0,tw_area.w,TDIST));
  draw_raised(&textr,cButBground,false);
  if (label)
    draw_ttf->draw_string(win,label,Point(2,0));
  draw_line();
}

void DialogWin::dialog_label(const char *s,Uint32 col) {
  label=s;
  Rect rect(0,0,tw_area.w,TDIST);
  clear(&rect);
  blit_upd(&rect);
  if (label) {
    rect.w=draw_ttf->text_width(label)+3;
    if (col) clear(&rect,col,false);
    draw_ttf->draw_string(win,label,Point(2,0));
    blit_upd(&rect);
  }
}

void DialogWin::draw_line() {
  char *s=lin->data;
  const int rmargin=15;
  draw_raised(&textr,cButBground,false);
  if (cursor>=0) {
    if (lin->xpos(cursor)>=tw_area.w-rmargin)
      lin->leftside=cursor - (tw_area.w-rmargin)/char_wid;
    int x1=lin->xpos(cursor);
    vlineColor(win,x1,textr.y+2,textr.y+textr.h-2,0xff);
  }
  draw_mono_ttf->draw_string(win,s+lin->leftside,Point(2,textr.y));
  blit_upd(&textr);
}

void DialogWin::dialog_def(const char *s,void(*_cmd)(const char*,int cmdid),int _cmd_id) {
  if (_cmd) cmd=_cmd;
  if (_cmd_id) cmd_id=_cmd_id;
  if (s) {
    lin->data[lin->dmax-1]=0;
    strncpy(lin->data,s,lin->dmax-1);
    lin->dlen=strlen(lin->data);
  }
  else { lin->dlen=0; lin->data[0]=0; }
  lin->leftside=0;
  set_cursor(lin->dlen);
  the_cursor.keep=true;
}

static void lr_shift(int sym,int &ks) {  // left or right shift key
  if (sym!=KMOD_LSHIFT && sym!=KMOD_RSHIFT) return;
  switch (ks) {  // todo: more symbols
    case '`': ks='~'; break;
    case '1': ks='!'; break;
    case '2': ks='@'; break;
    case '3': ks='#'; break;
    case '4': ks='$'; break;
    case '5': ks='%'; break;
    case '6': ks='^'; break;
    case '7': ks='&'; break;
    case '8': ks='*'; break;
    case '9': ks='('; break;
    case '0': ks=')'; break;
    case ';': ks=':'; break;
    case '-': ks='_'; break;
    case '=': ks='+'; break;
    case '\\': ks='|'; break;
    case '[': ks='{'; break;
    case ']': ks='}'; break;
    case '\'': ks='"'; break; 
    case ',': ks='<'; break;
    case '.': ks='>'; break;
    case '/': ks='?'; break;
    default:
      if (ks>='a' && ks<='z')
        ks += 'A'-'a';
      else ks='?';
  }
}

bool DialogWin::handle_key(SDL_keysym *key) {
  int ks=key->sym; // sym is an enum
  switch (ks) {
    case SDLK_LCTRL: case SDLK_RCTRL:
      break;
    case SDLK_RETURN:
      unset_cursor();
      return true;
    case SDLK_BACKSPACE:
      lin->rm_char(cursor);
      if (lin->xpos(cursor)<10 && lin->leftside>0) --lin->leftside;
      draw_line();
      break;
    case SDLK_LEFT:
      if (cursor>0) {
        --cursor;
        if (lin->xpos(cursor)<10 && lin->leftside>0) --lin->leftside;
        draw_line();
      }
      break;
    case SDLK_RIGHT:
      if (cursor<lin->dlen) {
        ++cursor;
        if (lin->xpos(cursor)>=tw_area.w-10) ++lin->leftside;
        draw_line();
      }
      break;
    default:
      if (ks>=0x20 && ks<0x80) {
        if (key->mod==KMOD_LCTRL || key->mod==KMOD_RCTRL) {
          if (ks==SDLK_d) {
            lin->reset();
            cursor=0; 
            draw_line();
          }
        }
        else {
          lr_shift(key->mod,ks);
          lin->insert_char(ks,cursor);
          if (lin->xpos(cursor)>tw_area.w-10) ++lin->leftside;
          draw_line();
        }
      }
  }
  return false;
}

EditWin::EditWin(WinBase *pw,Rect rect,const char *tit,void (*_cmd)(int ctrl_key,int key,int cmd_id),Id _id):
    WinBase(pw,tit,rect.x,rect.y,rect.w,rect.h,cWhite,_id),
    linenr(-1),
    lmax(10),
    y_off(0),
    lines(new_nulled(lines,lmax)),
    cmd(_cmd) {
  cursor.y=cursor.x=-1;
}

void WinBase::set_parent(WinBase *pw) {
  if (parent) { alert("set_parent: parent != 0"); return; }
  parent=pw;
  parent->add_child(this);
  tw_area.x=area.x+parent->tw_area.x;
  tw_area.y=area.y+parent->tw_area.y;
}

void EditWin::draw() {
  init_gui();
  clear();
  for (int i=0;i<=linenr;++i)
    draw_line(i,false);
  rectangleColor(win,0,0,tw_area.w-1,tw_area.h-1,grey0);
}

void EditWin::draw_line(int vpos,bool update) {
  int y1=vpos*TDIST-y_off;
  if (y1<-TDIST || y1>tw_area.h) return;
  clear(rp(1,y1,tw_area.w-2,TDIST));
  Line *lin=lines[vpos];
  if (cursor.y==vpos && cursor.x>=0) {
    int x1;
    if (lin) {
      if (cursor.x<0)
        lin->leftside=0;
      if (lin->xpos(cursor.x)>=tw_area.w-10)
        lin->leftside=cursor.x - (tw_area.w-10)/char_wid;
      x1=lin->xpos(cursor.x);
    }
    else
      x1=2;
    vlineColor(win,x1,y1,y1+13,0xff);
  }
  if (lin && lin->dlen) {
    draw_mono_ttf->draw_string(win,lin->data+lin->leftside,Point(2,y1));
  }
  if (update) blit_upd(rp(0,y1,tw_area.w,TDIST));
}

void EditWin::set_offset(int yoff) {
  if (abs(yoff-y_off)<5) return;
  int i,
      lst_y=linenr*TDIST-yoff;
  y_off=yoff;
  the_cursor.keep=true;
  clear(rp(0,lst_y,tw_area.w,tw_area.h-lst_y));
  for (i=0;i<=linenr;++i) draw_line(i,false);
  rectangleColor(win,0,0,tw_area.w-1,tw_area.h-1,grey0); // restore border
  blit_upd();
}

void EditWin::set_cursor(int x1,int y1) {
  if (the_cursor.diawd) {
    the_cursor.diawd->unset_cursor(); the_cursor.diawd=0;
  }
  else if (the_cursor.edwd && the_cursor.edwd!=this) {
    the_cursor.edwd->unset_cursor();
  }
  the_cursor.edwd=this;
  int new_y;
  if (linenr<0) y1=linenr=new_y=0;
  else new_y=max(0,(y1-tw_area.y+y_off)/TDIST);
  if (cursor.y!=new_y && cursor.y>=0) {
    cursor.x=-1; draw_line(cursor.y,true);  // print previous line, without cursor
  }
  cursor.y=new_y;
  if (cursor.y>linenr) {
    ++linenr;
    cursor.y=linenr;
  }
  Line *lin=lines[cursor.y];
  if (!lin)
    cursor.x=0;
  else {
    int wid=0,
        i=lin->leftside;
    for (;lin->data[i];++i) {
      wid+=draw_mono_ttf->ch_wid[lin->data[i]-' '];
      if (wid >= x1 - tw_area.x) break;
    }
    cursor.x=min(lin->dlen,i);
  }
  draw_line(cursor.y,true);
}

void EditWin::unset_cursor() {
  if (cursor.x>=0) {
    cursor.x=-1;
    if (cursor.y>=0) {
      Line *lin=lines[cursor.y];
      if (lin) lin->leftside=0;
      draw_line(cursor.y,true);
    }
  }
}
/*
void EditWin::read_file(FILE* in) {
  int len;
  char textbuf[1000];
  Line *lin;
  y_off=0;
  reset();
  for (;;) {
    if (!fgets(textbuf,1000-1,in)) break;
    len=strlen(textbuf)-1;
    textbuf[len]=0; // strip '\n'
    ++linenr;
    if (linenr>=lmax) lines=re_alloc(lines,lmax);
    if (!lines[linenr]) lines[linenr]=new Line;
    lin=lines[linenr];
    lin->cpy(textbuf);
    lin->dlen=len;
    if (sdl_running) draw_line(linenr,true);
  }
}
*/
static bool var_args(const char *form) {
  for (const char *p=form;*p;++p)
    if (*p=='%') return true;
  return false;
}

void EditWin::set_line(int n,bool update,const char *form,...) {
  if (n>=lmax-1) lines=re_alloc(lines,lmax);
  if (n>linenr) linenr=n;
  if (!lines[n]) lines[n]=new Line;
  if (var_args(form)) {
    char buf[200];
    va_list ap;
    va_start(ap,form);
    vsnprintf(buf,200,form,ap);
    va_end(ap);
    lines[n]->cpy(buf);
  }
  else
    lines[n]->cpy(form);
  if (sdl_running) draw_line(n,update);
}

void EditWin::insert_line(int n,bool update,const char *form,...) { // insert a line at lines[n]
  if (the_cursor.edwd==this && cursor.y>=n) ++cursor.y;
  if (n>linenr)
    linenr=n;
  else {
    ++linenr;
    for (int i=linenr+1;i>n;--i) {
      lines[i]=lines[i-1];
      draw_line(i,update);
    }
  }
  if (linenr>=lmax-1) lines=re_alloc(lines,lmax);
  lines[n]=new Line;
  if (var_args(form)) {
    char buf[200];
    va_list ap;
    va_start(ap,form);
    vsnprintf(buf,200,form,ap);
    va_end(ap);
    lines[n]->cpy(buf);
  }
  else
    lines[n]->cpy(form);
  draw_line(n,update);
}
/*
void EditWin::write_file(FILE *out) {
  for (int i=0;i<=linenr;++i) {
    Line *lin=lines[i];
    if (lin)
      fputs(lin->data,out);
    putc('\n',out);
  }
}
*/
const char *EditWin::get_line(int n) {
  if (n<=linenr) {
    return lines[n] ? lines[n]->data : "";
  }
  alert("get_line n > %d",linenr);
  return 0;
}
/*
char* EditWin::get_text(char *buf,int maxlen) {
  char *p=buf;
  int ln;
  for (ln=0;ln<=linenr;++ln) {
    if (lines[ln]) {
      if (p-buf >= maxlen-lines[ln]->dlen) {
        alert("get_text: buffer too small: %d",maxlen); 
        break;
      }
      p=stpncpy(p,lines[ln]->data,lines[ln]->dlen);
    }
    *p='\n';
    if (p-buf<maxlen) ++p;
    else { alert("get_text: buffer too small"); break; }
  }
  *p=0;
  return buf;
}
*/
void EditWin::reset() { // y_off is kept
  for (int i=0;i<=linenr;++i)
    if (lines[i]) { lines[i]->data[0]=0; lines[i]->dlen=0; }
  linenr=-1;
}

void EditWin::handle_key(SDL_keysym *key) {
  if (cursor.y<0) return;
  int i,
      ks1=key->sym; // sym is an enum
  Line *lin=lines[cursor.y],
       *lin2;

  switch (ks1) {
    case SDLK_RETURN:
      if (cmd) cmd(0,ks1,id.id1);
      ++linenr;
      if (linenr>=lmax) lines=re_alloc(lines,lmax);
      for (i=linenr;i>cursor.y+1;--i) {
        lines[i]=lines[i-1];
        draw_line(i,true);
      }
      lin=lines[cursor.y];
      ++cursor.y;
      if (lin) {
        if (lin->data[cursor.x])
          (lines[cursor.y]=new Line)->cpy(lin->data+cursor.x);
        else
          lines[cursor.y]=0;
        lin->dlen=cursor.x;
        lin->data[lin->dlen]=0;
        lin->leftside=0;
      }
      cursor.x=0;
      draw_line(cursor.y-1,true);
      draw_line(cursor.y,true);
      break;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
      break;
    case SDLK_BACKSPACE:
      if (cmd) cmd(0,ks1,id.id1);
      if (cursor.x==0) {
        if (cursor.y==0) return;
        if (lines[cursor.y-1]) {
          lin=lines[cursor.y-1];
          cursor.x=lin->dlen;
          lin2=lines[cursor.y];
          if (lin2) {
            while (lin->dlen+lin2->dlen>=lin->dmax-1)
              lin->data=re_alloc(lin->data,lin->dmax);
            lin->cat(lin2->data);
          }
        }
        else {
          lines[cursor.y-1]=lines[cursor.y];
          cursor.x=0;
        }
        --cursor.y;
        draw_line(cursor.y,true);

        for (i=cursor.y+1;i<linenr;++i) {
          lines[i]=lines[i+1];
          draw_line(i,true);
        }
        lines[linenr]=0;
        draw_line(linenr,true);
        --linenr;
      }
      else {
        lin->rm_char(cursor.x);
        if (lin->xpos(cursor.x)<10 && lin->leftside>0) --lin->leftside;
        draw_line(cursor.y,true);
      }
      break;
    case SDLK_LEFT:
      if (cursor.x>0) {
        --cursor.x;
        if (lin->xpos(cursor.x)<10 && lin->leftside>0) --lin->leftside;
        draw_line(cursor.y,true);
      }
      break;
    case SDLK_RIGHT:
      if (cursor.x<lines[cursor.y]->dlen) {
        ++cursor.x;
        draw_line(cursor.y,true);
      }
      break;
    case SDLK_UP:
      if (cursor.y>0) {
        --cursor.y;
        lin=lines[cursor.y];
        if (lin) {
          if (cursor.x>lin->dlen) cursor.x=lin->dlen;
        }
        else cursor.x=0;
        draw_line(cursor.y,true);
        lin=lines[cursor.y+1];
        if (lin) lin->leftside=0;
        draw_line(cursor.y+1,true);
      }
      break;
    case SDLK_DOWN:
      if (cursor.y<linenr) {
        ++cursor.y;
        lin=lines[cursor.y];
        if (lin) {
          if (cursor.x>lin->dlen) cursor.x=lin->dlen;
        }
        else cursor.x=0;
        draw_line(cursor.y,true);
        lin=lines[cursor.y-1];
        if (lin) lin->leftside=0;
        draw_line(cursor.y-1,true);
      }
      break;
    default:
      if (cmd) cmd(key->mod,ks1,id.id1);
      if (ks1>=0x20 && ks1<0x80) {
        if (key->mod==KMOD_LCTRL || key->mod==KMOD_RCTRL) {
          if (ks1==SDLK_d) {
            if (lin) lin->reset();
            cursor.x=0;
            draw_line(cursor.y,true);
          }
        }
        else {
          lr_shift(key->mod,ks1);
          if (!lin) lin=lines[cursor.y]=new Line;
          lin->insert_char(ks1,cursor.x);
          if (lin->xpos(cursor.x)>tw_area.w-10) ++lin->leftside;
          draw_line(cursor.y,true);
        }
     }
  }
}

void EditWin::get_info(bool *active,int* nr_lines,int* cursor_ypos,int *nr_chars,int *total_nr_chars) {
  if (active) *active= the_cursor.edwd==this;
  if (nr_lines) *nr_lines=linenr;
  if (cursor_ypos) *cursor_ypos=cursor.y;
  if (nr_chars) {
    if (cursor.y>=0 && lines[cursor.y]) *nr_chars= lines[cursor.y]->dlen;
    else *nr_chars=-1;
  }
  if (total_nr_chars) {
    *total_nr_chars=0;
    for (int i=0;i<=linenr;++i) {
      if (lines[i]) *total_nr_chars += lines[i]->dlen;
    }
  }
}

static struct FileChooser *f_chooser;

struct FileChooser {
  Id id;
  int f_max,d_max,
      dname_nr,fname_nr;
  void (*callback)(const char* path,Id id);
  char
    **dir_names,
    **f_names;
  BgrWin *bgw;
  RButWin *buttons;
  VScrollbar *sbar;
  static const int win_h=10*TDIST;  // height
  static void button_cmd(Button*) {
    f_chooser->bgw->hide();
  }
  static void draw_otwin(BgrWin *bgw) {
    if (the_menu) {
      the_menu->mclose(); the_menu=0;
    }
    bgw->clear();
    rectangleColor(bgw->win,0,0,bgw->tw_area.w-1,bgw->tw_area.h-1,0xff);
  }
  static int compare_fn(const void* a,const void* b) {
    return strcasecmp(*reinterpret_cast<char* const*>(a),*reinterpret_cast<char* const*>(b));
  }
  void rbw_fch_cmd(int nr);
  static void rbw_fch_cmd(RButWin*,int nr,int fire) {
    if (the_menu) { the_menu->mclose(); the_menu=0; }
    f_chooser->rbw_fch_cmd(nr);
  }
  void rbw_wdir_cmd(int nr) {
    if (chdir(dir_names[nr])) { alert("chdir failed"); return; }
    const char* cur_wdir=getcwd(0,0);
    SDL_WM_SetCaption(cur_wdir,0);
    working_dir();
    if (callback)
      callback(cur_wdir,id); // cur_wdir not free'd
  }
  static void rbw_wdir_cmd(RButWin*,int nr,int fire) {
    if (the_menu) { the_menu->mclose(); the_menu=0; }
    f_chooser->rbw_wdir_cmd(nr);
  }
  static void scb_cmd(VScrollbar*,int val,int range) {
    RButWin *rb=f_chooser->buttons;
    if (rb->y_off/TDIST!=val/TDIST) {
      rb->y_off=(val/TDIST)*TDIST;
      rb->draw_blit_upd();
    }
  }
  FileChooser();
  char *add_slash(const char* s,char c) {
    char *dn=(char*)malloc(strlen(s)+2);
    sprintf(dn,"%s%c",s,c);
    return dn;
  }
  bool fill_fname_array(int &dir_nr,int &fname_nr);
  bool fill_dname_array(int &dir_nr);
  void choose_file();
  void working_dir();
};

void FileChooser::rbw_fch_cmd(int nr) {
  if (nr<=dname_nr) {
    if (chdir(dir_names[nr])) { alert("chdir failed"); return; }
    const char *cur_wdir=getcwd(0,0); SDL_WM_SetCaption(cur_wdir,0);
    choose_file();
    free((void*)cur_wdir);
  }
  else {
    const char* fn=f_names[nr-dname_nr-1];
    if (callback) {
      int lst=strlen(fn)-1;
      if (fn[lst]=='@') const_cast<char*>(fn)[lst]=0; // destructive removal of last '@'
      callback(strdup(fn),id);  // strdup because fn is volatile
    }
    bgw->hide();
  }
}

FileChooser::FileChooser():
    id(0),
    f_max(50),d_max(50),
    dir_names(new char*[d_max]),
    f_names(new char*[f_max]) {
  bgw=new BgrWin(0,Rect(0,0,200,win_h+5),0,draw_otwin,mwin::down,mwin::move,mwin::up,cGrey);
  bgw->keep_on_top();
  new Button(bgw,Style(0,1),Rect(bgw->tw_area.w-20,3,14,13),"X",button_cmd);
  bgw->draw_blit_recur();
  bgw->upd();
  buttons=new RButWin(bgw,0,Rect(2,2,160,win_h),0,false,0); // will be drawn later
  sbar=new VScrollbar(bgw,Style(1,0,TDIST),Rect(164,2,0,win_h),0,scb_cmd);
}

bool FileChooser::fill_fname_array(int &dir_nr,int &file_nr) {
  DIR *dp;
  dirent *dir;
  dir_nr=file_nr=-1;
  const char *wd=getcwd(0,0);
  if (!(dp=opendir(wd))) {
    alert("wdir not accessable");
    return false;
  }
  free((void*)wd);
  while ((dir=readdir(dp))!=0) {
    if (!dir->d_ino) { alert("d_ino?"); continue; }
    switch (dir->d_type) {
      case DT_DIR:
        if (strcmp(dir->d_name,".")) { // current dir not listed
          if (dir_nr==d_max-1)
            dir_names=re_alloc(dir_names,d_max);
          dir_names[++dir_nr]=add_slash(dir->d_name,'/');
        }
        break;
      case DT_REG:
      case DT_LNK:
        if (file_nr==f_max-1)
          f_names=re_alloc(f_names,f_max);
        f_names[++file_nr]= dir->d_type==DT_LNK ? add_slash(dir->d_name,'@') : strdup(dir->d_name);
        break;
    }
  }
  closedir(dp);
  if (dir_nr>=0)
    qsort(dir_names,dir_nr+1,sizeof(char*),compare_fn);
  if (file_nr>=0)
    qsort(f_names,file_nr+1,sizeof(char*),compare_fn);
  return true;
}

bool FileChooser::fill_dname_array(int &dir_nr) {
  DIR *dp;
  dirent *dir;
  dir_nr=-1;
  const char *wd=getcwd(0,0);
  if (!(dp=opendir(wd))) {
    alert("wdir not accessable");
    return false;
  }
  free((void*)wd);
  while ((dir=readdir(dp))!=0) {
    if (!dir->d_ino) { alert("d_ino?"); continue; }
    if (dir->d_type==DT_DIR) {
      if (strcmp(dir->d_name,".")) {
        if (dir_nr==d_max-1)
          dir_names=re_alloc(dir_names,d_max);
        dir_names[++dir_nr]=add_slash(dir->d_name,'/');
      }
    }
  }
  closedir(dp);
  if (dir_nr>=0)
    qsort(dir_names,dir_nr+1,sizeof(char*),compare_fn);
  return true;
}

void FileChooser::choose_file() {
  int nr;
  dname_nr=fname_nr=-1;
  for (int i=0;i<=buttons->d->butnr;++i)
    free((void*)buttons->d->but[i].label.str); // was assigned by malloc()
  buttons->reset();
  if (!fill_fname_array(dname_nr,fname_nr)) return;
  for (nr=0;nr<=dname_nr;++nr)
    buttons->add_rbut(dir_names[nr])->label.render_t=draw_blue_ttf;
  for (nr=0;nr<=fname_nr;++nr)
    buttons->add_rbut(f_names[nr]);
  sbar->value=0;
  int range=(dname_nr+fname_nr+3)*TDIST;
  if (range<win_h) sbar->hide();
  else { sbar->set_range(range); sbar->show(); }
  buttons->d->act_button=0;
  buttons->draw_blit_upd();
}

void FileChooser::working_dir() {
  int nr;
  dname_nr=-1;
  buttons->reset();
  if (!fill_dname_array(dname_nr)) return;
  for (nr=0;nr<=dname_nr;++nr)
    buttons->add_rbut(dir_names[nr])->label.render_t=draw_blue_ttf;
  sbar->value=0;
  int range=(dname_nr+1)*TDIST + 10;
  if (range<win_h) sbar->hide();
  else { sbar->set_range(range); sbar->show(); }
  buttons->d->act_button=0;
  buttons->draw_blit_upd();
}

void file_chooser(void (*callb)(const char* path,Id),Id id) {
  if (f_chooser) f_chooser->bgw->show();
  else f_chooser=new FileChooser();
  if (the_menu) { the_menu->mclose(); the_menu=0; }
  f_chooser->id=id;
  f_chooser->buttons->rb_cmd=FileChooser::rbw_fch_cmd;
  f_chooser->callback=callb;
  f_chooser->choose_file();
}

void working_dir(void (*callb)(const char* path,Id),Id id) {
  if (f_chooser) f_chooser->bgw->show();
  else f_chooser=new FileChooser();
  if (the_menu) { the_menu->mclose(); the_menu=0; }
  f_chooser->id=id;
  f_chooser->buttons->rb_cmd=FileChooser::rbw_wdir_cmd;
  f_chooser->callback=callb;
  f_chooser->working_dir();
}

void handle_events(SDL_Event *ev) {
   SDL_MouseButtonEvent *mbe=reinterpret_cast<SDL_MouseButtonEvent*>(ev);
/* does not work: motion events of right button get button nr 4 instead of SDL_BUTTON_RIGHT
   switch (mbe->button) { 
     case SDL_BUTTON_LEFT:case SDL_BUTTON_MIDDLE:case SDL_BUTTON_RIGHT: break;
     default: return;
   }
*/
   WinBase *wb;
   static Button *but=0;
   static DialogWin *dialog=0;
   static EditWin *edw=0;
   CheckBox *chb=0;
   static RExtButton *rxbw=0;
   static HSlider *hsl=0;
   static VSlider *vsl=0;
   static HVSlider *hvsl=0;
   static Dial *dial=0;
   static HScrollbar *hscr=0;
   static VScrollbar *vscr=0;
   static BgrWin *bgw;
   RButWin *rbw=0;
   switch (ev->type) {
     case SDL_MOUSEBUTTONDOWN: { 
         wb=topw->in_a_win(mbe->x,mbe->y);
         if (!wb) break;
         if ((but=dynamic_cast<Button*>(wb))!=0) {
             but->is_down=true;
             if (but->cmd) but->cmd(but);
             but->draw_blit_upd();
         }
         else if ((dialog=dynamic_cast<DialogWin*>(wb))!=0) {
           dialog->set_cursor(mbe->x);
           the_cursor.keep=true;
         }
         else if ((edw=dynamic_cast<EditWin*>(wb))!=0) {
           edw->set_cursor(mbe->x,mbe->y);
           the_cursor.keep=true;
         }
         else if ((chb=dynamic_cast<CheckBox*>(wb))!=0) {
           *chb->d=!*chb->d;
           chb->draw_blit_upd();
           if (chb->cmd) chb->cmd(chb);
         }
         else if ((hsl=dynamic_cast<HSlider*>(wb))!=0) {
           SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
         }
         else if ((vsl=dynamic_cast<VSlider*>(wb))!=0) {
           SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
         }
         else if ((hvsl=dynamic_cast<HVSlider*>(wb))!=0) {
           SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
         }
         else if ((dial=dynamic_cast<Dial*>(wb))!=0) {
           SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
           dial->calc_dialval(mbe->x - dial->tw_area.x,true); // init dial value
         }
         else if ((bgw=dynamic_cast<BgrWin*>(wb))!=0) {
           if (bgw->down_cmd) {
             bgw->init_gui();
             bgw->down_cmd(bgw,mbe->x-bgw->tw_area.x,mbe->y-bgw->tw_area.y,mbe->button);
           }
         }
         else if ((rbw=dynamic_cast<RButWin*>(wb))!=0) {
           RButton *rb=rbw->is_in_rbutton(mbe);
           if (rb) {
             int fire=1;
             if (rb==rbw->d->act_button) {
               fire=0;
               if (rbw->maybe_z)
                 rbw->d->act_button=0;
             }
             else
               rbw->d->act_button=rb;
             rbw->draw_blit_upd(); // if used in menu, draw before calling rb->cmd()
             if (rb->cmd) {
               int nr=rbw->d->act_button ? rb-rbw->d->but : -1;
               if (the_menu && !the_menu->sticky) { // mclose() can't be used here
                 Button *src=the_menu->src;
                 src->bgcol=src->parent->bgcol;
                 src->draw_blit_upd();
                 the_menu->buttons->hide();
               }
               rb->cmd(rbw,nr,fire);
               if (the_menu && !the_menu->sticky) {
                 delete the_menu->buttons;
                 the_menu->buttons=0;
                 the_menu=0;
               }
             }
           }
         }
         else if ((rxbw=dynamic_cast<RExtButton*>(wb))!=0) {
           ExtRButCtrl *ctr=rxbw->rxb_ctr;
           bool is_act = ctr->act_lbut==rxbw;
           if (is_act) {
             if (ctr->maybe_z) {
               RExtButton *rb=ctr->act_lbut;
               ctr->act_lbut=0;
               if (ctr->reb_cmd) {
                 ctr->reb_cmd(rxbw,false);
               }
               rb->draw_blit_upd();
             }
           }
           else {
             if (ctr->act_lbut) {
               RExtButton *rb=ctr->act_lbut;
               ctr->act_lbut=rxbw;
               if (ctr->reb_cmd) {
                 ctr->reb_cmd(rb,false);
               }
               rb->draw_blit_upd();
             }
             else ctr->act_lbut=rxbw;
             if (ctr->reb_cmd) ctr->reb_cmd(rxbw,true);
             rxbw->draw_blit_upd();
           }
         }
         else if ((hscr=dynamic_cast<HScrollbar*>(wb))!=0) {
           if (repeat.on)
             hscr->inc_value(repeat.direction);
           else {
             if (hscr->in_ss_area(mbe,&repeat.direction)) {
               if (repeat.on)
                 hscr->inc_value(repeat.direction);
               else {
                 repeat.ev=*mbe;
                 repeat.on=true;
               }
             }
             else {
               hscr->p0=mbe->x;
               SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
             }
           }
         }
         else if ((vscr=dynamic_cast<VScrollbar*>(wb))!=0) {
           if (repeat.on)
             vscr->inc_value(repeat.direction);
           else {
             if (vscr->in_ss_area(mbe,&repeat.direction)) {
               if (repeat.on)
                 vscr->inc_value(repeat.direction);
               else {
                 repeat.ev=*mbe;
                 repeat.on=true;
               }
             }
             else {
               vscr->p0=mbe->y;
               SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
             }
           }
           the_cursor.keep=true; // maybe used for edit window
         }
         if (the_menu && the_menu->buttons && rbw!=the_menu->buttons) {
           the_menu->mclose(); the_menu=0;
         }
         if (!the_cursor.keep)
           the_cursor.unset();
         //the_cursor.keep=false;
       }
       break;
     case SDL_MOUSEMOTION:
       if (hsl) {
         int val=*hsl->d;
         hsl->calc_hslval(mbe->x - hsl->tw_area.x);
         if (val!=*hsl->d) {
           if (hsl->cmd) hsl->cmd(hsl,1,false);
           hsl->draw_blit_upd();
         }
       }
       else if (vsl) {
         int val=*vsl->d;
         vsl->calc_vslval(mbe->y - vsl->tw_area.y);
         if (val!=*vsl->d) {
           if (vsl->cmd) vsl->cmd(vsl,1,false);
           vsl->draw_blit_upd();
         }
       }
       else if (hvsl) {
         Int2 val=*hvsl->d;
         hvsl->calc_hvslval(Int2(mbe->x - hvsl->tw_area.x,mbe->y - hvsl->tw_area.y));
         if (val!=*hvsl->d) {
           if (hvsl->cmd) hvsl->cmd(hvsl,1,false);
           hvsl->draw_blit_upd();
         }
       }
       else if (dial) {
         int val=*dial->d;
         dial->calc_dialval(mbe->x - dial->tw_area.x,false);
         if (val!=*dial->d) {
           if (dial->cmd) dial->cmd(dial,1,false);
           dial->draw_blit_upd();
         }
       }
       else if (bgw) {
         if (bgw->moved_cmd) {
           bgw->init_gui();
           bgw->moved_cmd(bgw,mbe->x-bgw->tw_area.x,mbe->y-bgw->tw_area.y,mbe->button);
         }
       }
       else if (hscr) {
         if (!repeat.on)
           hscr->calc_xpos(mbe->x);
       }
       else if (vscr) {
         if (!repeat.on)
           vscr->calc_ypos(mbe->y);
       }
       break;
     case SDL_ACTIVEEVENT: {
         SDL_ActiveEvent *acte=reinterpret_cast<SDL_ActiveEvent*>(ev);
         //say("active t=%d g=%d s=%d",acte->type,acte->gain,acte->state);
         if (acte->gain!=0 || acte->state!=SDL_APPMOUSEFOCUS) // leave window -> mouse motion disabled
           SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
       }
       break;
     case SDL_MOUSEBUTTONUP:
       SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
       if (but) {
         but->is_down=false;
         but->draw_blit_upd();
         the_menu=the_menu_tmp;
         the_menu_tmp=0;
         but=0;
       }
       else if (hsl) {
         if (hsl->cmd) hsl->cmd(hsl,1,true);
         hsl=0;
       }
       else if (vsl) {
         if (vsl->cmd) vsl->cmd(vsl,1,true);
         vsl=0;
       }
       else if (hvsl) {
         if (hvsl->cmd) hvsl->cmd(hvsl,1,true);
         hvsl=0;
       }
       else if (dial) {
         if (dial->cmd) dial->cmd(dial,1,true);
         dial=0;
       }
       else if (bgw) {
         if (bgw->up_cmd) {
           bgw->init_gui();
           bgw->up_cmd(bgw,mbe->x-bgw->tw_area.x,mbe->y-bgw->tw_area.y,mbe->button);
         }
         bgw=0;
       }
       else if (hscr) {
         repeat.on=false;
         hscr=0;
       }
       else if (vscr) {
         repeat.on=false;
         vscr=0;
       }
       the_cursor.keep=false;
       break;
     case SDL_VIDEOEXPOSE:
       //say("expose");
       topw->draw_blit_recur();
       SDL_Flip(topw->win);
       break;
     case SDL_KEYDOWN:
       if (the_cursor.diawd && the_cursor.diawd->cursor>=0) {
         if (the_cursor.diawd->handle_key(&ev->key.keysym)) // enter key pressed?
           the_cursor.diawd->dok();
       }
       else if (the_cursor.edwd) {
         the_cursor.edwd->handle_key(&ev->key.keysym);
       }
       else if (handle_kev)
         handle_kev(&ev->key.keysym,true);
       break;
     case SDL_KEYUP:
       if (!(the_cursor.diawd && the_cursor.diawd->cursor>=0) && !the_cursor.edwd)
         handle_kev(&ev->key.keysym,false);
       break;
     case SDL_VIDEORESIZE: {
         int dw=ev->resize.w - topw->tw_area.w,
             dh=ev->resize.h - topw->tw_area.h;
         if (abs(dw)>=10 || abs(dh)>=10) {
           topw->tw_area.w=ev->resize.w;
           topw->tw_area.h=ev->resize.h;
           topw->win=SDL_SetVideoMode(ev->resize.w, ev->resize.h,pixdepth,video_flag);
           if (handle_rsev)
             handle_rsev(dw,dh);
           else
             topw->draw_blit_recur();
           SDL_Flip(topw->win);
         }
       }
       break;
     default:
       printf("unexpected event %d\n",ev->type);
   }
}

static void def_handle_uev(int cmd,int par1,int par2) {
  printf("user event: %d\n",cmd);
}

int keep_alivefun(void* data) {
  while (!quit) {
    ++go_ticks; // no mutex
    SDL_Delay(50); 
    send_uev('go');
  }
  return 0;
}

static const char *title_string(WinBase *child) {
  const char *typ=0;
  static char name[50];
  if (child->title_str)
    snprintf(name,50,"\"%s\"",child->title_str);
  else if (child->id.id1)
    sprintf(name,"id=%s",id2s(child->id.id1));
  else name[0]=0;
  if (child==awin::bgr) typ="(alert win)";
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
  if (!typ) typ="?";
  static char ret[100];
  snprintf(ret,100,"%s %s",typ,name);
  return ret;
}

static void print_widget_hcy(int indent,WinBase *top) {
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

static void print_ontopw() {
  puts("\non-top widgets:");
  for (int i=0;i<=ontop_nr;++i) {
    OnTopWin *otw=ontop_win+i;
    const char *tit=title_string(otw->wb);
    printf("   %s",tit ? tit : "--");
    if (otw->wb->hidden) fputs(" (hidden)",stdout);
    putchar('\n');
  }
}

void print_h() {
  puts("top-win");
  print_widget_hcy(1,topw);
  print_ontopw();
  fflush(stdout);
}

void get_events() {
  topw->draw_blit_recur();
  SDL_Flip(topw->win);
  sdl_running=true;
  awin::check_alert();
  SDL_Event ev;
  SDL_CreateThread(keep_alivefun,0);
  while (!quit) {
    while (true) {
      SDL_mutexP(mtx);
      if (uev_queue.is_empty()) {
        SDL_mutexV(mtx);
        break;
      }
      int uev_dat[3];
      uev_queue.pop(uev_dat);
      SDL_mutexV(mtx);
      if (!handle_uev) handle_uev=def_handle_uev;
      if (uev_dat[0]=='go') {
        if (repeat.on && go_ticks%3==0)  // 3 * 50 ms
          handle_events(reinterpret_cast<SDL_Event*>(&repeat.ev));
      }
      else
        handle_uev(uev_dat[0],uev_dat[1],uev_dat[2]);
    }
    if (SDL_PollEvent(&ev)) {
      if (ev.type==SDL_QUIT) {
        quit=true; break;
      }
      handle_events(&ev);
    }
    else {
      SDL_mutexP(mtx);
      SDL_CondWait(cond,mtx);
      SDL_mutexV(mtx);
    }
  }
  sdl_quit(0);
}
