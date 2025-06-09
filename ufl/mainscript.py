import serial
import time
import signal
import sys

arduino = serial.Serial('/dev/ttyACM0', 9600)
time.sleep(2)  # Let Arduino reset

# Send 'G' to start moving forward
print("Sending 'G' to Arduino to start motion.")
arduino.write(b'G')

def stop_motor(signum, frame):
    print(f"Received shutdown signal ({signum}). Sending 'S' to stop motor.")
    try:
        arduino.write(b'S')
        time.sleep(0.5)
    except Exception as e:
        print(f"Error sending stop command: {e}")
    finally:
        arduino.close()
        sys.exit(0)

# Catch termination signals from systemd and Ctrl+C
signal.signal(signal.SIGTERM, stop_motor)  # systemctl stop
signal.signal(signal.SIGINT, stop_motor)   # Ctrl+C

try:
    while True:
        if arduino.in_waiting > 0:
            line = arduino.readline().decode('utf-8').strip()
            print(f"Arduino: {line}")
        time.sleep(0.1)

except KeyboardInterrupt:
    stop_motor(signal.SIGINT, None)
