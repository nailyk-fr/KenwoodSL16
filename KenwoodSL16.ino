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
CTRL goes high
Some time later (2ms?) SDAT goes high for 5msec
SDAT goes low for 5msec
For each 0 bit, SDAT goes high for 2msec, then low for 2msec. Or for 1 bit, SDAT stays low for 4 msec.
After at least 20 bits (maybe more?), SDAT stays low, and some time later, CTRL goes low.
NOTE: It seems that things are somewhat variable, since CTRL stays on between 80 and 90 msec, depending on command.

Example word for Video1,2,3:
1111 11111110 11011010, where 1 is 5 Volts, 0 is 0 Volts.
0000 00000001 00100101 or number 256 + 37 or 293
Example word for Tape:
1111 11110110 11101101 =>
0000 00001001 00010010 or number 2048 + 256 + 16 + 2 or 2322
******************End of SL16******************
*/

const unsigned long MAX_WORD = 1l << 19; // 20 bits

enum {
  SDAT = 2,
  CTRL = 3,
  BIT_ON_DELAY_MICROSEC = 1950,
  BIT_OFF_DELAY_MICROSEC = 1450,
};

void setup() {
  Serial.begin(115200);

  // Set-up XS lines
  pinMode(CTRL, INPUT);
  pinMode(SDAT, INPUT);
  digitalWrite(CTRL, LOW);
  digitalWrite(SDAT, LOW);

  // Usage
  Serial.print("Max Word: ");
  Serial.print(MAX_WORD, DEC);
  Serial.print(" / 0x");
  Serial.println(MAX_WORD, HEX);

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
  delayMicroseconds(5000);

  for (unsigned long mask = MAX_WORD; mask; mask >>= 1) {
    // Bit
    if (word & mask) {
	    digitalWrite(SDAT, LOW);
	    delayMicroseconds(BIT_OFF_DELAY_MICROSEC);
    } else {
    	digitalWrite(SDAT, HIGH);
	    delayMicroseconds(BIT_ON_DELAY_MICROSEC);
    }
    digitalWrite(SDAT, LOW);
    delayMicroseconds(BIT_ON_DELAY_MICROSEC);
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
  delayMicroseconds(1000);
  digitalWrite(SDAT, LOW);
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
    } else if (val > 0 && val <= MAX_WORD) {
      sendCommand((unsigned long)(val));
    } else {
    	Serial.println("Unexpected Value");
    	Serial.println(val);
    }
  }
}
