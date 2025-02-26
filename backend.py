import serial
import threading
import logging
import time

# Logging configuration
logging.basicConfig(level=logging.INFO, format="- %(levelname)s - %(message)s")

class SerialDataHandler:
    def __init__(self, port="/dev/ttyUSB0", baudrate=9600):
        self.port = port
        self.baudrate = baudrate
        self.serial_connection = None
        self.running = False
        self.points = []  # List to store (x, y) points
        self.data_callback = None

    def connect(self):
        printed_wait_message = False
        while not self.running:
            try:
                self.serial_connection = serial.Serial(self.port, self.baudrate, timeout=1)
                self.running = True
                threading.Thread(target=self.read_data, daemon=True).start()
                logging.info(f"Connected to {self.port} at {self.baudrate} baud.")
                break
            except serial.SerialException:
                if not printed_wait_message:
                    logging.warning("USB not connected. Waiting for connection...")
                    printed_wait_message = True
                time.sleep(2)

    def read_data(self):
        while self.running:
            try:
                if self.serial_connection and self.serial_connection.in_waiting > 0:
                    line = self.serial_connection.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # If the received command is "toggle" (case-insensitive)
                        if line.lower() == "toggle":
                            logging.info("Received 'toggle' command.")
                            if self.data_callback:
                                self.data_callback("toggle")
                        else:
                            parts = line.split()
                            if len(parts) >= 2:
                                try:
                                    x = float(parts[0])
                                    y = float(parts[1])
                                    self.points.append((x, y))
                                    if self.data_callback:
                                        self.data_callback((x, y))
                                except ValueError:
                                    logging.error(f"Invalid format received: {line}")
            except serial.SerialException:
                logging.error("Serial communication error. Attempting to reconnect...")
                self.reconnect()
            except Exception as e:
                logging.exception(f"Unexpected error: {e}")

    def reconnect(self):
        self.running = False
        if self.serial_connection:
            self.serial_connection.close()
        time.sleep(2)
        self.connect()

    def set_callback(self, callback):
        self.data_callback = callback

    def get_points(self):
        return self.points

    def disconnect(self):
        self.running = False
        if self.serial_connection:
            self.serial_connection.close()
            logging.info("Serial connection closed.")
