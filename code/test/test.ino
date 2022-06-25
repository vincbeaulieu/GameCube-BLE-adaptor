// code from : http://www.int03.co.uk/crema/hardware/gamecube/gc-control.html
#include <Arduino.h>
#include <stdio.h>
#pragma comment (lib, "winmm")




//-----------------------------------------------------------------------------

/**
 *
 * THIS SOFTWARE IS FREE, USE IT FOR WHATEVER YOU LIKE
 * I accept no responsibility for any loss or damage
 * whatsoever. You use this software, hardware and
 * associated documentation AT YOUR OWN RISK.
 * This software may potentially burn down your house,
 * steal your girlfriend, drink all your beer.. and who
 * knows what. You'd have to be mad to compile it really.
 *
 * This program polls a gamecube controller attached to the parallel
 * port, using an external shift register to read back the serial
 * data from the pad at a lower speed. It is able to read all the
 * button states, and the analogue stick/c-stick/left/right values.
 * This is a work in progress: more hardware/software development
 * and testing is needed..
 *   J.Ward, 7th March 2004.
 *   www.crema.co.uk
 *
 * You need to build the hardware first, or this
 * won't do anything of interest.
 *
 * The program has only been tested on Windows 2000 (P4 2.8GHz, i875P chipset),
 * and with an official Nintendo controller. Hopefully it will work on XP, but
 * isn't tested yet (offers?) I'm pretty sure it won't work on Windows 9x
 * For the program to work, you must download and install the 'giveio' device
 * driver. This requires administrator rights.
 *
 * Keyboard controls:
 *   Q/ESC = Quit
 *   R     = Toggle rumble motor on/off
 */

/**
 * For the hardcore (and until I draw the circuit diagram), this
 * should be enough information to build the external hardware.
 *
 * The following external parts are needed:
 *   1x MC14557B (1-to-64 bit Variable Length Shift Register)
 *   1x  1K resistor
 *   1x 10K resistor
 *   1x 4K7 resistor
 *   2x 1N4148 diodes
 *   1x 3.3V regulator (and associated components)
 *   1x DB25 male connector
 *   1x Gamecube extension cable (for connectors..)
 *
 * Assuming:
 *   You have a 5V regulated supply, a GND rail that
 *   is common to your electronics and the parallel port,
 *   and a 3.3V regulated supply relative to common GND rail.
 *
 * Connect as follows:
 *   Wire 4557 pins 1,2,4,6,9,12,13,14,15,16 to +5V
 *   Wire 4557 pin 8 to GND.
 *   Wire pin 3 to parallel port D2
 *   Wire pin 5 to anode of 1N4148, cathode to parallel port D1
 *   Wire pin 5 via 4K7 resistor to gamepad data line
 *   Wire pin 7 via 10K resistor to gamepad data line
 *   Wire pin 10 to parallel port PE status bit
 *   Wire gamepad data line to anode of second 1N4148, cathode to D0
 *   Connect gamepad data line via 1K resistor to 3.3V supply rail
 *   Connect GND rail to gamepad ground pins (there are two!)
 *   Connect 3.3V, 5V supply rails to gamepad
 *
 * As far as I can tell, this circuit shouldn't really work, because it
 * omits an RC circuit to integrate the data bits before sampling with
 * the shift register, but it does work.. perfectly. I'll post an
 * improved circuit diagram soon. Like I said, it's a work in progress.
 * Comments welcome :)
 *
 */

//-----------------------------------------------------------------------------

// parallel port base i/o address
#define PAR_DATA 0x378
#define PAR_STAT (PAR_DATA+1)

// parallel port control bits
#define PC_NSTB 0x01    // /strobe
#define PC_NLFD 0x02    // /line feed
#define PC_INIT 0x04    // initialize
#define PC_NSEL 0x05    // /select in

// parallel port status bits
#define PS_ERR  0x08    // error
#define PS_SEL  0x10    // select
#define PS_EOP  0x20    // paper end
#define PS_ACK  0x40    // acknowledge
#define PS_NBSY 0x80    // /busy

// interface: data bit(s)
#define PD_DATA  0x01 // 0=pull pad-data line low, 1=float
#define PD_NCLK  0x02 // 0=clock (1->0 transition), 1=float
#define PD_RST   0x04 // 1=reset, 0=normal

// interface: status bit(s)
#define PS_SRDATA 0x20  // EOP

//-----------------------------------------------------------------------------

// commands for gamecube pad
//
#define GC_QUERY    0x400302    // command to query pad values
#define GC_RUMBLE_ON  0x400001    // turn on  rumble motor
#define GC_RUMBLE_OFF 0x400000    // turn off rumble motor

// button bit-masks for use with GCPadData struct 'buttons' field
//
#define GCB_START   0x1000  // start button
#define GCB_Y     0x0800  // Y button
#define GCB_X     0x0400  // X button
#define GCB_B     0x0200  // B button (red)
#define GCB_A     0x0100  // A button (green)
#define GCB_AlwaysOne 0x0080  // ?? this bit appears to always be high ??
#define GCB_LEFT    0x0040  // Left  Shoulder Button (end stop)
#define GCB_RIGHT   0x0020  // Right Shoulder Button (end stop)
#define GCB_Z     0x0010  // Z button (purple)
#define GCB_DPAD_UP   0x0008  // D pad up
#define GCB_DPAD_DOWN 0x0004  // D pad down
#define GCB_DPAD_RIGHT  0x0002  // D pad right
#define GCB_DPAD_LEFT 0x0001  // D pad left

/**
 * This structure contains all the button/analogue values
 * that are returned by the gamepad.
 */
#pragma pack(push,1)
typedef struct {
  short int buttons; ///> button state (use GCB_xxx bit masks)
  byte joy1X;   ///> joystick 1 X value
  byte joy1Y;   ///> joystick 1 Y value
  byte joy2X;   ///> joystick 2 X value (yellow C stick)
  byte joy2Y;   ///> joystick 2 Y value (yellow C stick)
  byte left;    ///> left  shoulder button (analogue)
  byte right;   ///> right shoulder button (analogue)
} GCPadData;
#pragma pack(pop)

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

// open interface
void gc_open() {
  // start giveio driver (if not already) and open it; this gives
  // us access to direct port i/o on windows 2000
  FILE *fp = fopen("\\\\.\\giveio","wb");

  // setup port state, delay a moment in case pad sends early
  asm("out PAR_DATA,PD_DATA|PD_NCLK|PD_RST");
  delay(1);

  // bump up our priority in hope of avoiding interruptions
  // during our I/O operations
}//gopen

//-----------------------------------------------------------------------------

/**
 * Send a command to the gamepad. Uses only the lower 24-bits.
 * For example:
 *   0x400300 = value given in YAGCD to initialise pads
 *   0x400302 = value seen when i 'scoped the pad in rogue leader
 *   0x400001 = rumble motor on?
 *   0x400000 = rumble motor off?
 */
void gc_send(long cmd) {
  //printf("gc_send(0x%06lX)\n", cmd);

  byte
    *buffer = new byte[4*25], // 4us per bit, 24+1 bits (last is stop bit)
    *p      = buffer;

  // add 1 in the LSB to output a stop bit at the end
  cmd=(cmd<<1)|1;

  // state of clock/reset bits during send
  byte std = PD_NCLK|PD_RST;

  // fill a buffer with raw data to send over the parallel port, this
  // avoids using a loop and/or conditionals which would delay I/O
  for (int b=0;b<25;b++) {
    // get the bit
    int z = (cmd>>(24-b))&1;

    // fill buffer with high/low pattern
    if (z) {
      // high: send 0111
      p[0]=std;
      p[1]=p[2]=p[3]=(PD_DATA|std);
    } else {
      // low: send 0001
      p[0]=p[1]=p[2]=std;
      p[3]=(PD_DATA|std);
    }

    // move buffer pointer
    p+=4;
  }//for(b)
  
  // enable shift register on last write
  buffer[4*25-1] = PD_DATA|PD_NCLK;

  // send the command
  asm (
    "mov ecx,4*25"
    "mov esi,buffer"
    "mov dx,PAR_DATA"
    "rep outsb"     // takes about 1us per byte hopefully
  );

  // wait for shift register to fill,
  // which actually takes about 348us
  delay(1);

  // free storage
  delete [] buffer;
}//gc_send

//-----------------------------------------------------------------------------

// close interface
void gc_close() {
  // make sure rumble is disabled
  gc_send(GC_RUMBLE_OFF);
}//gc_close

//-----------------------------------------------------------------------------

/**
 * Read out shift register contents into a
 * GCPadData structure
 */
bool gc_read_sr(GCPadData *padData) {
  if (!padData) return false;

  // This is the layout of the serial data sent from the Gamecube pad, as it
  // arrives in our shift register.
  // [0]      [1]      [2]      [3]      [4]      [5]      [6]      [7]
  // 76543210 76543210 76543210 76543210 76543210 76543210 76543210 76543210
  // 000SYXBA _LRZUDRL [-JOYX-] [-JOYY-] [-CSTX-] [-CSTY-] [-LBUT-] [-RBUT-]

  // read out contents of shift register
  byte *data = (byte*)padData;
  for (long i=0;i<64;i++) {
    // read a bit from the shift register, and stash
    // the result in our output buffer
    byte
      res = (_inp(PAR_STAT)&PS_SRDATA),
      bit = 0x80>>(i&7);
    data[i>>3] = (data[i>>3] & ~bit) | (res?bit:0);

    // dump out binary values for debugging purposes
    //printf("%d", res?1:0);

    // clock the shift register
    asm(
      "out PAR_DATA, PD_DATA"     // clock=0
      "out PAR_DATA, PD_DATA|PD_NCLK" // clock=1
    );
  }
  //for (i=0;i<8;i++) printf("%02X",(int)data[i]);
  //printf("\n");

  // swap byte order of button word
  byte t=data[1];
  data[1]=data[0];
  data[0]=t;

  // no easy way to detect an error, but we can check for
  // some obvious ones. for example, DPadUp+DPadDn simultaneously
  if ( ((padData->buttons & GCB_DPAD_UP)!=0) &&
     ((padData->buttons & GCB_DPAD_DOWN)!=0))  return false;
  if ( ((padData->buttons & GCB_DPAD_LEFT)!=0) &&
     ((padData->buttons & GCB_DPAD_RIGHT)!=0)) return false;

  // all zero case is almost certainly wrong. the sticks don't
  // usually go all the way to zero on any axis
  if ( (padData->joy1X==0) && (padData->joy1Y==0) &&
     (padData->joy2X==0) && (padData->joy2Y==0)) return false;
  // all high case is wrong for the same reason
  if ( (padData->joy1X==0xFF) && (padData->joy1Y==0xFF) &&
     (padData->joy2X==0xFF) && (padData->joy2Y==0xFF)) return false;

  // we expect one bit to always be high.. if not, then that
  // probably means that we have an error
  return ((padData->buttons & GCB_AlwaysOne)!=0);
}//gc_read_sr

//-----------------------------------------------------------------------------

static bool s_gcRumble = false;

void gc_set_rumble(bool rumble) {
  s_gcRumble=rumble;
  if (s_gcRumble)
    gc_send(GC_RUMBLE_ON);
  else
    gc_send(GC_RUMBLE_OFF);
}//gc_set_rumble

bool gc_get_rumble() {
  return s_gcRumble;
}//gc_get_rumble

bool gc_read_pad(GCPadData *padData) {
  if (!padData) return false;

  // pad command; preserve rumble bit if enabled
  long cmd = GC_QUERY | (gc_get_rumble()?GC_RUMBLE_ON:0);

  // send the command
  gc_send(cmd);

  // read shift register contents
  if (gc_read_sr(padData)) {
    return true;
  } else {
    // retry: added this because i get the odd error, probably due
    // to a timing error when sending the command. hopefully this
    // can be eliminated by developing a kernel mode driver
    gc_send(cmd);
    return gc_read_sr(padData);
  }
}//gc_read_pad

//-----------------------------------------------------------------------------

/**
 * Output pad data to console in readable format
 */
void gc_show_pad(GCPadData *padData) {
  if (!padData) return;

  // map button mask IDs to names
  struct {
    short int mask;
    char *desc,*alt;
  } map[]= {
    {GCB_START,   "S","-"},
    {GCB_Y,     "Y","-"},
    {GCB_X,     "X","-"},
    {GCB_B,     "B","-"},
    {GCB_A,     "A","-"},
    {GCB_LEFT,    "L","-"},
    {GCB_RIGHT,   "R","-"},
    {GCB_Z,     "Z","-"},
    {GCB_DPAD_UP, "Up","--"},
    {GCB_DPAD_DOWN, "Dn","--"},
    {GCB_DPAD_RIGHT,"Rt","--"},
    {GCB_DPAD_LEFT, "Lt","--"},
    {0,0,0} // null terminator
  };

  // display button data
  for (int n=0; map[n].desc; n++)
    if (padData->buttons & map[n].mask)
      printf("%s", map[n].desc);
    else
      printf("%s", map[n].alt);

  // joy1 analogue values
  printf(" %4d %4d", (int)padData->joy1X, (int)padData->joy1Y);
  // joy2 analogue values
  printf(" %4d %4d", (int)padData->joy2X, (int)padData->joy2Y);
  // L/R analogue values
  printf(" %4d %4d", (int)padData->left, (int)padData->right);

  // new line
  printf("\n");
}//gc_show_pad

//-----------------------------------------------------------------------------

int main (int argc, char **argv) {
  gc_open();

  bool quit = false;  // quit loop?

  do {
    if (kbhit()) {
      // handle keyboard input
      switch (getch()) {
      case 'r':
        // toggle rumble motor on/off
        gc_set_rumble(!gc_get_rumble());
        break;
      case 'q':
      case 27:
        quit=true;
        break;
      }
    } else {
      // read the pad data
      GCPadData pad;
      if (!gc_read_pad(&pad))
        printf("err: ");
      
      // display pad data
      gc_show_pad(&pad);
    }
    // delay 20ms to avoid querying pad too frequently
    delay(20);
  } while (!quit);

  gc_close();
  return 0;
}//main

//----------------
