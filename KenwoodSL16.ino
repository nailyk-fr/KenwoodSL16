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
10000000 00101011 or number 32768 + 43 = 32811
Example word for Tape:
01111111 01011010 =>
10000000 10100101 or number 32768 + 165 = 32933
Sending these commands to VR-410 seems to work:
4096 is power on 
4224 is power off
******************End of SL16******************
*/

const unsigned long MSB = 1l << 15; // 16 bits

enum {
  SDAT = 2,
  CTRL = 3,
  BIT_ONE_DELAY_MICROSEC = 3100,
  BIT_TERMINATOR_DELAY_MICROSEC = 1950,
  BIT_GAP_DELAY_MICROSEC = 1950,
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
  Serial.println("4096 is power on");
  Serial.println("4224 is power off");
  Serial.println("Type:");
  Serial.println("  value 0-65535 to send the corresponding command,");
  Serial.println("  value less than zero to start a loop to automatically try all commands with delay of '-value' ms.");
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
  for (unsigned long cmd = 4097; cmd <= 4224; cmd++) {
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
