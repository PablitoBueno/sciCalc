import sys
from PyQt5.QtWidgets import QApplication
from frontend import GraphWindow
from backend import SerialDataHandler

def main():
    app = QApplication(sys.argv)
    backend = SerialDataHandler(port="/dev/ttyUSB0", baudrate=9600)
    backend.connect()
    window = GraphWindow(backend)
    window.show()
    sys.exit(app.exec_())

if __name__ == "__main__":
    main()
