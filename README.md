# InternalResistanceTester
Lithium Battery Internal Resistance Tester

Internal Battery Reistance Tester
Terry Myers 
Schematic: https://easyeda.com/terryjmyers/InternalResistanceTester-3aa0731b58d94ed5bb953af61ea30eb0


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
