//This section is for user inputted data to make the code personalized to the particular controller

//enter values of cardinals here (be cognizant of signs)
#define n__notch_x_value -0.0000
#define n__notch_y_value  0.0000
 
#define e__notch_x_value  0.0000
#define e__notch_y_value -0.0000
 
#define s__notch_x_value -0.0000
#define s__notch_y_value -0.0000
 
#define w__notch_x_value -0.0000
#define w__notch_y_value -0.0000

//enter values for shield drop notches here
#define sw_notch_x_value -0.0000
#define sw_notch_y_value -0.0000
 
#define se_notch_x_value  0.0000
#define se_notch_y_value -0.0000

/*
once you have installed the mod, upload the code and go to live analog inputs display
hold dpad down for 3 seconds to turn off the mod then record the notch values
doing the steps in this order ensures the most accurate calibration

to return to a stock controller hold dpad down for 3 seconds
to set to dolphin mode hold dpad up for 3 seconds
*/

#include "Nintendo.h"
CGamecubeController controller(0); //sets RX0 on arduino to read data from controller
CGamecubeConsole console(1);       //sets TX1 on arduino to write data to console
Gamecube_Report_t gcc;             //structure for controller state
Gamecube_Data_t data;
struct{int8_t ax, ay, cx, cy; uint8_t l, r;}ini;
bool shield, dolphin=0, off=0, cal=1, button;
struct{float n, e, eh, el, s, w, se, sw;}g;
struct{float q1, q2, q3, q4;}quad;
struct{uint8_t db:5, cr:5;}buf;
struct{uint8_t l, r;}ls;
float r, deg, cr, ref;
int8_t ax, ay, cx, cy;
uint8_t cycles=3;
uint16_t mode;
uint32_t n, c;

void mods(){       //to remove mods delete any lines that you do not want here
  anglesfixed();   //reallocates angles properly based on the given cardinal notches
  maxvectors();    //snaps sufficiently high cardinal inputs to vectors of 1.0 magnitude of analog stick and c stick 
  perfectangles(); //reduces deadzone of cardinals and gives steepest/shallowest angles when on or near the gate
  shielddrops();   //gives an 8 degree range of shield dropping centered on SW and SE gates
  backdash();      //fixes dashback by imposing a 1 frame buffer upon tilt turn values
  backdashooc();   //allows more leniency for dash back out of crouch
  dolphinfix();    //ensures close to 0 values are reported as 0 on the sticks to fix dolphin calibration and fixes poll speed issues
  nocode();        //function to disable all code if dpad down is held for 3 seconds (unplug controller to reactivate)
} //more mods to come!

void anglesfixed(){
  ref = deg;
  if((ref>g.el&&ref<g.n)||ref>g.eh){
    if(ref>180) ref=(ref-g.eh)*quad.q1;         
    else        ref=(ref-g.el)*quad.q1;
  }
  else if(ref>g.n&&ref<g.w) {ref=(ref-g.n)*quad.q2; ref+= 90;}
  else if(ref>g.w&&ref<g.s) {ref=(ref-g.w)*quad.q3; ref+=180;}
  else if(ref>g.s&&ref<g.eh){ref=(ref-g.s)*quad.q4; ref+=270;}
  if(r>1.0){
    gcc.xAxis = 128+r*cos(ref/57.3);
    gcc.yAxis = 128+r*sin(ref/57.3);
  }else{gcc.xAxis=128; gcc.yAxis=128;}
}

void maxvectors(){
  if(r>75){
    if(arc(g.n)<6){gcc.xAxis = 128; gcc.yAxis = 255;}
    if(arc(g.e)<6){gcc.xAxis = 255; gcc.yAxis = 128;}
    if(arc(g.s)<6){gcc.xAxis = 128; gcc.yAxis =   1;}
    if(arc(g.w)<6){gcc.xAxis =   1; gcc.yAxis = 128;}
  }
  if(abs(cx)>75&&abs(cy)<23){gcc.cxAxis = (cx>0)?255:1; gcc.cyAxis = 128;}
  if(abs(cy)>75&&abs(cx)<23){gcc.cyAxis = (cy>0)?255:1; gcc.cxAxis = 128;}
}

void perfectangles(){
  if(r>75){
    if(mid(arc(g.n),6,17.2)){gcc.xAxis = (deg<g.n)               ?151:105; gcc.yAxis = 204;}
    if(mid(arc(g.e),6,17.2)){gcc.yAxis = (deg-360*(deg>180)>g.el)?151:105; gcc.xAxis = 204;}
    if(mid(arc(g.s),6,17.2)){gcc.xAxis = (deg>g.s)               ?151:105; gcc.yAxis =  52;}
    if(mid(arc(g.w),6,17.2)){gcc.yAxis = (deg<g.w)               ?151:105; gcc.xAxis =  52;}
  }
}

void shielddrops(){
  shield = gcc.l||gcc.r||ls.l>74||ls.r>74||gcc.z;
  if(shield){
    if(ay<0&&r>72){
      if(arc(g.sw)<4){gcc.yAxis = 73; gcc.xAxis =  73;}
      if(arc(g.se)<4){gcc.yAxis = 73; gcc.xAxis = 183;}
    }
  }
}

void backdash(){
  button=gcc.a||gcc.b||gcc.x||gcc.y||gcc.z||gcc.l||gcc.r||ls.l>74||ls.r>74;
  if(abs(ay)<23&&!button){ if(abs(ax)<23)buf.db = cycles;
    if(buf.db>0){buf.db--; if(abs(ax)<64) gcc.xAxis = 128+ax*(abs(ax)<23);}
  }else buf.db = 0;
}

void backdashooc(){
  if(ay<23){
    if(ay<-49) buf.cr = cycles;
    if(buf.cr>0){buf.cr--; if(ay>-50) gcc.yAxis = 78;}
  }else buf.cr = 0;
}

void dolphinfix(){
  if(r<8)         {gcc.xAxis  = 128; gcc.yAxis  = 128;}
  if(mag(cx,cy)<8){gcc.cxAxis = 128; gcc.cyAxis = 128;}
  if(gcc.dup&&mode<1500) dolphin = dolphin||(mode++>1000);
  else mode = 0; cycles = 3 + (16*dolphin);
}

void nocode(){
  if(gcc.ddown){
    if(n == 0) n = millis();
    off = off||(millis()-n>500);
  }else n = 0;
}

void recalibrate(){
  if(cal){ cal = gcc.x&&gcc.y&&gcc.start;
    ini.ax = gcc.xAxis -128; ini.ay = gcc.yAxis -128; //gets offset from analog stick in nuetral   
    ini.cx = gcc.cxAxis-128; ini.cy = gcc.cyAxis-128; //gets offset from c stick in nuetral
    ini.l  = gcc.left;       ini.r  = gcc.right;      //gets offset from analog triggers in nuetral
  }else if(gcc.x&&gcc.y&&gcc.start){
    if(c == 0) c = millis();
    cal = millis()-cal>500;
  }else c = 0;
}

void calibration(){
  ax = constrain(gcc.xAxis -128-ini.ax,-128,127); //offsets from nuetral position of analog stick x axis
  ay = constrain(gcc.yAxis -128-ini.ay,-128,127); //offsets from nuetral position of analog stick y axis
  cx = constrain(gcc.cxAxis-128-ini.cx,-128,127); //offsets from nuetral position of c stick x axis
  cy = constrain(gcc.cyAxis-128-ini.cy,-128,127); //offsets from nuetral position of c stick y axis
  r  = mag(ax, ay); deg  = ang(ax, ay);           //obtains polar coordinates for analog stick
  cr = mag(cx, cy);                               //obtains magnitude of c stick value
  ls.l = constrain(gcc.left -ini.l,0,255);        //fixes left trigger calibration
  ls.r = constrain(gcc.right-ini.r,0,255);        //fixes right trigger calibration
  gcc.left = ls.l; gcc.right = ls.r;              //sets proper analog shield values   
  gcc.xAxis  = 128+ax; gcc.yAxis  = 128+ay;       //reports analog stick values
  gcc.cxAxis = 128+cx; gcc.cyAxis = 128+cy;       //reports c stick values
  recalibrate();                                  //allows holding x+y+start for 3 seconds to recalibrate
}

float ang(float x, float y){return atan(y/x)*57.3+180*(x<0)+360*(y<0&&x>0);} //returns angle in degrees when given x and y components
float mag(char  x, char  y){return sqrt(sq(x)+sq(y));}                       //returns vector magnitude when given x and y components
bool  mid(float val, float n1, float n2){return val>n1&&val<n2;}             //returns whether val is between n1 and n2
float arc(float val){return abs(180-abs(abs(deg-val)-180));}                 //returns length of arc between the deg and val
int   dis(float val){return abs(fmod(val,90)-90*(fmod(val,90)>45));}         //returns how far off the given angle is from a cardinal

void setup(){
  gcc.origin=0;gcc.errlatch=0;gcc.high1=0;gcc.errstat=0; //init values
  g.n  = ang(n__notch_x_value, n__notch_y_value);        //calculates angle of N notch
  g.e  = ang(e__notch_x_value, e__notch_y_value);        //calculates angle of E notch
  g.s  = ang(s__notch_x_value, s__notch_y_value)+360;    //calculates angle of S notch
  g.w  = ang(w__notch_x_value, w__notch_y_value);        //calculates angle of W notch
  g.sw = ang(sw_notch_x_value, sw_notch_y_value);        //calculates angle of SW notch
  g.se = ang(se_notch_x_value, se_notch_y_value);        //calculates angle of SE notch
  g.el = g.e-360*(g.e>180); g.eh = g.e+360*(g.e<180);    //gets east gate in 2 notations
  quad.q1 = (g.n-g.el)/90.0; quad.q2 = (g.w -g.n)/90.0;  //finds disparity of quadrants 1 and 2
  quad.q3 = (g.s- g.w)/90.0; quad.q4 = (g.eh-g.s)/90.0;  //finds disparity of quadrants 3 and 4 
  controller.read(); gcc = controller.getReport();       //reads controller once for calibration
  recalibrate();                                         //calibrates the controller for initial plug in
}

void loop(){
  controller.read();                        //reads the controller
  data = defaultGamecubeData;               //this line is necessary for proper rumble
  gcc = controller.getReport();             //gets a report of the controller read
  calibration(); recalibrate();             //fixes normal calibration and allows resetting with x+y+start
  if(!off) mods();                          //implements all the mods (remove this line to unmod the controller)
  data.report = gcc; console.write(data);   //sends controller data to the console
  controller.setRumble(data.status.rumble); //allows for rumble
}
