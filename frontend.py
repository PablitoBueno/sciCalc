import matplotlib
matplotlib.use("Qt5Agg")  # Força o uso do backend Qt5Agg para PyQt5
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from PyQt5.QtWidgets import QMainWindow, QVBoxLayout, QWidget, QGraphicsDropShadowEffect
from PyQt5.QtGui import QColor
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas, NavigationToolbar2QT as NavigationToolbar

# Configuração global da fonte para Matplotlib
plt.rcParams['font.family'] = 'DejaVu Sans Mono'
plt.rcParams['font.size'] = 10

class GraphWindow(QMainWindow):
    def __init__(self, backend):
        super().__init__()
        self.backend = backend

        self.setWindowTitle("SciCalc - Graphic")
        self.setGeometry(100, 100, 700, 500)
        self.setStyleSheet("background-color: #2E2E2E; color: white; border: 3px solid #007ACC;")

        self.central_widget = QWidget(self)
        self.setCentralWidget(self.central_widget)
        layout = QVBoxLayout(self.central_widget)

        self.fig, self.ax = plt.subplots(figsize=(7, 7))
        self.fig.patch.set_facecolor("#2E2E2E")
        self.canvas = FigureCanvas(self.fig)
        self.canvas.setStyleSheet("border: 2px solid #007ACC;")
        layout.addWidget(NavigationToolbar(self.canvas, self))  # Adiciona a toolbar do Matplotlib
        layout.addWidget(self.canvas)

        shadow = QGraphicsDropShadowEffect(self.canvas)
        shadow.setBlurRadius(20)
        shadow.setXOffset(7)
        shadow.setYOffset(7)
        shadow.setColor(QColor(0, 0, 0, 180))
        self.canvas.setGraphicsEffect(shadow)

        self.setup_plot()

        self.ani = animation.FuncAnimation(self.fig, self.update_graph, interval=500)

    def setup_plot(self):
        self.ax.clear()
        self.ax.set_facecolor("#2E2E2E")

        for spine in self.ax.spines.values():
            spine.set_edgecolor("#007ACC")
            spine.set_linewidth(2)

        self.ax.axhline(0, color='#007ACC', linewidth=1)
        self.ax.axvline(0, color='#007ACC', linewidth=1)
        self.ax.set_xticks(range(-10, 11, 1))
        self.ax.set_yticks(range(-10, 11, 1))
        self.ax.grid(True, linestyle='--', linewidth=0.5, color='#3C3C3C')
        self.ax.minorticks_on()
        self.ax.grid(which='minor', linestyle=':', linewidth=0.5, color='#3C3C3C')
        self.ax.set_xlim(-10, 10)
        self.ax.set_ylim(-10, 10)
        self.ax.set_title("SciCalc - Graphic", color='#D4D4D4', fontsize=16, fontweight='bold')
        self.ax.tick_params(axis='both', labelsize=10, colors='#D4D4D4')

    def update_graph(self, frame):
        self.setup_plot()
        for x, y in self.backend.get_points():
            self.ax.scatter(x, y, color='#66FF66', s=60, zorder=5)
            self.ax.plot([x, x], [0, y], linestyle='dashed', color='#007ACC', linewidth=0.8)
            self.ax.plot([0, x], [y, y], linestyle='dashed', color='#007ACC', linewidth=0.8)
            self.ax.text(x + 0.2, y + 0.2, f"({x:.2f}, {y:.2f})", fontsize=10, color='#D4D4D4', fontweight='bold')
        self.canvas.draw()
