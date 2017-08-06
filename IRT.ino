/*
Internal Battery Reistance Tester
Terry Myers <TerryJMyers@gmail.com> https://github.com/terryjmyers


Connect Battery to a 0.1R shunt resistor to an XL6009 boost converter
Boost converter output connects to a 12R and a 6R 10W power resistor.
Each power resistor then connects to an N channel mosfet controlled by pins 2 and 3
Arduino pind 2 and 3 connect to gate of mosfet through 100R resistor
Mosfet gate has 100K resistor to ground on gate
Battery negative terminal has N channel mosfet connected backwards for reverse polarity detection
Reverse Polarity detection LED connects directly to nagative battery terminal through an appropriate resistor to the positive terminal so that it lights up if battery is placed in backwards.
Note that ONLY the LED and the drain of the reverse polarity mosfet should be connected to the battery negative.  All other "ground" points are actually connected to the source pin of the reserve polarity mosfet

ADS1115 ADC IC	pin A0 connects to battery positive
				pin A2 connects to high side of shunt
				pin A3 connects to low side of shunt

//ADS @ 0x48
LCD I2Cbackpac = 0x27
text size = 1
	14 char accross wide
	6 lines
text size = 2
	7 char wide
	3 lines
text size = 3
	4 char wide
	2 lines
	

	TODO:
	If voltage drops below 3.60 at any time error and tell user to recharge
	If voltage drops more than 0.2V from low to high tell user to recharge
*/


//#define DEBUG

//Includes
#include <ADS1115_lite.h>
#include <LoopStatistics.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
// Hardware SPI (faster, but must use certain hardware pins):
// SCK is LCD serial clock (SCLK) - this is pin 13 on Arduino Uno
// MOSI is LCD DIN - this is pin 11 on an Arduino Uno
// pin 6 - Data/Command select (D/C)
// pin 5 - LCD chip select (CS)
// pin 4 - LCD reset (RST)

Adafruit_PCD8544 display = Adafruit_PCD8544(7, 5, 4);


ADS1115_lite adc(ADS1115_DEFAULT_ADDRESS); //Initializes wire library, sets private configuration variables to ADS1115 default(2.048V, 128SPS, Differential mode between  AIN0 and AIN1.  The Address parameter is not required if you want default
float ShuntResistance = 0.1153;
float VoltageCalibrationFactorHigh= 1.011;
float VoltageCalibrationFactorLow = 1.0;
float NFETRDSon = 0.047;
float TotalWireReistance = 0.020; //estimated
bool debug = 1;
float voltageMinStart = 3.6;
float voltageMin = 3.5;
float voltageDropMax = 0.4;

//Setup a instance of LoopTime
#ifdef DEBUG
LoopStatistics LT;
#endif

//Serial Read tags
#define STRINGARRAYSIZE 8 //Array size of parseable string
#define SERIAL_BUFFER_SIZE 64 //reserve some string space.  Arduino has a max 64b buffer size
String sSerialBuffer; //Create a global string that is added to character by character to create a final serial read
String sLastSerialLine; //Create a global string that is the Last full Serial line read in from sSerialBuffer

String s;
float Vbat;
float Ibat;
float Pbat;
float R;

uint32_t timerstart;
uint32_t timerstop;
						

//SETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUP
void setup() {
	s.reserve(64);

	//Setup Serial 
	//In case the user uses the default setting in Arduino, give them a message to change BAUD rate
	Serial.begin(9600);
	Serial.println(F("Set baud rate to 250 000"));
	Serial.flush();   // wait for send buffer to empty
	delay(2);    // let last character be sent
	Serial.end();      // close serial
	Serial.begin(250000);
	Serial.println();

	//Set serial read buffer size to reserve the space to concatnate all bytes recieved over serial
	sSerialBuffer.reserve(SERIAL_BUFFER_SIZE);

	//Initialize the display
		display.begin();	
		display.setContrast(50);
		display.clearDisplay();   // clears the screen and buffer
		display.setTextSize(2);
		display.setTextColor(BLACK);
		display.setRotation(2);

	//Set pin modes
		pinMode(2, OUTPUT);
		pinMode(3, OUTPUT);

	//check ADC connection
		if (!adc.testConnection()) {
			Serial.println(F("ERROR: ADS1115 Connection failed")); //oh man...something is wrong
			return;
		}
		adc.setGain(ADS1115_REG_CONFIG_PGA_4_096V);
		adc.setSampleRate(ADS1115_REG_CONFIG_DR_8SPS); //Set the slowest and most accurate sample rate
	//Check Battery Voltage
		if (getVbat() <= voltageMinStart) errorVoltageTooLowStart();
		RunIRT();
	
}
//SETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUPSETUP






//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
void loop() {
	displayIR();

}
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP
//MAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOPMAINLOOP

bool RunIRT() {
	digitalWrite(3, HIGH);

	timerstart = millis();
		while (millis() - timerstart < 5000) {
			Vbat = getVbat()*VoltageCalibrationFactorLow;
			Ibat = getIbat();
			Pbat = Vbat * Ibat;
			if (Vbat<voltageMin) errorVoltageTooLow();

			s = ftos(Vbat,3); s += F("V "); s += ftos(Ibat,3); s += F("A ");  s += ftos(Pbat, 3);	s += F("W");

			Serial.println(s);
			//Update Display
			if (millis() - timerstart < 1000) {
				display.clearDisplay();
				display.setCursor(9,0);
				display.setTextSize(3);
				display.println(F("LOAD"));
				display.setCursor(33,24);
				display.println(F("1"));
				display.display();
			}
			else {
				display.clearDisplay();
				display.setCursor(0, 0);
				display.setTextSize(2);
				display.print(ftos(Vbat, 3)); display.println(F(" V"));
				display.print(ftos(Ibat, 3)); display.println(F(" A"));
				display.print(ftos(Pbat, 3)); display.println(F(" W"));
				display.display();
			}
		}
	float V1 = Vbat;
	float I1 = Ibat;
	digitalWrite(3, LOW);

	Serial.println(F("Setting Constant Power Load of ~6Watts for ~ 10 seconds"));
	digitalWrite(2, HIGH);
	timerstart = millis();
		while (millis() - timerstart < 10000) {
			Vbat = getVbat()*VoltageCalibrationFactorHigh;
			Ibat = getIbat();
			Pbat = Vbat * Ibat;
			if (Vbat<voltageMin) errorVoltageTooLow();
			if (V1 - Vbat>voltageDropMax) errorVoltageDip();

			s = ftos(Vbat, 3); s += F("V "); s += ftos(Ibat, 3); s += F("A ");  s += ftos(Pbat, 3);	s += F("W");

			Serial.println(s);
			//Update Display
			if (millis() - timerstart < 1000) {
				display.clearDisplay();
				display.setCursor(9, 0);
				display.setTextSize(3);
				display.println(F("LOAD"));
				display.setCursor(33, 24);
				display.println(F("2"));
				display.display();
			}
			else {
				display.clearDisplay();
				display.setCursor(0, 0);
				display.setTextSize(2);
				display.print(ftos(Vbat, 3)); display.println(F(" V"));
				display.print(ftos(Ibat, 3)); display.println(F(" A"));
				display.print(ftos(Pbat, 3)); display.println(F(" W"));
				display.display();
			}
		}
	float V2 = Vbat;
	float I2 = Ibat;
	digitalWrite(2, LOW);
	R = (V1 - V2) / (I2 - I1);
	Serial.print(F("Internal Reistance: ")); Serial.print(R, 4); Serial.println(F("Ohm"));



}
void errorVoltageTooLowStart() {
	display.clearDisplay();
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.println(F("ERROR: Initial"));
	display.println(F("voltage must"));
	display.println(F("start above"));
	display.print(voltageMinStart); display.print(F("V"));
	display.println();
	display.println(F("Charge Battery"));
	display.display();
	while (1) {
		delay(1);
	}
}
void errorVoltageTooLow() {
	display.clearDisplay();
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.println(F("ERROR: Voltage"));
	display.println(F("must stay"));
	display.print(F("above ")); display.print(voltageMin); display.println(F("V"));
	display.println(F("throughout "));
	display.println(F("test. "));
	display.println(F("Charge Battery"));
	display.display();
	while (1) {
		delay(1);
	}
}
void errorVoltageDip() {
	display.clearDisplay();
	display.setCursor(0, 0);
	display.setTextSize(1);
	display.println(F("ERROR: Voltage"));
	display.println(F("dropped more"));
	display.print(F("than ")); display.print(voltageDropMax); display.println(F("V"));
	display.println();
	display.println();
	display.println(F("Charge Battery"));
	display.display();
	while (1) {
		delay(1);
	}
}
bool displayIR() {
	Vbat = getVbat();
	display.clearDisplay();
	display.setTextSize(2);
	display.setCursor(0, 0);
	display.println(F("IR = "));
	display.print(ftos(R, 3)); display.println(F("R"));
	display.print(ftos(Vbat, 3)); display.print(F("V"));
	display.display();
}

float getVbat() {

	adc.setGain(ADS1115_REG_CONFIG_PGA_4_096V);
	adc.setMux(ADS1115_REG_CONFIG_MUX_SINGLE_0); //Set single ended mode between AIN0 and GND adc.triggerConversion(); //Start a conversion.  This immediatly returns
	delay(1); //delay to let the voltage settle
	adc.triggerConversion(); //Start a conversion.  This immediatly returns
	uint16_t VbatRaw = adc.getConversion(); //This polls the ADS1115 and wait for conversion to finish, THEN returns the value
	return VbatRaw / 32767.0 * 4.096;
}//==============================================================================================
float getIbat() {


	adc.setGain(ADS1115_REG_CONFIG_PGA_0_256V);
	adc.setMux(ADS1115_REG_CONFIG_MUX_DIFF_2_3); //Set single ended mode between AIN0 and GND adc.triggerConversion(); //Start a conversion.  This immediatly returns
	delay(1); //delay to let the voltage settle
	adc.triggerConversion(); //Start a conversion.  This immediatly returns
	uint16_t VshuntRaw = adc.getConversion(); //This polls the ADS1115 and wait for conversion to finish, THEN returns the value
	return VshuntRaw / 32767.0 * 0.256 / ShuntResistance;
}//==============================================================================================
String ftos(float f, float p) {
	//convert a float to a string with precision and trim whitespace
	String w = "";
	w = String(f, p);
	w.trim();
	return w;
}//===============================================================================================

void SerialMonitor() {
	//Read and process inputs from the serial port
	//This function should be called as often as possible

	if (serialRead() == 0) { return; } //Read Data in from the Serial buffer, immediatly return if there is no new data

									   //declare some variables
	uint8_t i;
	String aSerialStringParse[STRINGARRAYSIZE]; //Create a small array to store the parsed strings 0-7

												//--Split the incoming serial data to an array of strings, where the [0]th element is the number of CSVs, and elements [1]-[X] is each CSV
												//If no Commas are detected the string is still placed into the [1]st array
	StringSplit(sLastSerialLine, &aSerialStringParse[0]);

	/*
	MANUAL COMMANDS
	Commands can be any text that you can trigger off of.
	It can also be paramaterized via a CSV Format with no more than STRINGARRAYSIZE - 1 CSVs: 'Param1,Param2,Param3,Param4,...,ParamSTRINGARRAYSIZE-1'
	For Example have the user type in '9,255' for a command to manually set pin 9 PWM to 255.
	*/
	if (aSerialStringParse[0].toInt() == 1) {
		//Process single string serial commands without a CSV
		if (aSerialStringParse[1] == "?") { Serial.print(HelpMenu()); } //print help menu
		if (aSerialStringParse[1] == "debug") { if (debug) { debug = 0; Serial.println(F("debug off")); } else { debug = 1; Serial.println(F("debug on")); } } //toggle debug bool
																																							   //add more here
	}
	else if (aSerialStringParse[0].toInt() > 1) {
		//Process multiple serial commands that arrived in a CSV format
		//Do something with the values by adding custom if..then statements
		if (aSerialStringParse[1] == "m") {
			Serial.print(F("m detected, value:"));
			i = aSerialStringParse[2].toInt();
			Serial.println(i);
		}

		if (aSerialStringParse[1] == "t") {
			Serial.print(F("t detected, value(s):"));
			i = aSerialStringParse[2].toInt();
			Serial.print(i);
			Serial.print(F(","));
			i = aSerialStringParse[3].toInt();
			Serial.println(i);
		}

	} //end else if(aSerialStringParse[0].toInt() > 1)
	sLastSerialLine = ""; //Clear out buffer, This should ALWAYS be the last line in this if..then
} //END SerialMonitor()
  //==============================================================================================
bool serialRead(void) {
	/*Read hardware serial port and build up a string.  When a newline, carriage return, or null value is read consider this the end of the string
	RETURN 0 when no new full line has been recieved yet
	RETURN 1 when a new full line as been recieved.  The new line is put into sLastSerialLine
	*/
	//Returns 0 when no new full line, 1 when there is a new full line
	while (Serial.available()) {
		char inChar = (char)Serial.read();// get the new byte:
		if (inChar == '\n' || inChar == '\r' || inChar == '\0' || sSerialBuffer.length() >= SERIAL_BUFFER_SIZE) {//if the incoming character is a LF or CR finish the string
			sLastSerialLine = sSerialBuffer; //Transfer the entire string into the last serial line
			sSerialBuffer = ""; // clear the string buffer to prepare it for more data:
			return 1;
		}
		sSerialBuffer += inChar;// add it to the inputString:
	}
	return 0;
}
//==============================================================================================
void EndProgram(String ErrorMessage) {
	//An unrecoverable error occured
	//Loop forever and display the error message
	while (1) {
		Serial.print(F("Major Error: "));
		Serial.println(ErrorMessage);
		Serial.println(F("Cycle power to restart (probably won't help)"));
		delay(5000);
	}
}
//==============================================================================================
float SCP(float Raw, float RawLo, float RawHi, float ScaleLo, float ScaleHi) {
	return (Raw - RawLo) * (ScaleHi - ScaleLo) / (RawHi - RawLo) + ScaleLo;
}
//===============================================================================================
void DrawLine() {
	Serial.println(F("==============================================================================================="));
}
//===============================================================================================
void StringSplit(String text, String *StringSplit) {
	/*
	Perform a CSV string split.
	INPUTS:
	text - CSV string to separate
	StringSplit - Pointer to an array of Strings
	DESCRIPTION:
	text can have CSV or not
	Each CSV is placed into a different index in the Array of Strings "StringSplit", starting with element 1.
	Element 0 (StringSplit[0]) will contain the number of CSV found.  Rember to use String.toInt() to convert element[0]
	*/
	char *p;
	char buf[64]; //create a char buff array
				  //text += "\0"; //add null string terminator
	text.toCharArray(buf, 64); //convert string to char array

							   //Split string
	byte PTR = 1; //Indexer
	p = strtok(buf, ",");
	do
	{
		StringSplit[PTR] = p;//place value into array
		PTR++; //increment pointer for next loop
		p = strtok(NULL, ","); //do next split
	} while (p != NULL);

	//Place the number of CSV elements found into element 0
	StringSplit[0] = PTR - 1;//<--this error is fine...

}
//===============================================================================================
void SerialPrintArray(String *Array) {
	//Print out the array structure to the serial monitor
	//The array must have the length in its 0th element
	Serial.print(F("Array size ("));
	Serial.print(Array[0]);
	Serial.print(F("), elements:  "));
	for (int i = 1; i <= Array[0].toInt(); i++) {
		Serial.print(Array[i]);
		if (i < Array[0].toInt()) { Serial.print(","); }
	}
	Serial.println();
}
//===============================================================================================
String HelpMenu(void) {
	String temp;
	temp += F("\n\r");
	temp += Line();//====================================================================
	temp += F("HELP MENU\n\r");
	temp += F("\n\r");
	temp += F("FUNCTIONAL DESCRIPTION:\n\r");
	temp += F("\t18650 Internal Reistance Tester\n\r");
	temp += F("\Place ONLY ONE battery in the cradle.  \n\r");
	temp += F("\n\r");
	temp += F("COMMANDS:\n\r");
	temp += F("\t'debug' - to turn debugging code on/off\n\r");
	temp += F("\t'm,XXX' - Where XXX is an integer for something blah blah\n\r");
	temp += F("\t't,XXX,YYY' - Where XXX is an integer for something, and YYY is an integer for something blah blah\n\r");
	temp += F("\n\r");
	temp += F("For additional information please contact YOURNAME\n\r");
	temp += Line();//====================================================================
	temp += F("\n\r");
	return temp;
}
//===============================================================================================
String Line(void) {
	return F("===============================================================================================\n\r");
}
//===============================================================================================