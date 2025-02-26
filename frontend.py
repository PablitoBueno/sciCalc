import sys
import numpy as np
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

# Subclasse customizada da NavigationToolbar que sobrescreve os métodos back() e forward()
class CustomNavigationToolbar(NavigationToolbar):
    def __init__(self, canvas, parent, graph_window):
        super().__init__(canvas, parent)
        self.graph_window = graph_window

    def back(self):
        # Chama a alternância de modo quando o botão Back é pressionado
        self.graph_window.toggle_mode()

    def forward(self):
        # Chama a alternância de modo quando o botão Forward é pressionado
        self.graph_window.toggle_mode()

class GraphWindow(QMainWindow):
    def __init__(self, backend):
        super().__init__()
        self.backend = backend
        self.mode = 0  # 0: plano cartesiano; 1: gráfico de parábolas

        # Estilo Gamer Futurista
        self.setWindowTitle("SciCalc - Graphic")
        self.setGeometry(100, 100, 800, 600)
        self.setStyleSheet("""
            QMainWindow {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                            stop:0 #0f0f0f, stop:1 #1a1a1a);
                border: 3px solid #00FFAA;
            }
        """)

        self.central_widget = QWidget(self)
        self.central_widget.setStyleSheet("background: transparent;")
        self.setCentralWidget(self.central_widget)
        layout = QVBoxLayout(self.central_widget)
        layout.setContentsMargins(5, 5, 5, 5)

        # Cria figura e canvas com aparência futurista
        self.fig, self.ax = plt.subplots(figsize=(8, 8))
        self.fig.patch.set_facecolor("#0f0f0f")
        self.canvas = FigureCanvas(self.fig)
        self.canvas.setStyleSheet("border: 2px solid #00FFAA;")

        # Cria a toolbar customizada (não há referência a back_action aqui)
        self.toolbar = CustomNavigationToolbar(self.canvas, self, self)
        layout.addWidget(self.toolbar)
        layout.addWidget(self.canvas)

        # Aplica efeito de sombra para profundidade
        shadow = QGraphicsDropShadowEffect(self.canvas)
        shadow.setBlurRadius(20)
        shadow.setXOffset(7)
        shadow.setYOffset(7)
        shadow.setColor(QColor(0, 255, 170, 180))
        self.canvas.setGraphicsEffect(shadow)

        self.setup_plot()
        self.ani = animation.FuncAnimation(self.fig, self.update_graph, interval=500)

    def setup_plot(self):
        self.ax.clear()
        self.ax.set_facecolor("#0f0f0f")
        # Estilização dos eixos e spines
        for spine in self.ax.spines.values():
            spine.set_edgecolor("#00FFAA")
            spine.set_linewidth(2)
        self.ax.axhline(0, color='#00FFAA', linewidth=1)
        self.ax.axvline(0, color='#00FFAA', linewidth=1)
        self.ax.set_xticks(range(-10, 11, 1))
        self.ax.set_yticks(range(-10, 11, 1))
        self.ax.grid(True, linestyle='--', linewidth=0.5, color='#005f5f')
        self.ax.minorticks_on()
        self.ax.grid(which='minor', linestyle=':', linewidth=0.5, color='#005f5f')
        self.ax.set_xlim(-10, 10)
        self.ax.set_ylim(-10, 10)
        self.ax.set_title("SciCalc - Graphic", color='#00FFAA', fontsize=18, fontweight='bold')
        self.ax.tick_params(axis='both', labelsize=10, colors='#00FFAA')

    def update_graph(self, frame):
        self.setup_plot()
        if self.mode == 0:
            # Modo 0: Exibe os pontos do backend no plano cartesiano
            for x, y in self.backend.get_points():
                self.ax.scatter(x, y, color='#00FF00', s=80, zorder=5)
                self.ax.plot([x, x], [0, y], linestyle='--', color='#00FFAA', linewidth=0.8)
                self.ax.plot([0, x], [y, y], linestyle='--', color='#00FFAA', linewidth=0.8)
                self.ax.text(x + 0.2, y + 0.2, f"({x:.2f}, {y:.2f})",
                             fontsize=10, color='#00FFAA', fontweight='bold')
        elif self.mode == 1:
            # Modo 1: Exibe gráfico de parábolas
            x = np.linspace(-10, 10, 400)
            self.ax.plot(x, x**2, color='#FF00FF', linewidth=2, label="y = x²")
            self.ax.plot(x, 0.5 * x**2, color='#00FFFF', linewidth=2, label="y = 0.5x²")
            self.ax.plot(x, 2 * x**2, color='#FFFF00', linewidth=2, label="y = 2x²")
            self.ax.plot(x, -x**2, color='#FF0000', linewidth=2, label="y = -x²")
            self.ax.legend(facecolor="#0f0f0f", edgecolor="#00FFAA", labelcolor='#00FFAA')
        self.canvas.draw()

    def toggle_mode(self):
        # Alterna entre os modos e atualiza o gráfico imediatamente
        self.mode = (self.mode + 1) % 2
        self.update_graph(None)
