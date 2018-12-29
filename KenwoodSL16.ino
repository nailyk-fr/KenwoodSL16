/*
Kenwood XS System Control
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

\        |----|    |--|  |--|    |--|
\ ___----|    |----|  |--|  |----|  |____ Data signal

\    |    |  Start:
\ Busy up 5ms, then:
\ Data up 5ms, then down 3ms then:
\                | 0  |  0  |    1  |
\ 1: 4ms low 2ms high
\ 0: 2ms low 2ms high

\ Data is MSB first 16 bits.
\ After transmitting data, both busy and data come down at same time. (also, become inputs, not output).

Be aware that this could be run as a shared bus, so only output when you control the bus. Perhaps?

CTRL goes high
Some time later (2ms?) SDAT goes high for 5msec
SDAT goes low for 5msec

For each 0 bit, SDAT goes high for 2msec, then low for 2msec. Or for 1 bit, SDAT stays low for 4 msec.
After at least 20 bits (maybe more?), SDAT stays low, and some time later, CTRL goes low.
NOTE: It seems that things are somewhat variable, since CTRL stays on between 80 and 90 msec, depending on command.

Example word for Video1,2,3:
111111111 11010100, where 1 is 5 Volts, 0 is 0 Volts.
000000000 00101011 or number 43
Example word for Tape:
11111111 01011010 =>
00000000 10100101 or number 165
******************End of SL16******************
*/

const unsigned long MSB = 1l << 15; // 16 bits

enum {
  SDAT = 2,
  CTRL = 3,
  BIT_ONE_DELAY_MICROSEC = 3100,
  BIT_TERMINATOR_DELAY_MICROSEC = 1950,
  BIT_GAP_DELAY_MICROSEC = 1950,
};

void setup() {
  Serial.begin(115200);

  // Set-up XS lines
  pinMode(CTRL, INPUT);
  pinMode(SDAT, INPUT);
  digitalWrite(CTRL, LOW);
  digitalWrite(SDAT, LOW);

  // Usage
  Serial.print("MSB: ");
  Serial.print(MSB, DEC);
  Serial.print(" / 0x");
  Serial.println(MSB, HEX);

  Serial.println("Kenwood KX-3050 commands:");
  Serial.println("  Commands working in both the power-on mode and stand-by mode (in decimal):");
  Serial.println("    121 - play");
  Serial.println("    112, 113, 115, 117, 122, 123, 125 - stop");
  Serial.println("");
  Serial.println("  Commands working only in the power-on mode (in decimal):");
  Serial.println("    66 - search next track");
  Serial.println("    68 - stop");
  Serial.println("    70 - play if stopped or paused, repeat current song if playing");
  Serial.println("    72 - record");
  Serial.println("    74 - search previous track");
  Serial.println("    76 - pause");
  Serial.println("");
  Serial.println("Now type:");
  Serial.println("  value 0-255 to send the corresponding command,");
  Serial.println("  value >255 to start a loop to automatically try all commands with delay of 'value' ms.");
}

void sendWord(unsigned long word) {
  // StartBit
  digitalWrite(SDAT, HIGH);
  delayMicroseconds(5000);
  digitalWrite(SDAT, LOW);
  delayMicroseconds(3000);

  for (unsigned long mask = MSB; mask; mask >>= 1) {
    digitalWrite(SDAT, LOW);
    delayMicroseconds(BIT_GAP_DELAY_MICROSEC);
    // Bit
    if (word & mask) {
    	// This is a 1 bit.
//	    digitalWrite(SDAT, LOW);
	    delayMicroseconds(BIT_ONE_DELAY_MICROSEC);
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

  digitalWrite(CTRL, HIGH);
  delayMicroseconds(3000);

  sendWord(word);

  // Return to default state
  digitalWrite(SDAT, LOW);
  delayMicroseconds(2000);
  digitalWrite(CTRL, LOW);
  pinMode(SDAT, INPUT);
  pinMode(CTRL, INPUT);
}

void tryAllWords(unsigned long wait) {
  for (unsigned long cmd = 0; cmd < 512; cmd++) {
    sendCommand(cmd);
    delay(wait);
  }
}

void loop() {
  while (Serial.available()) {
    const long val = Serial.parseInt();
    if (val == 0) {
    	continue;
    }
    if (val < 0) {
      tryAllWords((unsigned long)(-val));
    } else if (val > 0 && val < (MSB << 1)) {
      sendCommand((unsigned long)(val));
    } else {
    	Serial.println("Unexpected Value");
    	Serial.println(val);
    }
  }
}
