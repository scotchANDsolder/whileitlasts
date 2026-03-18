import serial
import time
import json
import os
import threading

SERIAL_PORT = '/dev/ttyACM0' 
CONFIG_FILE = 'winch_config.json'

class WinchMaster:
    def __init__(self):
        self.settings = self.load_settings()
        self.current_pos = 0
        self.current_state = "IDLE"
        self.running = True
        try:
            self.ser = serial.Serial(SERIAL_PORT, 115200, timeout=0.1)
            time.sleep(2) 
            threading.Thread(target=self.serial_listener, daemon=True).start()
        except Exception as e:
            print(f"Error: {e}"); exit()

    def load_settings(self):
        if os.path.exists(CONFIG_FILE):
            with open(CONFIG_FILE, 'r') as f: return json.load(f)
        return {"speed": 1000, "unwind": 3000}

    def serial_listener(self):
        while self.running:
            try:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if "POS:" in line and "|STATE:" in line:
                    parts = line.split("|")
                    self.current_pos = int(parts[0].split(":")[1])
                    self.current_state = parts[1].split(":")[1]
                if "ERROR_LIMIT" in line:
                    print("\n[!] SAFETY ERROR: Motor moved too far past magnet. [!]")
            except: pass

    def run_calibration(self):
        print("\n" + "="*45)
        print("   PRECISION CALIBRATION MODE")
        print("="*45)
        print(" a  / s  : FAST Move (500 steps)")
        print(" aa / ss : SLOW Move (50 steps)")
        print(" r       : Set TOP (Zero Here)")
        print(" d       : Save & Exit")
        print("-" * 45)
        self.ser.write(b"CAL\n")
        
        while True:
            cmd = input(f"[Pos: {self.current_pos} | State: {self.current_state}] -> ").strip().lower()
            if cmd == 'a': self.ser.write(b"MOVE,-500\n")
            elif cmd == 'aa': self.ser.write(b"MOVE,-50\n")
            elif cmd == 's': self.ser.write(b"MOVE,500\n")
            elif cmd == 'ss': self.ser.write(b"MOVE,50\n")
            elif cmd == 'r':
                self.ser.write(b"SET_ZERO\n")
                print(">> Resetting Zero...")
                time.sleep(0.5)
            elif cmd == 'd':
                self.settings['unwind'] = abs(self.current_pos)
                with open(CONFIG_FILE, 'w') as f: json.dump(self.settings, f)
                print(f">> SAVED: {self.settings['unwind']} steps")
                break

    def main_menu(self):
        while self.running:
            print(f"\n--- WINCH STATUS: [{self.current_state}] ---")
            print(f"Target Unwind: {self.settings['unwind']} steps | Pos: {self.current_pos}")
            print("1. Start Auto  | 2. Calibrate | 3. STOP | 4. Exit")
            choice = input("Select: ")
            if choice == '1':
                self.ser.write(b"STOP\n")
                time.sleep(0.1)
                self.ser.write(f"{self.settings['speed']},{self.settings['unwind']}\n".encode())
                time.sleep(0.1)
                self.ser.write(b"START\n")
            elif choice == '2': self.run_calibration()
            elif choice == '3': self.ser.write(b"STOP\n")
            elif choice == '4': self.running = False; break

if __name__ == "__main__":
    WinchMaster().main_menu()
