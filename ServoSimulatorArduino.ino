/************************************************************************************
 * ServoSimulator
 ************************************************************************************
 * 
 * Arduino Sketck by Frank Scholl 2019
 * 
 * 
 * Read up to 8 PWM channel from an RC receiver
 * and send the results to a Nextion Display for graphical illustration
 * 
 * Thanks to Gabriel Staples for the 0.5us resolution library
 * http://www.ElectricRCAircraftGuy.com/
 */

#include <eRCaGuy_Timer2_Counter.h>

#define s_version "V.2019/10/31"

// Limits for Battery Icon
#define UBmax 8.1
#define UBnom 7.4
#define UBmin 6.8


//macros
#define fastDigitalRead(p_inputRegister, bitMask) ((*p_inputRegister & bitMask) ? HIGH : LOW)

//Global Variables & defines
byte input_pin_bitMask;
volatile byte* p_input_pin_register;
 
//volatile variables for use in the ISR (Interrupt Service Routine)
volatile boolean output_data = false; //the main loop will try to output data each time a new pulse comes in, which is when this gets set true
volatile unsigned long pulseCounts = 0; //units of 0.5us; the input signal high pulse time
volatile unsigned int pd = 0; //units of 0.5us; the pulse period (ie: time from start of high pulse to start of next high pulse)

volatile unsigned long channel_pulsestart[] = { 0,0,0,0,0,0,0,0 }; // measured starttime of pulses of each channel
volatile uint16_t channel_pulsetime[] = { 0,0,0,0,0,0,0,0 }; // measured pulse length of each channel


/************************************************************************************
 *      configure PinChange Interrupts
 ************************************************************************************/
void configurePinChangeInterruptsD234567B01()
{
 /* enable PinChangeInterrupt for 
  * D2 = PortD.2 = PCINT18
  * D3 = PortD.3 = PCINT19
  * D4 = PortD.4 = PCINT20
  * D5 = PortD.5 = PCINT21
  * D6 = PortD.6 = PCINT22
  * D7 = PortD.7 = PCINT23
  */
  PCMSK2 = 0b11111100;
  PCICR |= 0b00000001;
 /* enable PinChangeInterrupt for 
  * D8 = PortB.0 = PCINT0
  * D9 = PortB.1 = PCINT1
  */
  PCMSK0 = 0b00000011;
  PCICR |= 0b00000100;
}

/************************************************************************************
 *      write String to Nextion
 ************************************************************************************/
//NOTE: A great big thanks to: RamjetX for writing this function. You can find his/her post here: http://forum.arduino.cc/index.php?topic=89143.0. Please go give him/her some Karma!
void writeString(String stringData) { // Used to serially push out a String with Serial.write()

  for (int i = 0; i < stringData.length(); i++)
  {
    Serial.write(stringData[i]);   // Push each char 1 by 1 on each loop pass  
  }

  Serial.write(0xff); //We need to write the 3 ending bits to the Nextion as well
  Serial.write(0xff); //it will tell the Nextion that this is the end of what we want to send.
  Serial.write(0xff);

}// end writeString function


////Use macro instead
//boolean fastDigitalRead(volatile byte* p_inputRegister,byte bitMask)
//{
//  return (*p_inputRegister & bitMask) ? HIGH : LOW;
//}


/************************************************************************************
 *      >>>  setup  <<<
 ************************************************************************************/
void setup() 
{
//  pinMode(INPUT_PIN,INPUT_PULLUP); //use INPUT_PULLUP to keep the pin from floating and jumping around when nothing is connected
  String sendThis = ""; //Declare and initialise the string we will send for Nextion

  //use INPUT_PULLUP to keep the pin from floating and jumping around when nothing is connected
  pinMode(2,INPUT_PULLUP);
  pinMode(3,INPUT_PULLUP);
  pinMode(4,INPUT_PULLUP);
  pinMode(5,INPUT_PULLUP);
  pinMode(6,INPUT_PULLUP);
  pinMode(7,INPUT_PULLUP);
  pinMode(8,INPUT_PULLUP);
  pinMode(9,INPUT_PULLUP);
  
  //configure timer2
  timer2.setup();

  //prepare for FAST digital reads on INPUT_PIN, by mapping to the input register (ex: PINB, PINC, or PIND), and creating a bitMask
  //using this method, I can do digital reads in approx. 0.148us/reading, rather than using digitalRead, which takes 4.623us/reading (31x speed increase)
  //input_pin_bitMask = digitalPinToBitMask(INPUT_PIN);
  //p_input_pin_register = portInputRegister(digitalPinToPort(INPUT_PIN));

  // use internal 1.1V reference for Battery measurement
  analogReference(INTERNAL);
  
  configurePinChangeInterruptsD234567B01();

  Serial.begin(57600); // for Debug or Nextion

  // send version info
  sendThis = "pageS.tv.txt=";
  sendThis.concat('"'); 
  sendThis.concat(s_version);
  sendThis.concat('"'); 
  writeString(sendThis);
}

/************************************************************************************
 *      >>>  loop  <<<
 ************************************************************************************/
void loop() 
{
  //local variables
  uint8_t i; // for loop counter
  //static uint8_t n=0;     // for delayed version update
  float ubatt;

  static uint8_t vbp = 0; // battery picture for test
  String s_ubatt;         // battery value for test
  String sendThis = "";   //Declare and initialise the string we will send for Nextion

  //todo: check for timeouts: current tim - pulse start time > timeout (~30ms)

/* repeat if it doesn't work first time
  n++;
  if (n>=20)
  {
    // send version info sometimes
    sendThis = "pageS.tv.txt=";
    sendThis.concat('"'); 
    sendThis.concat(s_version);
    sendThis.concat('"'); 
    writeString(sendThis);
  }
*/

  // send mode inf to Nextion
  sendThis = "tmode.txt=";
  sendThis.concat('"'); 
  sendThis.concat("PWM");
  sendThis.concat('"'); 
  writeString(sendThis);

  // send all 8 values in microseconds to Nextion
  for (i=0; i<8; i++) {
    sendThis = "vm"; //Build the part of the string that we know
    //sendThis.concat(String(i)); 
    sendThis.concat(String(7-i)); // reverse order
    sendThis.concat(".val="); 
    sendThis.concat(String(channel_pulsetime[i]/2)); 
    writeString(sendThis);
  }
  
/*
  // serial debug output:
  
  for (i=0; i<8; i++) {
    if (((channel_pulsetime[i])>1400) & ((channel_pulsetime[i])<4600))
    {
      //Serial.write(channel_pulsetime[i]);
      Serial.print("[");
      Serial.print(i);
      Serial.print("]=");
      Serial.print((channel_pulsetime[i])/2);
      Serial.print(" ");
    }
    else
    {
      Serial.print("[");
      Serial.print(i);
      Serial.print("]=");
      Serial.print("----");
      Serial.print(" ");
    }
  }
  Serial.println(".");
*/

  // Battery Voltage
  
  ubatt = 0.0083* analogRead(A0); // using a voltage divider with 26k1/3k83 and internal 1.1V reference
  s_ubatt = String(ubatt,1); // battery voltage with 1 decimal place
  sendThis = "tbatt.txt=";
  sendThis.concat('"'); 
  sendThis.concat(s_ubatt);
  sendThis.concat("V");
  sendThis.concat('"'); 
  writeString(sendThis); //Use a function to write the message character by character to the Nextion because

  // select battery picture
  if (ubatt>=UBmax)
  {
    vbp=24;
  }
  else
  {
    if (ubatt>=UBnom)
    {
      vbp=23;
    }
    else
    {
      if (ubatt>=UBmin)
      {
        vbp=22;
      }
      else
      {
        vbp=21;
      }
    }
  }
  sendThis = "pbatt.pic="; 
  sendThis.concat(String(vbp)); 
  writeString(sendThis); //Use a function to write the message character by character to the Nextion because

  // improve loop time...
  delay(100);
} //end of loop()


//Interrupt Service Routines (ISRs) for Pin Change Interrupts
//see here: http://www.gammon.com.au/interrupts

/************************************************************************************
 *      PCINT0_vect is for pins D8 to D13
 ************************************************************************************/
ISR(PCINT0_vect)
{
  //local variables
  static uint8_t portb_new = 0; //initialize
  static uint8_t portb_old = 0; //initialize
  static unsigned long t_now = 0; //units of 0.5us

  // catch timer2 value
  t_now = timer2.get_count(); //0.5us units
  // enable interrupt for other events (timer2 overflow)
  interrupts();
  // check changes on pins D8 to D9 = PortB.0-.1
  portb_old=portb_new;
  portb_new = PINB;

  // D8 = PortB.0 = PCINT0
  if ((portb_new ^ portb_old) & 0b00000001) // check for changed D8
  {
    if ((portb_new & 0b00000001) == 0) // check for falling edge
    {
      // on falling edge set ch(x)_pulsetime to (timer2 value - ch(x)_starttime)
      channel_pulsetime[6] = (uint16_t)(t_now - channel_pulsestart[6]);
    }
    else
    {
      // on rising edge set ch(x)_starttime to timer2 value
      channel_pulsestart[6] = t_now;
    }
  }

  // D9 = Port.1 = PCINT1
  if ((portb_new ^ portb_old) & 0b00000010) // check for changed D9
  {
    if ((portb_new & 0b00000010) == 0) // check for falling edge
    {
      // on falling edge set ch(x)_pulsetime to (timer2 value - ch(x)_starttime)
      channel_pulsetime[7] = (uint16_t)(t_now - channel_pulsestart[7]);
    }
    else
    {
      // on rising edge set ch(x)_starttime to timer2 value
      channel_pulsestart[7] = t_now;
    }
  }
}

/************************************************************************************
 *      PCINT1_vect is for pins A0 to A5
 ************************************************************************************/
ISR(PCINT1_vect)
{
  //pinChangeIntISR();
}

/************************************************************************************
 *      PCINT2_vect is for pins D0 to D7
 ************************************************************************************/
ISR(PCINT2_vect)
{
  //local variables
  static uint8_t portd_new = 0; //initialize
  static uint8_t portd_old = 0; //initialize
  static unsigned long t_now = 0; //units of 0.5us
  uint8_t portmask = 0b00000100; // initialize with D2 = PortD.2
  uint8_t channel = 0; // initialize ch[0]=D2

  // catch timer2 value
  t_now = timer2.get_count(); //0.5us units
  // enable interrupt for other events (timer2 overflow)
  interrupts();

  // check changes on pins D2 to D7 = PortD.2-.7
  portd_old=portd_new;
  portd_new = PIND;
  portmask = 0b00000100;

  for(channel = 0;channel <6; channel++)
  {
    if ((portd_new ^ portd_old) & portmask) // check for changed port bit
    {
      if ((portd_new & portmask) == 0) // check for falling edge
      {
        // on falling edge set ch(x)_pulsetime to (timer2 value - ch(x)_starttime)
        channel_pulsetime[channel] = (uint16_t)(t_now - channel_pulsestart[channel]);
      }
      else
      {
        // on rising edge set ch(x)_starttime to timer2 value
        channel_pulsestart[channel] = t_now;
      }
    }
    portmask=portmask<<1; // shift to next bit
  }
}
