#!/usr/bin/python3
import sys
import serial
import time

def main():
    arduino='/dev/ttyUSB0'
    data = sys.argv


    # Strangest thing : https://stackoverflow.com/questions/21073086/wait-on-arduino-auto-reset-using-pyserial
    ser = serial.Serial(arduino)
    ser.setDTR(False)
    ser.flushInput()
    ser.setDTR(True)
    time.sleep(5)
    ## Once board is restarted, we can start talking to it

    if data[1:]:
        ser = serial.Serial(arduino,
                     115200,
                     bytesize=serial.EIGHTBITS,
                     parity=serial.PARITY_NONE,
                     stopbits=serial.STOPBITS_ONE,
                     timeout=1,
                     xonxoff=0,
                     rtscts=0
                     )
        print("Sending " + str(data[1]) + " \n")
        ser.write(data[1].encode())
        ser.flush()
        ser.close()

    else:
        print("No Input given!\n")

#    while True:
#        if ser.in_waiting > 0:
#            buffer = ser.read(ser.in_waiting)
#            print('ascii=', repr(buffer))
#    ser.close()

if __name__ == "__main__":
    main()
