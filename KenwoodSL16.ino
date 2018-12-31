/*
Kenwood SL-16 System Control
See XS-Connection.jpg for pinout.

Original code from: https://github.com/saproj/KenwoodXS
The following is XS8 I believe.
******************XS8******************
Based on reverse engineering of the protocol done by Olaf Such (ol...@ihf.rwth-aachen.de).
The following excerpts were taken from de.sci.electronics posts by Olaf from 1997.

"Spezifikationen:
positive Logik, TTL Pegel, Startbit 15ms, 8Bit daten, danach geht
CTRL wieder auf 0V"

"The code consists of: CTRL goes High, 2ms later, SDAT goes High for about
15ms (StartBit), then a Low on SDAT for either approx. 15ms or 7.5ms 
followed by a High level on SDAT for 7.5ms (FrameSignal)
then Low again for 15 or 7.5ms, High for 7.5, etc. 8 times all together.
CTRL then returns to Low."
******************End of XS8******************

My reverse engineering of SL16 on Kenwood VR410 using Oscilloscope and above info.
******************SL16******************
Based on https://sourceforge.net/p/amforth/mailman/message/34859069/
\ Kenwood SL16 protocol

\   |-------------------------------|
\  _|                               |____ Busy signal

\        |----|  |--|  |--|    |--|
\ ___----|    |--|  |--|  |----|  |____ Data signal

\    |    |  Start:
\ Busy up 5ms, then:
\ Data up 5ms, then:
\                | 0  |  0  |    1  |
\ 1: 4ms low 2ms high
\ 0: 2ms low 2ms high

\ Data is MSB first 16 bits.
\ After transmitting data, both busy and data come down at same time. (also, become inputs, not output).

Note - data zero is actually open collector, (or arduino input). IOW, there is no pull down, and the port just floats.
Be aware that this could be run as a shared bus, so only output when you control the bus. Perhaps?
CTRL stays on between 80 and 90 msec, depending on command.

Example word for Video1,2,3:
01111111 11010100, where 1 is 5 Volts, 0 is 0 then 5 Volts.
10000000 00101011 or number 32768 + 43 = 32811 (0x802B)
Example word for Tape:
01111111 01011010 =>
10000000 10100101 or number 32768 + 165 = 32933 (0x80A5)
01000011 01000001 is sent after power on - number 16384+512+256+64+1 = 17217 (

******************End of SL16******************
*/

const unsigned long MSB = 1l << 15; // 16 bits

enum {
  SDAT = 2,
  CTRL = 3,
  BIT_ONE_DELAY_MICROSEC = 3200,
  BIT_TERMINATOR_DELAY_MICROSEC = 2250,
  BIT_GAP_DELAY_MICROSEC = 2250,
  ENABLE_OPEN_COLLECTOR = 0,
};

void setup() {
  Serial.begin(115200);

  // Set-up SL lines
  digitalWrite(CTRL, LOW);
  digitalWrite(SDAT, LOW);
  pinMode(CTRL, INPUT);
  pinMode(SDAT, INPUT);

  // Usage
  Serial.print("MSB: ");
  Serial.print(MSB, DEC);
  Serial.print(" / 0x");
  Serial.println(MSB, HEX);

  Serial.println("Kenwood VR-410 SL-16 commands:");
  Serial.println("Sending these commands to VR-410 works:");
  Serial.println("4096 0x1000 activates power on");
  Serial.println("4224 0x1080 activates power off");
  Serial.println("1097 0x0499 activates TAPE input");
  Serial.println("2120 0x0848 activates CD/DVD input");
  Serial.println("63560 0xF848 activates PHONO input");
  Serial.println("Found this matching reference:");
  Serial.println("https://www.mikrocontroller.net/topic/101728");

  Serial.println("  value 0-65535 to send the corresponding command,");
  Serial.println("  range like 4096-4224 to send commands every 20 msec.");
}

void sendWord(unsigned long word) {
  // StartBit
  if (ENABLE_OPEN_COLLECTOR) {
	  pinMode(SDAT, OUTPUT);
  }
  digitalWrite(SDAT, HIGH);
  delayMicroseconds(5000);

  for (unsigned long mask = MSB; mask; mask >>= 1) {
    digitalWrite(SDAT, LOW);
	  if (ENABLE_OPEN_COLLECTOR) {
		  pinMode(SDAT, INPUT);
	  }
    delayMicroseconds(BIT_GAP_DELAY_MICROSEC);
    // Bit
    if (word & mask) {
    	// This is a 1 bit.
//	    digitalWrite(SDAT, LOW);
	    delayMicroseconds(BIT_ONE_DELAY_MICROSEC);
    }
	  if (ENABLE_OPEN_COLLECTOR) {
		  pinMode(SDAT, OUTPUT);
	  }
    digitalWrite(SDAT, HIGH);
	  delayMicroseconds(BIT_TERMINATOR_DELAY_MICROSEC);
  }
}

void sendCommand(unsigned long word) {
  pinMode(CTRL, OUTPUT);
  pinMode(SDAT, OUTPUT);
  Serial.print("Command ");
  Serial.print(word, DEC);
  Serial.print(" / 0x");
  Serial.println(word, HEX);

  digitalWrite(SDAT, LOW);
  if (ENABLE_OPEN_COLLECTOR) {
	  pinMode(SDAT, INPUT);
  }
  digitalWrite(CTRL, HIGH);
  delayMicroseconds(5000);

  sendWord(word);

  // Return to default state
  digitalWrite(SDAT, LOW);
  delayMicroseconds(2000);
  digitalWrite(CTRL, LOW);
  pinMode(SDAT, INPUT);
  pinMode(CTRL, INPUT);
}

void tryAllWords(unsigned long wait) {
  for (unsigned long cmd = 0; cmd <= 4096; cmd++) {
    sendCommand(cmd);
    delay(wait);
  }
}

void tryWordRange(unsigned long startValue, unsigned long endValue) {
	int increment = endValue >= startValue ? 1 : -1;
  for (unsigned long cmd = startValue; cmd != endValue + increment; cmd += increment) {
    sendCommand(cmd);
    delay(20);
  }
  Serial.println("Scan Finished: " + String(startValue) + "-" + String(endValue));
}

char input[65];
void loop() {
  while (Serial.available()) {
  	size_t length = Serial.readBytesUntil('\n', input, 64);
		input[length] = 0;
  	long startValue = -1;
  	long endValue = -1;
  	char *value = strchr(input, '-');
  	if (value != 0) {
  		*value = 0;
  		startValue = atol(input);
  		endValue = atol(value + 1);
  		tryWordRange(startValue, endValue);
  	} else {
  		startValue = atol(input);
  		sendCommand(startValue);
  	}
  }
}
