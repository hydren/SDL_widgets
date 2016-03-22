/*  
    Demo program for SDL-widgets-1.0
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

#include <stdio.h>
#include <SDL_widgets.h>
#include "dump-wave.h"
#include "templates.h"

TopWin *top_win;

ExtRButCtrl *tab_ctr;
BgrWin *waves_ctr,*bgw1,*bgw2, // waves
       *bouncy_ctr,*bgw3,      // bouncy
       *harm_ctr,*bgw4,        // harmonics
       *filt_ctr,*bgw5,        // filters
       *scope_win;
Button *play_but,
       *save,
       *wave_out;
CheckBox *freeze,
         *low_frict,
         *non_lin_down,
         *hard_attack,
         *open_ended,
         *randomize,
         *no_clip_harm,
         *log_scale;
HSlider *am_amount,*fm_amount,
        *ab_detune,
        *node_mass,*asym_xy,
        *harm_offset,*ampl_mult,*harm_freq,*harm_clip,*harm_dist,*harm_n_mode,
        *filt_mode,*filt_range,*filt_freq;
VSlider *filt_qfactor;
HVSlider *freqs_sl;
RButWin *mode_select,
        *bouncy_in,
        *test_filter;
TextWin *harm_info;
const int bg_w=128, // wave background width
          bg_h=50,  // wave background height/2
          filt_h=100, // filter display height 
          bncy_w=280, // bouncy bgr width
          harm_w=280, // harmonics bgr width
          filt_w=200, // filters bgr width
          SAMPLE_RATE=44100,
          scope_dim=bg_w,
          scope_h=30,
          wav_pnt_max=20, // points in waveforms
          harm_max=20;  // harmonics
int task,
    scope_buf1[scope_dim],scope_buf2[scope_dim],
    i_am_playing;
bool debug,
     write_wave; // write wave file?
SDL_cond *is_false=SDL_CreateCond(),
         *stop=SDL_CreateCond();
SDL_mutex *mtx=SDL_CreateMutex();

struct WavF;

namespace we {  // wave edit
  int state,
      lst_x,lst_y;
  WavF *wf;
  Point *pt;
  bool do_point(int x,int y,int but);
  int y2pt(int y) { return (y-bg_h)*20; }
  int pt2y(int y) { return y/20 + bg_h; }
}

struct Node {
  float x,nom_x,
        y,
        vx,
        vy,
        ax,
        ay,
        mass;
  bool fixed;
  void set(float _x, float _y, bool _fixed) {
    x=_x; y=_y; vx=0; vy=0; fixed=_fixed; 
  }
};

struct Spring {
  Node *a,
       *b;
  void set(Node *_a, Node *_b) {
    a=_a; b=_b;
  }
  Spring() { }
};

enum { eA,eB,eA_B,eB_am_A,eB_fm_A,eB_trig_A,  // wave display modes
       eString, eStringOEnd,         // bouncy mode
       eRunWaves, eRunBouncy, eRunHarmonics, eRunFilters, // task
       ePlaying, eDecaying,          // play modes
       eIdle, eMoving                // wave edit mode
     };

void do_atexit() { puts("make_waves: bye!"); }

struct WavF {  // waveform buffer
  short wform[128];
  Array<Point,wav_pnt_max> ptbuf;
  WavF() {
    ptbuf[0].set(0,0);
    ptbuf[1].set(bg_w/4,-400);
    ptbuf[2].set(bg_w*3/4,400);
    ptbuf[3].set(bg_w+1,0); // this is the guard
    ptbuf.lst=3;
  }
} wavf1,wavf2;

static int max(int a, int b) { return a>b ? a : b; }
static int minmax(int a, int x, int b) { return x>b ? b : x<a ? a : x; }
static float minmax(double a, double x, double b) { return x>b ? b : x<a ? a : x; }
static float min(double a, double b) { return a>b ? b : a; }
static float max(double a, double b) { return a<b ? b : a; }

bool we::do_point(int x,int y,int but) {
  for (int i=0;i<20;++i) {
    pt=&wf->ptbuf[i];
    //printf("x=%d pt=%d y=%d pt=%d\n",x,pt->x,y,pt2y(pt->y));
    switch (but) {
      case SDL_BUTTON_LEFT:
        if (pt->x >= x-3) {
          if (abs(pt->x-x)>3) {
            wf->ptbuf.insert(i,Point(x,y2pt(y)));
            return true;
          }
          if (abs(y-pt2y(pt->y))<3) {
            lst_x=x; lst_y=y;
            state=eMoving;
            SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
          }
          return false;
        }
        break;
      case SDL_BUTTON_MIDDLE:
        if (pt->x >= x-3) {
          if (abs(x-pt->x)<3 && abs(y- pt2y(pt->y))<3) {
            if (wf->ptbuf.lst==2) {
              alert("last 2 points can't be removed");
              return false;
            }
            wf->ptbuf.remove(i);
            return true;
          }
          return false;
        }
        break;
    }
  }
  return false;
}

void make_spline(
    int pdim,  // dim of xs (points x buffer)
    int wdim,  // dim of ys (point val buffer)
    Point *pnt, 
    short *out // waveform buffer
  ) {
  int i,j,
      i0,i2,i3,
      delta;
  float x,xi,xi0,xi2,xi3;
  for (i=0;i<pdim;++i) {
    i0= i==0 ? pdim-1 : i-1;
    i2= i==pdim-1 ? 0 : i+1;
    i3= i2==pdim-1 ? 0 : i2+1;
    delta= pnt[i2].x-pnt[i].x;
    if (delta<0) delta+=wdim;
    xi0=pnt[i0].y;
    xi=pnt[i].y;
    xi2=pnt[i2].y;
    xi3=pnt[i3].y;
    for (j=0;j<delta;++j) {
      x=float(j)/delta;
      // From: www.musicdsp.org/showone.php?id=49
      out[pnt[i].x+j]=int(
        ((((3 * (xi-xi2) - xi0 + xi3) / 2 * x) +
          2*xi2 + xi0 - (5*xi + xi3) / 2) * x +
          (xi2 - xi0) / 2) * x +
          xi);
    }
  }
}

void draw_wform(BgrWin *bgw,WavF *wf) { // supposed: bgw->parent = top_window
  bgw->clear();
  int i;
  for (i=0;i<wf->ptbuf.lst;++i) {  // wf->ptbuf.lst is a guard
    int x=wf->ptbuf[i].x,
        val=wf->ptbuf[i].y;
    aacircleColor(bgw->win,x,we::pt2y(val),2,0xff);
    if (x==0) aacircleColor(bgw->win,x+bg_w,we::pt2y(val),2,0xff);
  }
  int val2=we::pt2y(wf->wform[127]);
  for (i=0;i<bg_w;++i) {
    int val1=val2;
    val2=we::pt2y(wf->wform[i]);
    aalineColor(bgw->win,i,val1,i+1,val2,0xff);
  }
  bgw->blit_upd();
}

namespace we {
  void down(BgrWin* bgw,int x,int y,int but) {
    switch (bgw->id.id2) {
      case 1: wf=&wavf1; break;
      case 2: wf=&wavf2; break;
      default: alert("the_bgw?"); wf=&wavf1;
    }
    state=eIdle;
    if (do_point(x,y,but)) {
      make_spline(wf->ptbuf.lst,bg_w,wf->ptbuf.buf,wf->wform);
      draw_wform(bgw,wf);
    }
  }
  void move(BgrWin* bgw,int x,int y,int but) {
    if (state!=eMoving || (abs(x-lst_x)<3 && abs(y-lst_y)<3)) return;
    lst_x=x; lst_y=y;
    if (x<1 || x>=bg_w-1 || pt[-1].x>=x || pt[1].x<=x || y<0 || y>=bg_h*2) return;
    pt->set(x,y2pt(y));
    make_spline(wf->ptbuf.lst,bg_w,wf->ptbuf.buf,wf->wform);
    draw_wform(bgw,wf);
  }
  void up(BgrWin* bgw,int x,int y,int but) {
    state=eIdle;
    SDL_EventState(SDL_MOUSEMOTION,SDL_DISABLE);
  }
}

struct Scope {
  int pos,
      scope_start;
  bool display1,display2;
  void update(Sint16 *buffer,int i);
} scope;

struct Audio {
  SDL_Thread *audio_thread;
  Audio() {
    scope.scope_start=0;
  }
  ~Audio() {
    SDL_WaitThread(audio_thread,0);
  }
} *audio;

void waves_mode_cmd(RButWin*,int nr,int fire) {
  switch (nr) {
    case 0: case 3: case 4: case 5:
      scope.display1=true; scope.display2=false; break;
    case 1:
      scope.display2=true; scope.display1=false; break;
    case 2:
      scope.display1=scope.display2=true; break;
  }
  switch (nr) {
    case 0: case 1: case 5:
      am_amount->hide(); fm_amount->hide(); ab_detune->hide(); break;
    case 2:
      am_amount->hide(); fm_amount->hide(); ab_detune->show(); break;
    case 3:
      fm_amount->hide(); am_amount->show(); ab_detune->show(); break;
    case 4:
      am_amount->hide(); fm_amount->show(); ab_detune->show(); break;
  }
}

void bgr_clear(BgrWin *bgr) { bgr->clear(); }

struct Waves {
  float freq1,
        freq2,
        fm_val,
        am_val,
        am_bias,
        ab_det,
        ind_f1,
        ind_f2,
        ampl;
  const float fmult;
  int ind1,
      ind2;
  Waves();
  void audio_callback(Uint8 *stream, int len);
} *waves;

void Scope::update(Sint16 *buffer,int i) {
  if (scope_start<0) return;
  Sint16 *test_buf=buffer;
  if (!display1) ++test_buf; // then other channel is tested for startup
  if (scope_start==0) { // set by handle_user_event()
    if (i>=2 && test_buf[i-2]<=0 && test_buf[i]>0) {
      scope_start=true;
      pos=0;
    }
    return;
  }
  if (i%4==0) {
    if (pos<scope_dim) {
      scope_buf1[pos]=buffer[i]/400 + scope_h;
      scope_buf2[pos]=buffer[i+1]/400 + scope_h;
      ++pos;
    }
    else {
      scope_start=-1;
      send_uev('scop');
    }
  }
}

void dump_to_wavef(Sint16 *buffer,int len) {
  Sint16 buf[len/4];
  for (int i=0;i<len/4;++i) buf[i]=(buffer[i*2]+buffer[i*2+1])/2; // stereo -> mono
  if (!dump_wav((char*)buf,len/2)) write_wave=false;
}

static void stop_audio() {
  i_am_playing=0;
  SDL_CondSignal(stop);
}

void Waves::audio_callback(Uint8 *stream, int len) {
  float fr1=freq1*fmult,
        fr2=freq2*fmult*ab_det;
  int val1,val2;
  bool trigger;
  Sint16 *buffer=reinterpret_cast<Sint16*>(stream);
  int mode=mode_select->act_rbutnr();
  for (int i=0;i<len/2;i+=2) {
    if (i_am_playing==ePlaying) { if (ampl<10.) ampl+=0.01; }
    else if (i_am_playing==eDecaying) {
      if (ampl>0.) ampl-=0.0004;
      else
        stop_audio();
    }
    // wave B
    ind_f2+=fr2; ind2=int(ind_f2);

    if (ind2>=bg_w) {
      ind2-=bg_w; ind_f2-=bg_w;
      trigger=true;
    }
    else
      trigger=false;
    if (mode==eB_trig_A) val2=0;
    else val2=int(wavf2.wform[ind2]);

    // wave A
    if (mode==eB_trig_A) {
      if (trigger) ind_f1=0.;
      else ind_f1+=fr1;
      ind1=int(ind_f1);
      if (ind1>=bg_w) val1=0.;
      else val1=int(wavf1.wform[ind1]);
    }
    else {
      if (mode==eB_fm_A) 
        ind_f1+=fr1 + fm_val * val2/400.;
      else
        ind_f1+=fr1;
      ind1=int(ind_f1);
      if (ind1>=bg_w) { ind1-=bg_w; ind_f1-=bg_w; }
      else if (ind1<0) { ind1+=bg_w; ind_f1+=bg_w; }
      val1=int(wavf1.wform[ind1]);
      if (mode==eB_am_A) val1 *= am_bias + am_val * val2/400.;
      //if (mode==eB_am_A) val1 *= am_bias + am_val * val2/400. * (1.+val1/300.); // <-- with feedback
    }
    val1 *= ampl;
    val2 *= ampl;
    switch (mode) {
      case eA:
      case eB_am_A:
      case eB_fm_A:
      case eB_trig_A:
        buffer[i]=val1; buffer[i+1]=0; break;
      case eB:
        buffer[i]=0; buffer[i+1]=val2; break;
      case eA_B:
        buffer[i]=val1; buffer[i+1]=val2; break;
    }
    scope.update(buffer,i);
  }
  if (write_wave) dump_to_wavef(buffer,len);
}

void wav_audio_callback(void*, Uint8 *stream, int len) {
  waves->audio_callback(stream,len);
}

struct Noise {
  static const int NOISE=100000;
  float noisebuf[NOISE];
  float nval() { return (rand() & 0xff)-127.5; }
  Noise() {
    const int pre_dim=100;
    float b1=0.,b2=0.,
          white,
          pre_buf[pre_dim];
    for (int i=-pre_dim;i<NOISE;++i) {
      white=i<NOISE-pre_dim ?  nval() : pre_buf[i-NOISE+pre_dim];
      b1 = 0.98 * b1 + white * 0.4;
      b2 = 0.6 * b2 + white;
      if (i<0) pre_buf[i+pre_dim]=white;
      else noisebuf[i] = b1 + b2;
    }
  }
} noise;

struct Common {  // for Bouncy and PhysModel
  static const float
    nominal_friction = 0.9,
    radius = 10.,
    nominal_springconst = 0.05;
  static const int
    NN=9,  // nodes
    xdist=(bncy_w-40)/(NN-1);
  int mode, // eString, eStringOEnd
    pickup;
  float
    springconstant,
    mass,
    asym;
  Common():
      mode(eString),
      pickup(NN/3),
      springconstant(0.8*nominal_springconst),
      mass(5),
      asym(1.) {
  }
} com;

struct Mass_Spring {
  Node nodes[Common::NN];
  Spring springs[Common::NN-1];
  int
    n_ind;
  float
    friction,
    in_val,
    sync_val;
  Mass_Spring():
      n_ind(0),
      friction(com.nominal_friction),
      in_val(0.),
      sync_val(0.) {
    int i;
    for (i=0;i<Common::NN;++i) nodes[i].mass=com.mass;
    for (i = 0; i < Common::NN; i++) {
      nodes[i].nom_x=20+i*com.xdist;
      nodes[i].set(nodes[i].nom_x,bg_h + 30 * (i % 3),false); 
    }
    nodes[0].fixed = nodes[Common::NN-1].fixed = true;
    nodes[0].y = nodes[Common::NN-1].y = bg_h;
    for (i = 0; i < Common::NN-1; i++)
      springs[i].set(nodes+i, nodes+i+1);
  }
  void eval_model();
};

struct Bouncy:Mass_Spring {
  int down, selected;
  Node *selnode;
  SDL_Thread *bouncy_thread;
  Bouncy();
  void audio_callback(Uint8 *stream, int len);
} *bouncy;

struct PhysModel:Mass_Spring {
  PhysModel() {
    friction=1.-(1.-com.nominal_friction)/2200.;  // 44100 / 20 = 2205
  }
} *phys_model;

struct Sinus {
  static const int dim=500;
  float buf[dim];
  Sinus() {
    for (int i=0;i<dim;++i)
      buf[i]=5*sin(2*M_PI*i/dim);
  }
  float get(float &ind_f) { // ind_f: 0 -> dim
    int ind=int(ind_f);
    if (ind<0) {
      while (ind<0) ind+=dim;
      if (ind>=dim) { alert("Sinus: ind=%d",ind); return 0.; }
      ind_f+=float(dim);
    }
    else if (ind>=dim) {
      while (ind>=dim) ind-=dim;
      if (ind<0) { alert("Sinus: ind=%d",ind); return 0.; }
      ind_f-=float(dim);
    }
    return buf[ind];
  }
  float get(float &ind_f,float aa) { // ind_f: 0 -> dim
    int ind=int(ind_f);
    if (ind<0) {
      while (ind<0) ind+=dim;
      if (ind>=dim) { alert("Sinus: ind=%d",ind); return 0.; }
      ind_f+=float(dim);
    }
    else if (ind>=dim) {
      while (ind>=dim) ind-=dim;
      if (ind<0) { alert("Sinus: ind=%d",ind); return 0.; }
      ind_f-=float(dim);
    }
    float bb=(1.-aa)/dim;
    ind=int(aa*ind + bb*ind*ind); if (ind<0) ind+=dim; else if (ind>=dim) ind-=dim;
    return buf[ind];
  }
} sinus;
/*
static float clip2(float in) {
  if (in>0)
    return in>6 ? 3 : in - in*in/12;
  return in<-6 ? -3 : in + in*in/12;
}
*/
struct NoisySinus {
  static const int nr_pts=200,//56,  // points per sample
                   nr_samp=500,//12,
                   dim=nr_pts*nr_samp,
                   pre_dim=dim/2;
  static const float freqcut=2 * M_PI / nr_pts,
                     qres=0.05,
                     fdelta=1.01;
  float buf[dim],
        pre_buf[pre_dim];
  NoisySinus() {
    float white=0,output,
          d1=0,d2=0,d3=0,d4=0;
    const float fc1=freqcut/fdelta,  // flatter peak
                fc2=freqcut*fdelta;
    unsigned int seed=12345;
    for (int i=-pre_dim;i<dim;++i) { // such that end and begin of buf are matching
/* not better 
      if (i%(nr_pts/2)==0) {
        n4=fabs((float(rand())/RAND_MAX - 0.5) * 0.1);
        if (i%nr_pts==0) n4=-n4;
      }
*/
      if (i<dim-pre_dim) {
        if (i%(nr_pts/2)==0)
          white=(float(rand_r(&seed))/RAND_MAX - 0.5) * 0.2;
      }
      else {
        white=pre_buf[i-dim+pre_dim];
      }
      output=bp_section(fc1,d1,d2,white);
      output=bp_section(fc2,d3,d4,output);

      if (i<0)
        pre_buf[i+pre_dim]=white; 
      else
        buf[i]=output;
    }
  }
  float bp_section(const float fcut,float& d1,float& d2,const float white) {
    d2+=fcut*d1;                    // d2 = lowpass output
    float highpass=white-d2-qres*d1;
    d1+=fcut*highpass;              // d1 = bandpass output
    return d1;
  }
  float get(float &ind_f) { // ind_f: 0 -> dim
    int ind=int(ind_f);
    if (ind<0) {
      ind+=dim;
      if (ind<0 || ind>=dim) { alert("NoisySinus: ind=%d",ind); return 0.; }
      ind_f+=float(dim);
    }
    else if (ind>=dim) {
      ind-=dim;
      if (ind<0 || ind>=dim) { alert("NoisySinus: ind=%d",ind); return 0.; }
      ind_f-=float(dim);
    }
    return buf[ind];
  }
} noisy_sinus;

struct HLine {  // one harmonic
  int ampl,
      act_ampl;
  float offset;
  bool sel,
       info;
  float freq,
        ind_f;
  HLine() { reset(); }
  void reset() { ampl=act_ampl=0; offset=0.; sel=false; info=false; ind_f=0.; }
};

struct LoFreqNoise {
  float noise,  // -1. -> 1.
        d1,d2;  // filter state
  int cnt;
  LoFreqNoise():noise(0),d1(0),d2(0),cnt(-1) { }
};

namespace lfnoise {
  int div=10;
  const float qres=0.5;
  int n_mode=0;
  bool noisy=false;
  float n_mult=0.5,
        n_cutoff=0.3;
  const char* n_mname;
  LoFreqNoise lfn_buf[harm_max];
  float lp_section(const float fcut,float& d1,float& d2,const float white) {
    d2+=fcut*d1;                    // d2 = lowpass output
    float highpass=white-d2-qres*d1;
    d1+=fcut*highpass;              // d1 = bandpass output
    return d2;
  }
  void set_mode(int n) {
    n_mode=n;
    noisy=false;
    switch (n_mode) {
      case 0:
        n_mname="(no)";
        break;
      case 1: // noise hf freq mod
        n_mname="HF freq mod";
        n_mult=0.5;
        n_cutoff=0.3;
        div=10;
        break;
      case 2: // noise lf freq mod
        n_mname="LF freq mod";
        n_mult=2.;
        n_cutoff=0.03;
        div=50;
        break;
      case 3: // hf noise ampl mod
        n_mname="HF ampl mod";
        n_mult=0.5;
        n_cutoff=0.3;
        div=10;
        break;
      case 4: // lf noise ampl mod
        n_mname="LF ampl mod";
        n_mult=2.;
        n_cutoff=0.03;
        div=50;
        break;
      case 5: // noise distorsion mod
        n_mname="dist mod";
        n_mult=2.;
        n_cutoff=0.03;
        div=50;
        break;
      case 6:
        n_mname="noisy";
        noisy=true;
        break;
    }
  }
  void update(int har);
}

struct Harmonics {
  static const int
    ldist=harm_w/harm_max+1; // 280/20+1 = 15
  Array<HLine,harm_max> lines;
  int info_lnr;
  float sclip_limit,
        freq,
        ampl,
        aa;
  const float fmult,
              ns_fmult;
  Harmonics();
  void draw_line(int);
  void audio_callback(Uint8 *stream, int len);
  void print_info();
  int x2ind(int x) { return (x+ldist/2)/ldist; }

  int non_linear(int x) {
    const int level=10000;
    x *= 2/sclip_limit;
    if (x>0) {
      if (x>level) return level;
      return 2*(x-x*x/2/level);
    }
    else {
      if (x<-level) return -level;
      return 2*(x+x*x/2/level);
    }
  }
} *harm;

struct FilterTest {
  struct FilterBase *the_filter;
  int range,
      pos; // for audio
  float ampl,val; // for audio
  bool updating;
  float fq, // filter Q
        cutoff;
  FilterTest();
  void draw_fresponse();
  void audio_callback(Uint8 *stream, int len);
} *filt;

#include "filter-test.cpp"

void bnc_audio_callback(void*, Uint8 *stream, int len) {
  bouncy->audio_callback(stream,len);
}

void harm_audio_callback(void*, Uint8 *stream, int len) {
  harm->audio_callback(stream,len);
}

void filt_audio_callback(void*, Uint8 *stream, int len) {
  filt->audio_callback(stream,len);
}

void lfnoise::update(int har) {
  LoFreqNoise &lfn=lfn_buf[har];
  lfn.cnt=(lfn.cnt+1)%div;
  if (lfn.cnt==0) {
    float val= float(rand()) / RAND_MAX * 2. - 1.;
    lfn.noise=minmax(-1., n_mult * lp_section(n_cutoff,lfn.d1,lfn.d2,val), 1.);
    if (debug && lfn.noise>0.99) { printf("noise=%.2f\n",lfn.noise); fflush(stdout); }
  }
}

void Mass_Spring::eval_model() {
  int i;
  for (i = 0; i < Common::NN; i++) 
    nodes[i].ax = nodes[i].ay = 0;
  switch (bouncy_in->act_rbutnr()) {
    case 0:  // no input
      break;
    case 1: {  // pink noise
        const float ns=0.005;
        n_ind=(n_ind+1)%Noise::NOISE;
        int ny=(n_ind+100)%Noise::NOISE;
        nodes[0].x=nodes[0].nom_x + noise.noisebuf[n_ind]*ns;
        nodes[0].y=bg_h+noise.noisebuf[ny]*ns;
      }
      break;
    case 2: {  // feedback
        Node *nod=nodes+com.pickup;
        const float fb=0.3;
        nodes[0].x=minmax(-1.,(nod->x-nod->nom_x) * fb,1.) + nodes[0].nom_x;
        nodes[0].y=minmax(-1.,(nod->y-bg_h) * fb,1.) + bg_h;
      }
      break;
    case 3: { // not used
        in_val+=0.002;
        float sync=nodes[7].y-bg_h;
        if (sync_val<=0. && sync>0.) { // rising zero-crossing
          if (in_val>1.8) in_val=0;
        }
        else {
          if (in_val>2.) in_val=0;
        }
        sync_val=sync;
        float pulse= in_val<0.2 ? 2. : 0;
        nodes[0].x=pulse+nodes[0].nom_x;
        nodes[0].y=pulse+bg_h;
      }
      break;
  }
  for (i = 0; i < Common::NN-1; i++) {
    Spring *s = springs+i;
    float dx,dy;
    dx = s->b->x - s->a->x - com.xdist;
    dy = s->b->y - s->a->y;
    if (non_lin_down->value()) {
      float ddy = dy*dy/20;
      dy += dy>0 ? ddy : -ddy;
      float dx1 = dx*dx/20;
      dx += dx>0 ? dx1 : -dx1;
    }
    dx *= com.springconstant;
    dy *= com.springconstant;
    s->a->ax += dx / (s->a->mass/com.asym);
    s->a->ay += dy / (s->a->mass*com.asym);
    s->b->ax -= dx / (s->b->mass/com.asym);
    s->b->ay -= dy / (s->b->mass*com.asym);
  }
  Node *n;
  for (i = 1; i < Common::NN; i++) {
    n = nodes+i;
    if (n == bouncy->selnode || n->fixed)
      continue;
    n->vy = (n->vy + n->ay) * friction;
    n->vx = (n->vx + n->ax) * friction;
    n->x += n->vx;
    n->y += n->vy;

    float val=n->x - n->nom_x;
    const float limit=bg_h;
    if (val>limit) n->x -= (val-limit)*0.5;
    else if (val<-limit) n->x -= (val+limit)*0.5;
    val=n->y - bg_h;
    if (val>limit) n->y -= (val-limit)*0.5;
    else if (val<-limit) n->y -= (val+limit)*0.5;
  }
}

void Bouncy::audio_callback(Uint8 *stream, int len) {
  if (i_am_playing==eDecaying) { // set by play_cmd()
    i_am_playing=0;
    SDL_CondSignal(stop);        // audio_thread killed
  }
  Sint16 *buffer=reinterpret_cast<Sint16*>(stream);
  const float scale=300.;
  if (freeze->value())
    for (int i=0;i<len/2;++i) buffer[i]=0;
  else
    for (int i=0;i<len/2;i+=2) {
      phys_model->eval_model();
      Node *nod=phys_model->nodes+com.pickup;
      buffer[i]  =minmax(-30000,int((nod->y-bg_h) * scale),30000);
      buffer[i+1]=minmax(-30000,int((nod->x-nod->nom_x) * scale),30000);
      scope.update(buffer,i);
    }
  if (write_wave) dump_to_wavef(buffer,len);
}

void init_audio() {
  SDL_AudioSpec *ask=new SDL_AudioSpec,
                *got=new SDL_AudioSpec;
  ask->freq=SAMPLE_RATE;
  ask->format=AUDIO_S16SYS;
  ask->channels=2;
  ask->samples=1024;
  switch (task) {
    case eRunWaves: ask->callback=wav_audio_callback; break;
    case eRunBouncy: ask->callback=bnc_audio_callback; break;
    case eRunHarmonics: ask->callback=harm_audio_callback; break;
    case eRunFilters: ask->callback=filt_audio_callback; break;
    default: ask->callback=0;
  }
  ask->userdata=0;
  if ( SDL_OpenAudio(ask, got) < 0 ) {
     alert("Couldn't open audio: %s",SDL_GetError());
     exit(1);
  }
  //printf("samples=%d channels=%d freq=%d format=%d (LSB=%d) size=%d\n",
  //       got->samples,got->channels,got->freq,got->format,AUDIO_S16LSB,got->size);
  SDL_PauseAudio(0);
}

int play_threadfun(void* data) {
  if (SDL_GetAudioStatus()!=SDL_AUDIO_PLAYING)
    init_audio();
  SDL_mutexP(mtx);
  SDL_CondWait(stop,mtx);
  SDL_mutexV(mtx);
  SDL_CloseAudio();
  if (write_wave) {
    close_dump_wav();
    write_wave=false;
  }
  return 0;
}

void right_arrow(SDL_Surface *win,int par,int y_off) {
  trigonColor(win,6,6,12,10,6,14,0x000000ff);
}

void square(SDL_Surface *win,int par,int y_off) {
  rectangleColor(win,6,7,12,13,0xff0000ff);
}

void play_cmd(Button* but) {
  if (i_am_playing) {
    but->label.draw_cmd=right_arrow;
    i_am_playing=eDecaying; // then: audio_thread will be killed
    delete audio; audio=0;
    //i_am_playing=0; // sometimes audio_callback cannot do this
    wave_out->hide();
    wave_out->style.param=0; // blue background
    wave_out->draw();
  }
  else {
    i_am_playing=ePlaying;
    play_but->label.draw_cmd=square;
    wave_out->show();
    audio=new Audio();
    audio->audio_thread=SDL_CreateThread(play_threadfun,0);
  }
}

void save_cmd(Button*) {
  FILE *sav=fopen("x.mw","w");
  if (!sav) { alert("x.mw can't be written"); return; }
  fprintf(sav,"Format:1.0\n");

  fprintf(sav,"Waves\n");
  fprintf(sav,"  A freq:%d\n",freqs_sl->value().x);
  fprintf(sav,"  B freq:%d\n",freqs_sl->value().y);
  fprintf(sav,"  mode:%d\n",mode_select->act_rbutnr());
  fprintf(sav,"  AM amount:%d\n",am_amount->value());
  fprintf(sav,"  FM amount:%d\n",fm_amount->value());
  fprintf(sav,"  detune:%d\n",ab_detune->value());
  fprintf(sav,"  A:");
  for (int i=0;i<=wavf1.ptbuf.lst;++i)
    fprintf(sav,"%d,%d;",wavf1.ptbuf[i].x,wavf1.ptbuf[i].y);
  putc('\n',sav);
  fprintf(sav,"  B:");
  for (int i=0;i<=wavf2.ptbuf.lst;++i)
    fprintf(sav,"%d,%d;",wavf2.ptbuf[i].x,wavf2.ptbuf[i].y);
  putc('\n',sav);

  fprintf(sav,"Harmonics\n");
  fprintf(sav,"  freq:%d (%.1fHz)\n",harm_freq->value(),harm->freq);
  fprintf(sav,"  clip:%d\n",harm_clip->value());
  fprintf(sav,"  rand:%d\n",randomize->value());
  fprintf(sav,"  add_dist:%d\n",harm_dist->value());
  fprintf(sav,"  n_mode:%d\n",harm_n_mode->value());
  fprintf(sav,"  harm:");
  for (int i=1;i<harm_max;++i) {
    HLine *hl=&harm->lines[i];
    if (hl->ampl) fprintf(sav,"%d,%d,%.2f;",i,hl->act_ampl,hl->offset);
  }
  putc('\n',sav);
  fclose(sav);
}

void wave_out_cmd(Button* but) {
  if (init_dump_wav("out.wav",1,SAMPLE_RATE)) {
    write_wave=true;
    but->style.param=4;  // rose background
    but->draw_blit_upd();
  }
}

void freqs_cmd(HVSlider *sl,int fire,bool rel) {
   static const float
     C=260., // middle C
     G=1.5*C,
     freqs[10]={ C/4., G/4., C/2., G/2., C, G, C*2., G*2., C*4., G*4. };
   static int xval,yval;
   if (xval!=sl->value().x) {
     xval=sl->value().x;
     float freq=freqs[xval];
     set_text(sl->text_x,"%.0fHz/%.0fHz",freq,freqs[yval]);
     if (waves) waves->freq1=freq;
   }
   if (yval!=sl->value().y) {
     yval=sl->value().y;
     float freq=freqs[yval];
     set_text(sl->text_x,"%.0fHz/%.0fHz",freqs[xval],freq);
     if (waves) waves->freq2=freq;
   }
}

void amval_cmd(HSlider *sl,int fire,bool rel) {
   float amval[6]={  0.,0.1,0.2,0.5,1. ,2. },
         ambias[6]={ 1.,1. ,1. ,0.7,0.5,0. };
   set_text(sl->text,"%.1f",amval[sl->value()]);
   if (waves) {
     waves->am_val=amval[sl->value()];
     waves->am_bias=ambias[sl->value()];
   }
}

void fmval_cmd(HSlider* sl,int fire,bool rel) {
   float fmval[10]={ 0.,0.1,0.2,0.3,0.5,0.7,1.,1.5,2.,3. };
   set_text(sl->text,"%.1f",fmval[sl->value()]);
   if (waves)
     waves->fm_val=fmval[sl->value()];
}

void mass_spring_cmd(HSlider* sl,int fire,bool rel) { // val 0 - 7
  static const float m[8]={ 0.3,0.4,0.6,0.8,1.2,1.6,2.4,3.2 };
  float fval=m[sl->value()];
  com.mass=4./fval;
  if (bouncy)
    for (int i=0;i<Common::NN;++i) bouncy->nodes[i].mass=phys_model->nodes[i].mass=com.mass;
  com.springconstant=com.nominal_springconst*fval;
  set_text(sl->text,"%.1f",fval);
}

void asym_cmd(HSlider *sl,int fire,bool rel) { // val 0 - 8
  static const float as[9]={ 1/1.99, 1/1.5, 0.98, 0.99, 1., 1.01, 1.02, 1.5, 1.99 };
  com.asym = as[sl->value()];
  set_text(sl->text,"%.2f",as[sl->value()]);
}

void topw_disp() {
  top_win->clear(rp(0,0,300,17),cGrey,false);
  top_win->draw_raised(rp(0,17,top_win->tw_area.w,top_win->tw_area.h-17),top_win->bgcol,true);
  top_win->border(scope_win);
}

int bouncy_threadfun(void* data) {
  SDL_mutex *bouncy_mtx=SDL_CreateMutex();
  while (task==eRunBouncy) {
    SDL_mutexP(bouncy_mtx);
    SDL_CondWaitTimeout(is_false,bouncy_mtx,50);
    if (!freeze->value())
      bouncy->eval_model();
    send_uev('bdis');
  }
  SDL_DestroyMutex(bouncy_mtx);
  return 0;
}

void hide(int what) {
  switch (what) {
    case 'wavs':
      if (i_am_playing) { // audio_thread is running?
        stop_audio();
        delete audio; audio=0;
      }
      bgw1->hide(); bgw2->hide(); waves_ctr->hide();
      break;
    case 'bncy':
      if (i_am_playing) {
        stop_audio();
        delete audio; audio=0;
      }
      if (task==eRunBouncy) {
        task=0;
        SDL_WaitThread(bouncy->bouncy_thread,0);
      }
      bgw3->hide(); bouncy_ctr->hide();
      break;
    case 'harm':
      if (i_am_playing) { // audio_thread running?
        stop_audio();
        delete audio; audio=0;
      }
      bgw4->hide(); harm_ctr->hide();
      break;
    case 'filt':
      if (i_am_playing) { // audio_thread running?
        stop_audio();
        delete audio; audio=0;
      }
      bgw5->hide(); filt_ctr->hide();
      break;
  }
  scope_win->clear();
  if (sdl_running) {
    play_but->label.draw_cmd=right_arrow; // reset play_but->label
    play_but->draw_blit_upd();
  }
}

void extb_cmd(RExtButton* rb,bool is_act) { 
  if (!is_act)
    return;
  switch (rb->id.id1) {
    case 'wavs':
      if (task==eRunWaves) break;
      hide('bncy'); hide('harm'); hide('filt');
      task=eRunWaves;
      bgw1->show(); bgw2->show(); waves_ctr->show();
      break;
    case 'bncy':
      if (task==eRunBouncy) break;
      hide('wavs'); hide('harm'); hide('filt');
      task=eRunBouncy;
      bgw3->show(); bouncy_ctr->show();
      scope.display1=scope.display2=true;
      bouncy->bouncy_thread=SDL_CreateThread(bouncy_threadfun,0);
      if (!phys_model) phys_model=new PhysModel();
      break;
    case 'harm':
      if (task==eRunHarmonics) break;
      hide('wavs'); hide('bncy'); hide('filt');
      task=eRunHarmonics;
      scope.display1=true; scope.display2=false;
      bgw4->show(); harm_ctr->show();
      break;
    case 'filt':
      if (task==eRunFilters) break;
      hide('wavs'); hide('bncy'); hide('harm');
      task=eRunFilters;
      scope.display1=true; scope.display2=false;
      bgw5->show(); filt_ctr->show();
      break;
  }
}

void draw_bgw(BgrWin *wb) {
  switch (wb->id.id1) {
    case 'wavs':
      if (wb->id.id2==1) {
        make_spline(wavf1.ptbuf.lst,bg_w,wavf1.ptbuf.buf,wavf1.wform);
        draw_wform(bgw1,&wavf1);
      }
      else {
        make_spline(wavf2.ptbuf.lst,bg_w,wavf2.ptbuf.buf,wavf2.wform);
        draw_wform(bgw2,&wavf2);
      }
      break;
    case 'harm':
      harm->print_info();
      bgw4->clear();
      for (int i=1;i<harm_max;++i) harm->draw_line(i);
      break;
    case 'filt':
      // bgw5 has been drawn already
      break;
  }
}

void slider_cmd(HSlider* sl,int fire,bool rel) {
  switch (sl->id.id1) {
    case 'abdt': { // A/B detune
        static const float det[6]={ 1,1.003,1.005,1.007,1.01,1.02 };
        set_text(sl->text,"%.3f",det[sl->value()]);
        if (waves) waves->ab_det=det[sl->value()];
      }
      break;
    case 'hfre': {
        static const float fr[12]={ 20,30,40,60,80,120,160,240,320,480,640,960 };
        harm->freq=fr[sl->value()];
        set_text(sl->text,"%.0f",fr[sl->value()]);
        if (rel) harm->print_info();
      }
      break;
    case 'hoff': { // range: -6 -> 6
        static const float off_arr[13]= { -0.4, -0.2, -0.1, -0.04, -0.02, -0.01, 0, 0.01, 0.02, 0.04, 0.1, 0.2, 0.4 };
        float val=off_arr[sl->value()+6];
        for (int i=0;i<harm_max;++i) {
          if (harm->lines[i].sel) {
            harm->lines[i].offset=val;
            if (rel && harm->lines[i].info) harm->print_info();
          }
        }
        if (!sl->value()) set_text(harm_offset->text,"0");
        else set_text(sl->text,"%0.2f",val);
      }
      break;
    case 'amul': {
        static const float ampl_mult_arr[9]={ 0.2,0.4,0.6,0.8,1,1.2,1.5,2,3 };
        for (int i=0;i<harm_max;++i) {
          if (harm->lines[i].sel) {
            harm->lines[i].act_ampl=min(int(harm->lines[i].ampl * ampl_mult_arr[sl->value()]), bg_h*2);
            harm->draw_line(i);
            if (rel && harm->lines[i].info) harm->print_info();
          }
        }
        set_text(sl->text,"%0.1f",ampl_mult_arr[sl->value()]);
      }
      break;
    case 'hdis': {
        static const float a_arr[5]={ 1.,1.2,1.5,2,3 };
        harm->aa=a_arr[sl->value()];
        if (sl->value()) set_text(sl->text,"%0.2f",harm->aa-1.);
        else set_text(sl->text,"(no)");
      }
      break;
    case 'nmod':   // noise mode
      lfnoise::set_mode(sl->value());
      set_text(sl->text,"%s",lfnoise::n_mname);
      break;
    case 'clip': {
        static float lim[5]={ 0,2,1,0.7,0.5 };
        float val=lim[sl->value()];
        if (harm) harm->sclip_limit=val;
        if (sl->value()) set_text(sl->text,"%.2f",val);
        else set_text(sl->text,"(no)");
      }
      break;
    case 'fran': {
        static int range[]={ 1000,3000,10000,30000 };
        int rng=range[sl->value()];
        if (filt) filt->range=rng;
        if (rel && filt) filt->draw_fresponse();
        set_text(sl->text,"0-%dHz",rng);
      }
      break;    
    case 'ffre': {
        static int ffreq[13]={ 100,150,200,300,400,600,800,1200,1600,2400,3200,4800,6400 };
        int ffr=ffreq[sl->value()];
        if (filt) filt->cutoff=ffr;
        if (rel && filt) filt->draw_fresponse();
        set_text(sl->text,"%dHz",ffr);
      }
      break;    
    case 'fmod': {
        int fmod=sl->value();
        if (filt) {
          if (!filt->the_filter) { bgw5->clear(); bgw5->blit_upd(); break; }
          const char *mm= filt->the_filter->get_mode(fmod);
          if (mm) {
            set_text(sl->text,mm);
            if (rel || fire==2) {          // fire == 2: from test_filt_cmd()
              filt->the_filter->mode=fmod;
              filt->draw_fresponse();
            }
          }
          else {
            set_text(sl->text,"-");
            bgw5->clear(); bgw5->blit_upd();
          }
        }
      }
      break;
    default: puts("slider_cmd?");
  }
}

Waves::Waves():
    freq1(260.),freq2(1.5*freq1),
    fm_val(0.7),am_val(0.5),am_bias(1.),ab_det(1.),
    ind_f1(0),ind_f2(0.),ampl(0.),fmult(float(bg_w)/SAMPLE_RATE),
    ind1(0),ind2(0) {
  bgw1=new BgrWin(top_win, Rect(10,40,bg_w,bg_h*2),"A",draw_bgw,we::down,we::move,we::up,cWhite,Id('wavs',1));
  bgw2=new BgrWin(top_win, Rect(150,40,bg_w,bg_h*2),"B",draw_bgw,we::down,we::move,we::up,cWhite,Id('wavs',2));
  //make_spline(wavf1.ptbuf.lst,bg_w,wavf1.ptbuf.buf,wavf1.wform);
  //draw_wform(bgw1,&wavf1);
  //make_spline(wavf2.ptbuf.lst,bg_w,wavf2.ptbuf.buf,wavf2.wform);
  //draw_wform(bgw2,&wavf2);
  waves_ctr=new BgrWin(top_win, Rect(50,150,240,160),0,bgr_clear,0,0,0,cForeground);
  freqs_sl=new HVSlider(waves_ctr,1,Rect(10,20,86,92),Int2(0,0),Int2(9,9),"A/B frequency",freqs_cmd);
  freqs_sl->set_hvsval(Int2(4,5),1,false);
  am_amount=new HSlider(waves_ctr,1,Rect(10,130,60,0),0,5,"AM amount",amval_cmd);
  am_amount->hidden=true;
  am_amount->set_hsval(3,1,false);
  fm_amount=new HSlider(waves_ctr,1,Rect(10,130,80,0),0,9,"FM amount",fmval_cmd);
  fm_amount->hidden=true;
  fm_amount->set_hsval(5,1,false);
  ab_detune=new HSlider(waves_ctr,1,Rect(100,130,60,0),0,5,"A/B detune",slider_cmd,'abdt');

  mode_select=new RButWin(waves_ctr,0,Rect(135,20,90,6*TDIST+2),"Mode",false,waves_mode_cmd);
  mode_select->add_rbut("A");
  mode_select->add_rbut("B");
  mode_select->add_rbut("A and B");
  mode_select->add_rbut("B mods A, AM");
  mode_select->add_rbut("B mods A, FM");
  mode_select->add_rbut("B triggers A");
  waves_mode_cmd(0,0,false);
}

void bouncy_down(BgrWin *bgw,int x,int y,int but) {
  int i,
      nearest = -1;
  bouncy->down=0;
  for (i = 1; i < Common::NN; i++) {
    Node *n = bouncy->nodes+i;
    int dx = n->x - x, dy = n->y - y;
    if (sqrt((dx * dx) + (dy * dy)) < com.radius) {
      nearest = i;
      break;
    }
  }
  bouncy->selnode= nearest>=0 ? bouncy->nodes+nearest : 0;
  switch (but) {
  case SDL_BUTTON_LEFT:
    if (nearest>=0) {
      bouncy->down = 2;
      SDL_EventState(SDL_MOUSEMOTION,SDL_ENABLE);
    }
    break;
  case SDL_BUTTON_MIDDLE:
    if (nearest>=0) {
      bool fixed=bouncy->nodes[nearest].fixed;
      phys_model->nodes[nearest].fixed=bouncy->nodes[nearest].fixed= !fixed;
    }
    break;
  }
}

void bouncy_moved(BgrWin *bgw,int x,int y,int but) {
  if (bouncy->down == 2 && bouncy->selnode) {
    bouncy->selnode->x = x;
    bouncy->selnode->y = y;
    if (hard_attack->value())
      bouncy->selnode->vy = y<bg_h ? 10 : -10; 
  }
}

void bouncy_up(BgrWin *bgw,int x,int y,int but) {
  if (bouncy->selnode) {
    bouncy->selnode=0;
    for (int i=0;i<Common::NN;++i) {
      phys_model->nodes[i].x=bouncy->nodes[i].x;
      phys_model->nodes[i].y=bouncy->nodes[i].y;
    }
  }
  SDL_EventState(SDL_MOUSEMOTION,SDL_IGNORE);
}

void checkbox_cmd(CheckBox* chb) {
  switch (chb->id.id1) {
    case 'rand':
      if (chb->value())
        for (int i=1;i<harm_max;++i)
          harm->lines[i].freq=i * (1.+(float(rand())/RAND_MAX-0.5)/100.);
      else
        for (int i=1;i<harm_max;++i)
          harm->lines[i].freq=i;
      if (debug) {
        printf("harm:");
        for (int i=1;i<harm_max;++i) printf(" %.2f",harm->lines[i].freq);
        putchar('\n');
      }
      break;
    case 'logs':
      filt->draw_fresponse();
      break;
    case 'frez':
      if (!chb->value())
        for (int i=0;i<Common::NN;++i) {
          phys_model->nodes[i].x=bouncy->nodes[i].x;
          phys_model->nodes[i].y=bouncy->nodes[i].y;
        }
      break;
    case 'lofr':
      bouncy->friction= chb->value() ? phys_model->friction : com.nominal_friction;
      break;
    case 'oend': {
        Node &bn=bouncy->nodes[Common::NN-1],
        &pmn=phys_model->nodes[Common::NN-1];
        if (chb->value()) {
          com.mode=eStringOEnd;
          com.pickup=Common::NN-2;
          bn.fixed=pmn.fixed=false;
        }
        else {
          com.mode=eString;
          com.pickup=Common::NN/3;
          bn.fixed=pmn.fixed=true;
        }
      }
      break;
    default: alert("checkbox_cmd?");
  }
}

Bouncy::Bouncy():
    down(0),selected(0),selnode(0) {
  bgw3=new BgrWin(top_win,Rect(10,40,bncy_w,bg_h*2),"the string",0,bouncy_down,bouncy_moved,bouncy_up,cWhite);
  bouncy_ctr=new BgrWin(top_win,Rect(50,150,240,140),0,bgr_clear,0,0,0,cForeground);
  node_mass=new HSlider(bouncy_ctr,1,Rect(8,20,100,0),0,7,"spring/mass",mass_spring_cmd);
  node_mass->set_hsval(3,1,false);
  asym_xy=new HSlider(bouncy_ctr,1,Rect(8,58,100,0),0,8,"x/y asymmetry",asym_cmd);
  asym_xy->set_hsval(4,1,false);
  bouncy_in=new RButWin(bouncy_ctr,1,Rect(10,92,80,3*TDIST),"Input",false,0);
  bouncy_in->add_rbut("no input");
  bouncy_in->add_rbut("pink noise");
  bouncy_in->add_rbut("feedback");
  freeze=new CheckBox(bouncy_ctr,0,Rect(122,6,0,14),"freeze",checkbox_cmd,'frez');
  low_frict=new CheckBox(bouncy_ctr,0,Rect(122,22,0,14),"low friction",checkbox_cmd,'lofr');
  open_ended=new CheckBox(bouncy_ctr,0,Rect(122,38,0,14),"open ended",checkbox_cmd,'oend');
  non_lin_down=new CheckBox(bouncy_ctr,0,Rect(122,54,0,14),"non-linear",0);
  hard_attack=new CheckBox(bouncy_ctr,0,Rect(122,70,0,14),"hard attack",0);
}

void harm_down(BgrWin*,int x,int y,int but) {
  int amp,
      ind=harm->x2ind(x);
  HLine *lin;
  if (ind>0) {
    switch (but) {
      case SDL_BUTTON_LEFT:
        amp=2*bg_h-y;
        lin=&harm->lines[ind];
        if (amp<5) {
          lin->ampl=lin->act_ampl=0;
          lin->sel=false;
          lin->offset=0;
        }
        else
          lin->ampl=lin->act_ampl=amp;
        if (lin->info)
          harm->print_info();
        harm->draw_line(ind);
        break;
      case SDL_BUTTON_MIDDLE:
        harm->lines[harm->info_lnr].info=false;
        harm->draw_line(harm->info_lnr);
        harm->info_lnr=ind;
        harm->lines[ind].info=true;
        harm->draw_line(ind);
        harm->print_info();
        break;
      case SDL_BUTTON_RIGHT:
        if (harm->lines[ind].ampl) {
          harm->lines[ind].sel=!harm->lines[ind].sel;
          harm->draw_line(ind);
        }
        break;
    }
  }
}


void slider_cmd(VSlider* sl,int fire,bool rel) { // for vertical slider
  switch (sl->id.id1) {
    case 'fres': {
        static float filter_q[8]={ 1.4,1.0,0.6,0.4,0.3,0.2,0.14,0.1 };
        float fq=filter_q[sl->value()];
        if (rel && filt) {
          filt->fq=fq;
          filt->draw_fresponse();
        }
        set_text(filt_qfactor->text,"%.1f",1./fq);
      }
      break;
    default: puts("slider_cmd?");
  }
}

Harmonics::Harmonics():
    info_lnr(0),
    freq(260.),
    ampl(0.),
    aa(1.),
    fmult(float(Sinus::dim)/SAMPLE_RATE),
    ns_fmult(float(NoisySinus::nr_pts)/SAMPLE_RATE) {
  bgw4=new BgrWin(top_win,Rect(10,40,harm_w,bg_h*2),"harmonics",draw_bgw,harm_down,0,0,cWhite,'harm');
  harm_ctr=new BgrWin(top_win,Rect(50,150,240,160),0,bgr_clear,0,0,0,cForeground);
  harm_freq=new HSlider(harm_ctr,1,Rect(10,20,100,0),0,11,"frequency",slider_cmd,'hfre');
  harm_freq->value()=7;
  set_text(harm_freq->text,"240 Hz");
  harm_offset=new HSlider(harm_ctr,1,Rect(10,58,100,0),-6,6,"sel: freq offset",slider_cmd,'hoff');
  harm_offset->value()=0;
  set_text(harm_offset->text,"0");
  ampl_mult=new HSlider(harm_ctr,1,Rect(10,96,100,0),0,8,"sel: ampl mult",slider_cmd,'amul');
  ampl_mult->value()=4;
  set_text(ampl_mult->text,"1");
  harm_dist=new HSlider(harm_ctr,1,Rect(10,132,60,0),0,4,"distorsion",slider_cmd,'hdis');
  set_text(harm_dist->text,"(no)");
  harm_n_mode=new HSlider(harm_ctr,1,Rect(80,132,72,0),0,6,"noise mode",slider_cmd,'nmod');
  set_text(harm_n_mode->text,"(no)");
  harm_clip=new HSlider(harm_ctr,1,Rect(162,132,60,0),0,4,"soft clipping",slider_cmd,'clip');
  harm_clip->set_hsval(0,0,false);
  harm_info=new TextWin(harm_ctr,1,Rect(120,20,100,2*TDIST+4),4,"info");
  harm_info->bgcol=top_win->bgcol;
  randomize=new CheckBox(harm_ctr,0,Rect(120,66,0,14),"randomized freq",checkbox_cmd,'rand');
  no_clip_harm=new CheckBox(harm_ctr,0,Rect(120,84,0,14),"only clip base freq",0);

  for (int i=1;i<4;++i) lines[i].ampl=lines[i].act_ampl=bg_h;
  for (int i=1;i<harm_max;++i) lines[i].freq=i;
  lines[1].info=true;
  info_lnr=1;
}

void FilterTest::draw_fresponse() {
  bgw5->clear();
  if (!the_filter) return;
  updating=true;
  int freq,
      tmp,
      spos=0,
      xpos=1,
      delta_f=range * 2 / bgw5->tw_area.w;
  float tIn, tOut,
        sIn, sOut;
  the_filter->init(filt->cutoff,filt->fq,filt_qfactor->value());
  for (freq=0; xpos < bgw5->tw_area.w; freq+=delta_f,xpos+=2) {
    tIn=tOut=0;
    for (spos=0;spos<TESTSAMPLES;++spos) {
      sIn  = freq==0 ? 0.1 : sin((2 * M_PI * spos * freq) / SAMPLE_RATE);
      sOut = the_filter->getSample(sIn);
      if (fabs(sOut)>50.) { alert("filter out = %.1f",sOut); updating=false; return; }
      tIn+=fabs(sIn);
      tOut+=fabs(sOut);
    }
    if (log_scale->value())
      tmp=max(0,int(30*log(tOut / tIn))+20);
    else
      tmp=int(20. * tOut / tIn);
    if (tmp) {
      vlineColor(bgw5->win,xpos,max(0,bgw5->tw_area.h-tmp-1),bgw5->tw_area.h-1,0xff);
    }
  }
  bgw5->blit_upd();
  updating=false;
}

void test_filt_cmd(RButWin*,int nr,int fire) {
  filt->updating=true;
  delete filt->the_filter;
  filt->the_filter=0;
  switch (nr) {
    case 0: filt->the_filter=new Filter2(); break;
    case 1: filt->the_filter=new Biquad(); break;
    case 2: filt->the_filter=new PinkNoise(); break;
    case 3: filt->the_filter=new ThreePoint(); break;
  }
  filt_mode->set_hsval(0,2,true); // will call draw_fresponse()
}

FilterTest::FilterTest():
    the_filter(0),
    range(10000),
    pos(0),
    ampl(0.),val(0),
    updating(false),
    fq(0.6),cutoff(3200) {
  bgw5=new BgrWin(top_win,Rect(10,40,filt_w,filt_h),"amplitude",draw_bgw,0,0,0,cWhite,'filt');
  filt_ctr=new BgrWin(top_win,Rect(50,150,240,150),0,bgr_clear,0,0,0,cForeground);
  test_filter=new RButWin(filt_ctr,0,Rect(10,18,130,5*TDIST),"filter",true,test_filt_cmd);
  test_filter->add_rbut("state-var 24dB/oct");
  test_filter->add_rbut("biquad 12dB/oct");
  test_filter->add_rbut("lowpass 3dB/oct");
  test_filter->add_rbut("2,3,4-point");
  test_filter->set_rbutnr(0,0,false);
  filt_mode=new HSlider(filt_ctr,1,Rect(10,120,70,0),0,6,"mode",slider_cmd,'fmod');
  filt_mode->set_hsval(0,1,false);
  filt_range=new HSlider(filt_ctr,1,Rect(145,18,70,0),0,3,"range",slider_cmd,'fran');
  filt_range->set_hsval(2,1,false);
  filt_qfactor=new VSlider(filt_ctr,1,Rect(145,60,0,42),0,5,"filter Q",slider_cmd,'fres');
  filt_qfactor->set_vsval(2,1,false);
  log_scale=new CheckBox(filt_ctr,0,Rect(175,90,0,14),"log scale",checkbox_cmd,'logs');
  filt_freq=new HSlider(filt_ctr,1,Rect(90,120,140,0),0,12,"corner freq",slider_cmd,'ffre');
  filt_freq->set_hsval(10,1,false);
}

void Harmonics::audio_callback(Uint8 *stream, int len) {
  int i,
      har;
  bool noisy=lfnoise::noisy,
       no_clip_h=no_clip_harm->value();
  float val=0;
  Sint16 *buffer=reinterpret_cast<Sint16*>(stream);
  for (i=0;i<len/2;++i)
    buffer[i]=0;
  for (har=1;har<harm_max;++har) {
    if (!lines[har].ampl && har!=1) continue;
    float har1=lines[har].freq*(1.+lines[har].offset/ldist), // shifted harmonic
          add=freq*(noisy ? ns_fmult : fmult)*har1;
    for (i=0;i<len/2;i+=2) {
      if (noisy) {
        lines[har].ind_f+=add;
        val=noisy_sinus.get(lines[har].ind_f) * lines[har].act_ampl;  // can modify lines[i].ind_f
      }
      else {
        lfnoise::update(har);
        //float n_val=lfnoise::lfn_buf[1].noise; // range: -1 -> 1
        float n_val=lfnoise::lfn_buf[har].noise; // range: -1 -> 1
        switch (lfnoise::n_mode) {
          case 0: // noise: off
            lines[har].ind_f+=add;
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl;
            break;
          case 1: // noise: hf freq mod 
            lines[har].ind_f += add * (1. + n_val * 0.3);
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl;
            break;
          case 2: // noise: lf freq mod 
            lines[har].ind_f += add * (1. + n_val * 0.03);
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl;
            break;
          case 3: // noise: hf ampl mod
            lines[har].ind_f+=add;
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl * (n_val + 1.);
            break;
          case 4: // noise: lf ampl mod
            lines[har].ind_f+=add;
            val=sinus.get(lines[har].ind_f,aa) * lines[har].act_ampl * (n_val/* *0.5*/ + 1.);
            break;
          case 5: // noise: distorsion mod
            lines[har].ind_f+=add;
            val=sinus.get(lines[har].ind_f,aa * (1. + n_val * 0.5)) * lines[har].act_ampl;
            break;
        }
      }
      if (no_clip_h && har==1 && harm_clip->value()>0) val=non_linear(val*10)/10;
      buffer[i] += val;
    }
  }
  for (i=0;i<len/2;i+=2) {
    if (i_am_playing==ePlaying) { if (ampl<10.) ampl+=0.01; }
    else if (i_am_playing==eDecaying) {
      if (ampl>0.) ampl-=0.0004;
      else
        stop_audio();
    }
    if (!no_clip_h && harm_clip->value()>0)
      buffer[i]=non_linear(buffer[i] * ampl);
    else
      buffer[i]*=ampl;
    buffer[i+1]=buffer[i];
    scope.update(buffer,i);
  }
  if (write_wave) dump_to_wavef(buffer,len);
}

void FilterTest::audio_callback(Uint8 *stream, int len) {
  int i;
  const int period=SAMPLE_RATE/220,   // 200 Hz
            edge=period/50,           // = 4, rising/falling
            flat=period/5;            // uppper flat part of puls
  const float top=1.,
              bot=-0.5;
  Sint16 *buffer=reinterpret_cast<Sint16*>(stream);
  for (i=0;i<len/2;i+=2) {
    if (i_am_playing==ePlaying) { if (ampl<1.) ampl+=0.001; }
    else if (i_am_playing==eDecaying) {
      if (ampl>0.) ampl-=0.00004;
      else
        stop_audio();
    }
    else if (updating) {
      if (ampl>0.) ampl-=0.001;
    }
    if (pos==0) val=bot;
    else if (pos<edge) val=min(val+(top-bot)/edge,top);
    else if (pos<flat) val=top;
    else if (pos<flat+edge) val=max(val-(top-bot)/edge,bot);
    else val=bot;
    pos=(pos+1)%period;
    if (the_filter && !updating)
      buffer[i]=buffer[i+1]=int(ampl*the_filter->getSample(val)*7000);
    else
      buffer[i]=buffer[i+1]=0;
    scope.update(buffer,i);
  }
  if (write_wave) dump_to_wavef(buffer,len);
}

void Harmonics::draw_line(int ind) {
  Rect rect(ind*ldist-ldist/2,0,ldist,2*bg_h);
  bgw4->clear(&rect);
  Uint32 col;
  HLine *lin=lines+ind;
  static Uint32 cSel=calc_color(0x707070);
  if (lin->ampl) {
    int x=ind*ldist, // int(ind*ldist+lin->offset),
        h=lin->act_ampl;
    col=lin->sel ? cSel : fabs(lin->offset)>0.001 ? cBlue : cBlack;
    SDL_FillRect(bgw4->win,rp(x-1,bg_h*2-h,3,h),col);
  }
  if (lin->info)
    aacircleColor(bgw4->win,ind*ldist,3,2,0xf00000ff);
  vlineColor(bgw4->win,ind*ldist-ldist/2,0,bg_h*2,0x70ff70ff); // green
  bgw4->blit_upd(&rect);
}

void Harmonics::print_info() {
  char buf[100],*bp=buf;
  harm_info->reset();
  HLine *lin=lines+info_lnr;
  bp+=sprintf(bp,"harmonic: %.2f\n",info_lnr+lin->offset);
  bp+=sprintf(bp,"ampl: %.2f",lin->ampl/2./bg_h);
  if (lin->ampl!=lin->act_ampl)
    bp+=sprintf(bp,"* %.1f",float(lin->act_ampl)/lin->ampl);
  harm_info->add_text(buf,sdl_running);
}

void draw_curve(SDL_Surface *win,Rect *r,int *buf,int dim,Uint32 color) {
  int val2=buf[0];
  pixelColor(win,r->x,val2+r->y,color);
  for (int i=0;i<dim-1;++i) {
    int val1=val2;
    val2=buf[i+1];
    if (abs(val1-val2)<=1)
      pixelColor(win,i+1+r->x,val2+r->y,color);
    else
      lineColor(win,i+r->x,val1+r->y,i+1+r->x,val2+r->y,color);
  }
}


void handle_user_event(int cmd,int par1,int par2) {
  switch (cmd) {
    case 'scop': {
        Rect *sr=&scope_win->tw_area;
        top_win->clear(sr,cWhite,false);
        if (scope.display1) draw_curve(top_win->win,sr,scope_buf1,scope_dim,0xff);
        if (scope.display2) draw_curve(top_win->win,sr,scope_buf2,scope_dim,0x7000ff);
        scope.scope_start=0;
        top_win->upd(sr);
      }
      break;
    case 'bdis':
      if (task!=eRunBouncy)
        break;
      {
        int i;
        bgw3->clear();
        for (i = 0; i < Common::NN-1; i++) {
          Spring *s = bouncy->springs+i;
          lineColor(bgw3->win, s->a->x, s->a->y, s->b->x, s->b->y,0xff);
        }
        for (i = 0; i < Common::NN; i++) {
          Node *n = bouncy->nodes+i;
          if (n->fixed)
            boxColor(bgw3->win,n->x-5,n->y-10,n->x+5,n->y+10,0x00ff00ff);
          else {
            int rrx=int(sqrt(8 * n->mass/com.asym)),
                rry=int(sqrt(8 * n->mass*com.asym));
            filledEllipseColor(bgw3->win,n->x,n->y,rrx,rry,0xff6060ff);
          }
          if (i==com.pickup)
            filledCircleColor(bgw3->win,n->x,n->y,2,0xff);
        }
        bgw3->blit_upd();
      }
      break;
    default: puts("uev?");
  }
}

bool set_config(FILE *conf) {  // no drawing!
  char buf[20];
  int i,
      n1,n2;
  if (fscanf(conf,"Format:%20s\n",buf)!=1) { alert("Format missing"); return false; }
  if (strcmp(buf,"1.0")) { alert("%s: bad format (expected: 1.0)",buf); return false; }
  while (true) {
    if (1!=fscanf(conf,"%20s\n",buf)) break;
    if (!strcmp(buf,"Waves")) {
      if (1!=fscanf(conf," A freq:%d\n",&n1)) return false;
      if (1!=fscanf(conf," B freq:%d\n",&n2)) return false;
      freqs_sl->set_hvsval(Int2(n1,n2),1,false);
      if (1!=fscanf(conf," mode:%d\n",&n1)) return false;
      mode_select->set_rbutnr(n1,1,false);
      if (1!=fscanf(conf," AM amount:%d\n",&n1)) return false;
      am_amount->set_hsval(n1,1,false);
      if (1!=fscanf(conf," FM amount:%d\n",&n1)) return false;
      fm_amount->set_hsval(n1,1,false);
      if (1!=fscanf(conf," detune:%d\n",&n1)) return false;
      ab_detune->set_hsval(n1,1,false);
      if (0!=fscanf(conf," A:"));
      for (i=0;;++i) {
        if (2!=fscanf(conf,"%d,%d;",&n1,&n2)) return false;
        if (i>=wav_pnt_max) {
          alert("waves A: %d points (should be < %d)",i,wav_pnt_max);
          while (getc(conf)!='\n');
          break;
        }
        wavf1.ptbuf[i].x=n1; wavf1.ptbuf[i].y=n2;
        n1=getc(conf); if (n1=='\n') break; ungetc(n1,conf);
      }
      if (0!=fscanf(conf," B:"));
      for (i=0;;++i) {
        if (2!=fscanf(conf,"%d,%d;",&n1,&n2)) return false;
        wavf2.ptbuf[i].x=n1; wavf2.ptbuf[i].y=n2;
        n1=getc(conf); if (n1=='\n') break; ungetc(n1,conf);
      }
    }
    else if (!strcmp(buf,"Harmonics")) {
      char key[10];
      while (fscanf(conf," %[^:]%*c",key)==1) { // read upto-and-incl ':'
        if (!strcmp(key,"freq")) {
          if (fscanf(conf,"%d (%*s)\n",&n1)!=1) return false;
          harm_freq->set_hsval(n1,1,false);
        }
        else if (!strcmp(key,"clip")) {
          if (fscanf(conf,"%d\n",&n1)!=1) return false;
          harm_clip->set_hsval(n1,1,false);
        }
        else if (!strcmp(key,"add_dist")) {
          if (fscanf(conf,"%d\n",&n1)!=1) return false;
          harm_dist->set_hsval(n1,1,false);
        }
        else if (!strcmp(key,"n_mode")) {
          if (fscanf(conf,"%d\n",&n1)!=1) return false;
          harm_n_mode->set_hsval(n1,1,false);
        }
        else if (!strcmp(key,"rand")) {
          if (fscanf(conf,"%d\n",&n1)!=1) return false;
          randomize->set_cbval(n1,1,false);
        }
        else if (!strcmp(key,"harm")) {
          float f1;
          HLine *hl;
          for (i=0;i<harm_max;++i) {
            hl=&harm->lines[i];
            hl->ampl=hl->act_ampl=0; hl->offset=0;
          }
          for (i=1;;++i) {
            if (fscanf(conf,"%d,%d,%f;",&n1,&n2,&f1)!=3) return false;
            if (n1>=harm_max) {
              alert("harmonic %d (should be < %d)",n1,harm_max);
              while (getc(conf)!='\n');
              break;
            }
            if (n2) {
              hl=&harm->lines[n1];
              hl->ampl=hl->act_ampl=n2; hl->offset=f1;
            }
            n1=getc(conf); if (n1=='\n') break;
            ungetc(n1,conf);
          }
        }
        else { alert("unknown keyword: %s",key); return false; }
      }
    }
  }
  return true;
}

int main(int argc,char** argv) {
  const char* inf=0;
  for (int an=1;an<argc;an++) {
    if (!strcmp(argv[an],"-h")) {
      puts("This is make-waves, an audio test suite using the SDL-widgets toolkit");
      puts("Usage:");
      puts("  make_waves [<config-file>]");
      puts("  (the config-file can be created by this program)");
      puts("Option:");
      puts("  -db  - extra debug info");
      exit(1);
    }
    if (!strcmp(argv[an],"-db")) debug=true;
    else inf=argv[an];
  }
  top_win=new TopWin("Make Waves",Rect(100,100,300,390),SDL_INIT_VIDEO|SDL_INIT_AUDIO,0,topw_disp);
  top_win->bgcol=cBackground;
  handle_uev=handle_user_event;

  // tabs
  tab_ctr=new ExtRButCtrl(Style(0,cBackground),extb_cmd);
  tab_ctr->maybe_z=false;
  RExtButton *wrex=tab_ctr->add_extrbut(top_win,Rect(0,1,60,17),"waves",'wavs'),
             *brex=tab_ctr->add_extrbut(top_win,Rect(64,1,60,17),"bouncy",'bncy'),
             *hrex=tab_ctr->add_extrbut(top_win,Rect(128,1,64,17),"harmonics",'harm'),
             *frex=tab_ctr->add_extrbut(top_win,Rect(196,1,60,17),"filters",'filt');

  play_but=new Button(top_win,0,Rect(10,150,24,20),right_arrow,play_cmd);
  scope_win=new BgrWin(top_win,Rect(10,320,scope_dim,scope_h*2),"sound",bgr_clear,0,0,0,cGrey);
  save=new Button(top_win,1,Rect(145,320,0,0),"save settings (file x.mw)",save_cmd);
  wave_out=new Button(top_win,1,Rect(145,340,0,0),"write sound (out.wav)",wave_out_cmd);
  wave_out->hidden=true;

  waves=new Waves();
  bouncy=new Bouncy();
  harm=new Harmonics();
  filt=new FilterTest();
  test_filt_cmd(0,0,2); // needs filt

  const int startup=1;
  switch (startup) {
    case 1: tab_ctr->act_lbut=wrex; tab_ctr->reb_cmd(wrex,true); break; // task = eRunWaves
    case 2: tab_ctr->act_lbut=brex; tab_ctr->reb_cmd(brex,true); break; //        eRunBouncy
    case 3: tab_ctr->act_lbut=hrex; tab_ctr->reb_cmd(hrex,true); break; //        eHarmonics
    case 4: tab_ctr->act_lbut=frex; tab_ctr->reb_cmd(frex,true); break; //        eFilters
  }
  if (inf) {
    FILE *config=fopen(inf,"r");
    if (!config) alert("config file %s not opened",inf);
    else {
      if (!set_config(config)) {
        alert("problems reading %s",inf);
        fclose(config);
      }
      else
        fclose(config);
    }
  }
  get_events();
  return 0;
}
