import sys
import threading
from PyQt5.QtWidgets import QApplication
from frontend import GraphWindow
from backend import SerialDataHandler

def main():
    app = QApplication(sys.argv)
    backend = SerialDataHandler(port="/dev/ttyUSB0", baudrate=9600)
    # Start the serial connection in a separate thread to avoid blocking the UI
    threading.Thread(target=backend.connect, daemon=True).start()
    window = GraphWindow(backend)
    window.show()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
