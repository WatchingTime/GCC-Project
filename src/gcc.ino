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

CGamecubeController controller(0); // sets RX0 on arduino to read data from controller
CGamecubeConsole console(1);       // sets TX1 on arduino to write data to console
Gamecube_Report_t gcc;             // structure for controller state
Gamecube_Data_t data;

bool shield, dolphin, off, cal = 1, button;
int8_t ax, ay, cx, cy;
float r, deg, cr;
uint8_t cycles = 3;
uint16_t mode;
uint32_t n, c;

struct
{
    int8_t ax, ay, cx, cy;
    uint8_t l, r;
} ini;

struct
{
    float n, e, eh, el, s, w, se, sw;
} g;

struct
{
    uint8_t db : 5;
    uint8_t cr : 5;
} buf;

struct
{
    bool u, d, l, r;
} perf;

struct
{
    uint8_t l, r;
} ls;

static void mods()          // to remove mods delete any lines that you do not want here
{
    anglesfixed();   // reallocates angles properly based on the given cardinal notches
    perfectangles(); // reduces deadzone of cardinals and gives steepest/shallowest angles when on or near the gate
    maxvectors();    // snaps sufficiently high cardinal inputs to vectors of 1.0 magnitude of analog stick and c stick
    shielddrops();   // gives an 8 degree range of shield dropping centered on SW and SE gates
    backdash();      // fixes dashback by imposing a 1 frame buffer upon tilt turn values
    backdashooc();   // allows more leniency for dash back out of crouch
    dolphinfix();    // ensures close to 0 values are reported as 0 on the sticks to fix dolphin calibration and fixes poll speed issues
    nocode();        // function to disable all code if dpad down is held for 3 seconds (unplug controller to reactivate)
} // more mods to come!

static void anglesfixed()
{
    if (deg > g.el && deg < g.n)
        deg = map(deg, g.el, g.n, 0, 90);
    else if (deg > g.n && deg < g.w)
        deg = map(deg, g.n, g.w, 90, 180);
    else if (deg > g.w && deg < g.s)
        deg = map(deg, g.w, g.s, 180, 270);
    else if (deg > g.s && deg < g.eh)
        deg = map(deg, g.s, g.eh, 270, 360);
    else
        deg = map(deg-g.eh, g.el, g.n, 0, 90);
    perf.u = deg > 73 && deg < 107;
    perf.d = deg > 254 && deg < 287;
    perf.l = deg > 163 && deg < 197;
    perf.r = deg > 343 || deg < 17;
    gcc.xAxis = 128 + r * cos(deg / 57.3);
    gcc.yAxis = 128 + r * sin(deg / 57.3);
}

static void perfectangles()
{
    if (r > 75)
    {
        if (perf.u)
        {
            gcc.xAxis = (ax > 0) ? 151 : 105;
            gcc.yAxis = 204;
        }
        if (perf.r)
        {
            gcc.yAxis = (ay > 0) ? 151 : 105;
            gcc.xAxis = 204;
        }
        if (perf.d)
        {
            gcc.xAxis = (ax > 0) ? 151 : 105;
            gcc.yAxis = 52;
        }
        if (perf.l)
        {
            gcc.yAxis = (ay > 0) ? 151 : 105;
            gcc.xAxis = 52;
        }
    }
}

static void maxvectors()
{
    if (r > 75)
    {
        if (arc(g.n) < 6)
        {
            gcc.xAxis = 128;
            gcc.yAxis = 255;
        }
        if (arc(g.e) < 6)
        {
            gcc.xAxis = 255;
            gcc.yAxis = 128;
        }
        if (arc(g.s) < 6)
        {
            gcc.xAxis = 128;
            gcc.yAxis = 1;
        }
        if (arc(g.w) < 6)
        {
            gcc.xAxis = 1;
            gcc.yAxis = 128;
        }
    }
    if (abs(cx) > 75 && abs(cy) < 23)
    {
        gcc.cxAxis = (cx > 0) ? 255 : 1;
        gcc.cyAxis = 128;
    }
    if (abs(cy) > 75 && abs(cx) < 23)
    {
        gcc.cyAxis = (cy > 0) ? 255 : 1;
        gcc.cxAxis = 128;
    }
}

static void shielddrops()
{
    shield = gcc.l || gcc.r || ls.l > 74 || ls.r > 74 || gcc.z;
    if (shield)
    {
        if (ay < 0 && r > 72)
        {
            if (arc(g.sw) < 4)
            {
                gcc.yAxis = 73;
                gcc.xAxis = 73;
            }
            if (arc(g.se) < 4)
            {
                gcc.yAxis = 73;
                gcc.xAxis = 183;
            }
        }
    }
}

static void backdash()
{
    button = gcc.a || gcc.b || gcc.x || gcc.y || gcc.z || gcc.l || gcc.r || ls.l > 74 || ls.r > 74;
    if (abs(ay) < 23 && !button)
    {
        if (abs(ax) < 23)
            buf.db = cycles;
        if (buf.db > 0)
        {
            buf.db--;
            if (abs(ax) < 64)
                gcc.xAxis = 128+ax*(abs(ax) < 23);
        }
    }
    else
        buf.db = 0;
}

static void backdashooc()
{
    if (ay < 23)
    {
        if (ay < -49)
            buf.cr = cycles;
        if (buf.cr > 0)
        {
            buf.cr--;
            if (ay > -50)
                gcc.yAxis = 78;
        }
    }
    else
        buf.cr = 0;
}

static void dolphinfix()
{
    if (r < 8)
    {
        gcc.xAxis = 128;
        gcc.yAxis = 128;
    }
    if (mag(cx, cy) < 8)
    {
        gcc.cxAxis = 128;
        gcc.cyAxis = 128;
    }
    if (gcc.dup && mode < 1500)
        dolphin = dolphin || (mode++ > 1000);
    else
        mode = 0;
    cycles = 3 + (16*dolphin);
}

static void nocode()
{
    if (gcc.ddown)
    {
        if (n == 0)
            n = millis();
        off = off || (millis()-n > 500);
    }
    else
        n = 0;
}

static void recalibrate()
{
    if (cal)
    {
        cal = gcc.x && gcc.y && gcc.start;
        ini.ax = gcc.xAxis -128; ini.ay = gcc.yAxis -128; // gets offset from analog stick in neutral
        ini.cx = gcc.cxAxis-128; ini.cy = gcc.cyAxis-128; // gets offset from c stick in neutral
        ini.l  = gcc.left;           ini.r  = gcc.right;  // gets offset from analog triggers in neutral
    }
    else if (gcc.x && gcc.y && gcc.start)
    {
        if (c == 0)
            c = millis();
        cal = millis() - c > 500;
    }
    else
        c = 0;
}

static void calibration()
{
    ax = constrain(gcc.xAxis -128-ini.ax, -128, 127); // offsets from neutral position of analog stick x axis
    ay = constrain(gcc.yAxis -128-ini.ay, -128, 127); // offsets from neutral position of analog stick y axis
    cx = constrain(gcc.cxAxis-128-ini.cx, -128, 127); // offsets from neutral position of c stick x axis
    cy = constrain(gcc.cyAxis-128-ini.cy, -128, 127); // offsets from neutral position of c stick y axis
    r  = mag(ax, ay); deg = ang(ax, ay);            // obtains polar coordinates for analog stick
    cr = mag(cx, cy);                               // obtains magnitude of c stick value
    ls.l = constrain(gcc.left -ini.l, 0, 255);        // fixes left trigger calibration
    ls.r = constrain(gcc.right-ini.r, 0, 255);        // fixes right trigger calibration
    gcc.left = ls.l; gcc.right = ls.r;              // sets proper analog shield values
    gcc.xAxis  = 128+ax; gcc.yAxis  = 128+ay;       // reports analog stick values
    gcc.cxAxis = 128+cx; gcc.cyAxis = 128+cy;       // reports c stick values
    recalibrate();                                  // allows holding x+y+start for 3 seconds to recalibrate
}

static float ang(float x, float y) { return atan2(y, x)*57.3 + 360*(y < 0); }        // returns angle in degrees when given x and y components
static float mag(char x, char y) { return sqrt(sq(x) + sq(y)); }                     // returns vector magnitude when given x and y components
static bool  mid(float val, float n1, float n2) { return val > n1 && val < n2; }     // returns whether val is between n1 and n2
static float arc(float val) { return abs(180 - abs(abs(deg-val) - 180)); }           // returns length of arc between the deg and val
static int   dis(float val) { return abs(fmod(val, 90) - 90*(fmod(val, 90) > 45)); } // returns how far off the given angle is from a cardinal
static float map(long val, float in, float ix, float on, float ox) { return (val-in)*(ox-on)/(ix-in)+on; }

void setup()
{
    gcc.origin = gcc.errlatch = gcc.high1 = gcc.errstat = 0; // init values
    g.n = ang(N_NOTCH_X, N_NOTCH_Y);          // calculates angle of N notch
    g.e = ang(E_NOTCH_X, E_NOTCH_Y);          // calculates angle of E notch
    g.s = ang(S_NOTCH_X, S_NOTCH_Y);          // calculates angle of S notch
    g.w = ang(W_NOTCH_X, W_NOTCH_Y);          // calculates angle of W notch
    g.sw = ang(SW_NOTCH_X, SW_NOTCH_Y);       // calculates angle of SW notch
    g.se = ang(SE_NOTCH_X, SE_NOTCH_Y);       // calculates angle of SE notch
    g.el = g.e-360*(g.e > 180); g.eh = g.e+360*(g.e < 180);  // gets east gate in 2 notations
    controller.read(); gcc = controller.getReport();         // reads controller once for calibration
    recalibrate();                                           // calibrates the controller for initial plug in
}

void loop()
{
    controller.read();                          // reads the controller
    data = defaultGamecubeData;                 // this line is necessary for proper rumble
    gcc = controller.getReport();               // gets a report of the controller read
    calibration(); recalibrate();               // fixes normal calibration and allows resetting with x+y+start
    if (!off)
        mods();                                 // implements all the mods (remove this to unmod the controller)
    data.report = gcc; console.write(data);     // sends controller data to the console
    controller.setRumble(data.status.rumble);   // allows for rumble
}
