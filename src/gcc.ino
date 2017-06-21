#include "Nintendo.h"

// This section is for user inputted data to make the code personalized to the particular controller

// Enter values of cardinals here (be cognizant of signs)
#define N_NOTCH_X -0.0000
#define N_NOTCH_Y  0.0000

#define E_NOTCH_X  0.0000
#define E_NOTCH_Y -0.0000

#define S_NOTCH_X -0.0000
#define S_NOTCH_Y -0.0000

#define W_NOTCH_X -0.0000
#define W_NOTCH_Y -0.0000

// enter values for shield drop notches here
#define SW_NOTCH_X -0.0000
#define SW_NOTCH_Y -0.0000

#define SE_NOTCH_X  0.0000
#define SE_NOTCH_Y -0.0000

// once you have installed the mod, upload the code and go to live analog inputs display
// hold dpad down for 3 seconds to turn off the mod then record the notch values
// doing the steps in this order ensures the most accurate calibration

// to return to a stock controller hold dpad down for 3 seconds
// to set to dolphin mode hold dpad up for 3 seconds

static CGamecubeController s_controller(0); // sets RX0 on arduino to read data from controller
static CGamecubeConsole s_console(1);       // sets TX1 on arduino to write data to console
static Gamecube_Report_t s_gcc;             // structure for controller state
static Gamecube_Data_t s_data;

static bool s_shield, s_dolphin, s_off, s_cal = 1, s_button;
static int8_t s_ax, s_ay, s_cx, s_cy;
static float s_r, s_deg, s_cr;
static uint8_t s_cycles = 3;
static uint16_t s_mode;
static uint32_t s_n, s_c;

static struct
{
    int8_t ax, ay, cx, cy;
    uint8_t l, r;
} s_ini;

static struct
{
    float n, e, eh, el, s, w, se, sw;
} s_g;

static struct
{
    uint8_t db : 5;
    uint8_t cr : 5;
} s_buf;

static struct
{
    bool u, d, l, r;
} s_perf;

static struct
{
    uint8_t l, r;
} s_ls;

static void mods()          // to remove mods delete any lines that you do not want here
{
    angles_fixed();   // reallocates angles properly based on the given cardinal notches
    perfect_angles(); // reduces deadzone of cardinals and gives steepest/shallowest angles when on or near the gate
    max_vectors();    // snaps sufficiently high cardinal inputs to vectors of 1.0 magnitude of analog stick and c stick
    shield_drops();   // gives an 8 degree range of shield dropping centered on SW and SE gates
    backdash();       // fixes dashback by imposing a 1 frame buffer upon tilt turn values
    backdash_ooc();   // allows more leniency for dash back out of crouch
    dolphin_fix();    // ensures close to 0 values are reported as 0 on the sticks to fix dolphin calibration and fixes poll speed issues
    no_code();        // function to disable all code if dpad down is held for 3 seconds (unplug controller to reactivate)
} // more mods to come!

static void angles_fixed()
{
    if (s_deg > s_g.el && s_deg < s_g.n)
        s_deg = map(s_deg, s_g.el, s_g.n, 0, 90);
    else if (s_deg > s_g.n && s_deg < s_g.w)
        s_deg = map(s_deg, s_g.n, s_g.w, 90, 180);
    else if (s_deg > s_g.w && s_deg < s_g.s)
        s_deg = map(s_deg, s_g.w, s_g.s, 180, 270);
    else if (s_deg > s_g.s && s_deg < s_g.eh)
        s_deg = map(s_deg, s_g.s, s_g.eh, 270, 360);
    else
        s_deg = map(s_deg - s_g.eh, s_g.el, s_g.n, 0, 90);
    s_perf.u = s_deg > 73 && s_deg < 107;
    s_perf.d = s_deg > 254 && s_deg < 287;
    s_perf.l = s_deg > 163 && s_deg < 197;
    s_perf.r = s_deg > 343 || s_deg < 17;
    s_gcc.xAxis = 128 + s_r * cos(s_deg / 57.3);
    s_gcc.yAxis = 128 + s_r * sin(s_deg / 57.3);
}

static void perfect_angles()
{
    if (s_r > 75)
    {
        if (s_perf.u)
        {
            s_gcc.xAxis = (s_ax > 0) ? 151 : 105;
            s_gcc.yAxis = 204;
        }
        if (s_perf.r)
        {
            s_gcc.yAxis = (s_ay > 0) ? 151 : 105;
            s_gcc.xAxis = 204;
        }
        if (s_perf.d)
        {
            s_gcc.xAxis = (s_ax > 0) ? 151 : 105;
            s_gcc.yAxis = 52;
        }
        if (s_perf.l)
        {
            s_gcc.yAxis = (s_ay > 0) ? 151 : 105;
            s_gcc.xAxis = 52;
        }
    }
}

static void max_vectors()
{
    if (s_r > 75)
    {
        if (arc(s_g.n) < 6)
        {
            s_gcc.xAxis = 128;
            s_gcc.yAxis = 255;
        }
        if (arc(s_g.e) < 6)
        {
            s_gcc.xAxis = 255;
            s_gcc.yAxis = 128;
        }
        if (arc(s_g.s) < 6)
        {
            s_gcc.xAxis = 128;
            s_gcc.yAxis = 1;
        }
        if (arc(s_g.w) < 6)
        {
            s_gcc.xAxis = 1;
            s_gcc.yAxis = 128;
        }
    }
    if (abs(s_cx) > 75 && abs(s_cy) < 23)
    {
        s_gcc.cxAxis = (s_cx > 0) ? 255 : 1;
        s_gcc.cyAxis = 128;
    }
    if (abs(s_cy) > 75 && abs(s_cx) < 23)
    {
        s_gcc.cyAxis = (s_cy > 0) ? 255 : 1;
        s_gcc.cxAxis = 128;
    }
}

static void shield_drops()
{
    s_shield = s_gcc.l || s_gcc.r || s_ls.l > 74 || s_ls.r > 74 || s_gcc.z;
    if (s_shield)
    {
        if (s_ay < 0 && s_r > 72)
        {
            if (arc(s_g.sw) < 4)
            {
                s_gcc.yAxis = 73;
                s_gcc.xAxis = 73;
            }
            if (arc(s_g.se) < 4)
            {
                s_gcc.yAxis = 73;
                s_gcc.xAxis = 183;
            }
        }
    }
}

static void backdash()
{
    s_button = s_gcc.a || s_gcc.b || s_gcc.x || s_gcc.y || s_gcc.z || s_gcc.l || s_gcc.r || s_ls.l > 74 || s_ls.r > 74;
    if (abs(s_ay) < 23 && !s_button)
    {
        if (abs(s_ax) < 23)
            s_buf.db = s_cycles;
        if (s_buf.db > 0)
        {
            s_buf.db--;
            if (abs(s_ax) < 64)
                s_gcc.xAxis = 128+s_ax*(abs(s_ax) < 23);
        }
    }
    else
        s_buf.db = 0;
}

static void backdash_ooc()
{
    if (s_ay < 23)
    {
        if (s_ay < -49)
            s_buf.cr = s_cycles;
        if (s_buf.cr > 0)
        {
            s_buf.cr--;
            if (s_ay > -50)
                s_gcc.yAxis = 78;
        }
    }
    else
        s_buf.cr = 0;
}

static void dolphin_fix()
{
    if (s_r < 8)
    {
        s_gcc.xAxis = 128;
        s_gcc.yAxis = 128;
    }
    if (mag(s_cx, s_cy) < 8)
    {
        s_gcc.cxAxis = 128;
        s_gcc.cyAxis = 128;
    }
    if (s_gcc.dup && s_mode < 1500)
        s_dolphin = s_dolphin || (s_mode++ > 1000);
    else
        s_mode = 0;
    s_cycles = 3 + (16*s_dolphin);
}

static void no_code()
{
    if (s_gcc.ddown)
    {
        if (s_n == 0)
            s_n = millis();
        s_off = s_off || (millis() - s_n > 500);
    }
    else
        s_n = 0;
}

static void recalibrate()
{
    if (s_cal)
    {
        s_cal = s_gcc.x && s_gcc.y && s_gcc.start;
        s_ini.ax = s_gcc.xAxis - 128; // gets offset from analog stick in neutral
        s_ini.ay = s_gcc.yAxis - 128;
        s_ini.cx = s_gcc.cxAxis - 128; // gets offset from c stick in neutral
        s_ini.cy = s_gcc.cyAxis - 128;
        s_ini.l = s_gcc.left; // gets offset from analog triggers in neutral
        s_ini.r = s_gcc.right;
    }
    else if (s_gcc.x && s_gcc.y && s_gcc.start)
    {
        if (s_c == 0)
            s_c = millis();
        s_cal = millis() - s_c > 500;
    }
    else
        s_c = 0;
}

static void calibration()
{
    s_ax = constrain(s_gcc.xAxis  - 128 - s_ini.ax, -128, 127); // offsets from neutral position of analog stick x axis
    s_ay = constrain(s_gcc.yAxis  - 128 - s_ini.ay, -128, 127); // offsets from neutral position of analog stick y axis
    s_cx = constrain(s_gcc.cxAxis - 128 - s_ini.cx, -128, 127); // offsets from neutral position of c stick x axis
    s_cy = constrain(s_gcc.cyAxis - 128 - s_ini.cy, -128, 127); // offsets from neutral position of c stick y axis
    s_r = mag(s_ax, s_ay); // obtains polar coordinates for analog stick
    s_deg = ang(s_ax, s_ay);
    s_cr = mag(s_cx, s_cy);                               // obtains magnitude of c stick value
    s_ls.l = constrain(s_gcc.left  - s_ini.l, 0, 255);    // fixes left trigger calibration
    s_ls.r = constrain(s_gcc.right - s_ini.r, 0, 255);    // fixes right trigger calibration
    s_gcc.left  = s_ls.l; // sets proper analog shield values
    s_gcc.right = s_ls.r;
    s_gcc.xAxis  = 128 + s_ax; // reports analog stick values
    s_gcc.yAxis  = 128 + s_ay;
    s_gcc.cxAxis = 128 + s_cx; // reports c stick values
    s_gcc.cyAxis = 128 + s_cy;
    recalibrate(); // allows holding x+y+start for 3 seconds to recalibrate
}

static float ang(float x, float y) { return atan2(y, x)*57.3 + 360*(y < 0); }        // returns angle in degrees when given x and y components
static float mag(int8_t x, int8_t y) { return sqrt(sq(x) + sq(y)); }                     // returns vector magnitude when given x and y components
static bool  mid(float val, float n1, float n2) { return val > n1 && val < n2; }     // returns whether val is between n1 and n2
static float arc(float val) { return abs(180 - abs(abs(s_deg-val) - 180)); }         // returns length of arc between the s_deg and val
static int   dis(float val) { return abs(fmod(val, 90) - 90*(fmod(val, 90) > 45)); } // returns how far off the given angle is from a cardinal
static float map(long val, float in, float ix, float on, float ox) { return (val-in)*(ox-on)/(ix-in)+on; }

void setup()
{
    s_gcc.origin = s_gcc.errlatch = s_gcc.high1 = s_gcc.errstat = 0; // init values
    s_g.n = ang(N_NOTCH_X, N_NOTCH_Y);          // calculates angle of N notch
    s_g.e = ang(E_NOTCH_X, E_NOTCH_Y);          // calculates angle of E notch
    s_g.s = ang(S_NOTCH_X, S_NOTCH_Y);          // calculates angle of S notch
    s_g.w = ang(W_NOTCH_X, W_NOTCH_Y);          // calculates angle of W notch
    s_g.sw = ang(SW_NOTCH_X, SW_NOTCH_Y);       // calculates angle of SW notch
    s_g.se = ang(SE_NOTCH_X, SE_NOTCH_Y);       // calculates angle of SE notch
    s_g.el = s_g.e - 360*(s_g.e > 180);  // gets east gate in 2 notations
    s_g.eh = s_g.e + 360*(s_g.e < 180);
    s_controller.read();             // reads controller once for calibration
    s_gcc = s_controller.getReport();
    recalibrate();                                           // calibrates the controller for initial plug in
}

void loop()
{
    s_controller.read();                        // reads the controller
    s_data = defaultGamecubeData;               // this line is necessary for proper rumble
    s_gcc = s_controller.getReport();           // gets a report of the controller read
    calibration();                              // fixes normal calibration
    recalibrate();                              // allows resetting with x+y+start
    if (!s_off)
        mods();                                 // implements all the mods (remove this to unmod the controller)
    s_data.report = s_gcc;
    s_console.write(s_data);                        // sends controller data to the console
    s_controller.setRumble(s_data.status.rumble);   // allows for rumble
}
