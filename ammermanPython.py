import serial
import time
import json
import os

SERIAL_PORT = '/dev/ttyACM0' 
CONFIG_FILE = 'winch_config.json'

settings = {"speed": 1000, "unwind": 3000}

def save_settings(data):
    with open(CONFIG_FILE, 'w') as f:
        json.dump(data, f)

def load_settings():
    if os.path.exists(CONFIG_FILE):
        with open(CONFIG_FILE, 'r') as f:
            return json.load(f)
    return settings

try:
    ser = serial.Serial(SERIAL_PORT, 115200, timeout=1)
    time.sleep(2) 
except:
    print("Error: Could not find Arduino.")
    exit()

settings = load_settings()
ser.write(f"{settings['speed']},{settings['unwind']}\n".encode())

print("--- CHAIR CONTROL ---")
print("Type 'stop' at any time to kill motor power.")

try:
    while True:
        print(f"\n[STATUS] Speed: {settings['speed']} | Unwind: {settings['unwind']}")
        val = input("Enter 'Speed,Steps' (or 'stop'): ").strip().lower()
        
        if val == 'stop':
            ser.write(b"STOP\n")
            print("!!! KILL COMMAND SENT !!!")
            continue

        if ',' in val:
            try:
                s, u = val.split(',')
                settings['speed'] = int(s)
                settings['unwind'] = int(u)
                save_settings(settings)
                ser.write(f"{settings['speed']},{settings['unwind']}\n".encode())
                print(">> Settings Updated.")
            except ValueError:
                print("Invalid format. Use: 1000,3000")
        else:
            print("Input not recognized. Use 'speed,steps' or 'stop'.")

except KeyboardInterrupt:
    ser.close()
