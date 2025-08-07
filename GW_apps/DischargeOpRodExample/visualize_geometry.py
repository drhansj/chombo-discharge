import re
import sys
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

INPUT_FILE = "doubleElectrode_domLeng1_dx128_amr1lev4ref_works.inputs"

def parse_cylinders(filename):
    with open(filename) as f:
        lines = f.readlines()
    cylinders = {}
    for line in lines:
        line = line.strip()
        m = re.match(r"DoubleStl\.(cylinder\d*)(?:\.(\w+))?\s*=\s*(.*)", line)
        if not m:
            m = re.match(r"DoubleStl\.(cylinder)(?:\.(\w+))?\s*=\s*(.*)", line)
        if m:
            name, param, value = m.groups()
            if name not in cylinders:
                cylinders[name] = {"name": name}
            if param == "on":
                cylinders[name]["on"] = value.lower() == "true"
            elif param == "radius":
                cylinders[name]["radius"] = float(value)
            elif param == "bot":
                cylinders[name]["bot"] = [float(x) for x in value.split()]
            elif param == "top":
                cylinders[name]["top"] = [float(x) for x in value.split()]
            elif param == "live":
                cylinders[name]["live"] = value.lower() == "true"
            elif param == "permittivity":
                cylinders[name]["permittivity"] = float(value)
    # Only keep cylinders that are 'on'
    return [c for c in cylinders.values() if c.get("on", False)]

def plot_cylinder(ax, bot, top, radius, color, alpha=0.5, label=None):
    bot = np.array(bot)
    top = np.array(top)
    v = top - bot
    mag = np.linalg.norm(v)
    if mag == 0:
        return
    v = v / mag
    # Create basis
    not_v = np.array([1, 0, 0]) if abs(v[0]) < 0.99 else np.array([0, 1, 0])
    n1 = np.cross(v, not_v)
    n1 /= np.linalg.norm(n1)
    n2 = np.cross(v, n1)
    # Create circle
    t = np.linspace(0, 2 * np.pi, 30)
    circle = np.array([radius * np.cos(t), radius * np.sin(t), np.zeros_like(t)])
    # Create cylinder
    n_steps = 20
    for i in np.linspace(0, mag, n_steps):
        center = bot + v * i
        points = center[:, None] + n1[:, None] * circle[0] + n2[:, None] * circle[1]
        ax.plot(points[0], points[1], points[2], color=color, alpha=alpha)
    # Draw ends
    for end in [bot, top]:
        center = end
        points = center[:, None] + n1[:, None] * circle[0] + n2[:, None] * circle[1]
        ax.plot(points[0], points[1], points[2], color=color, alpha=alpha)
    if label:
        ax.text(*center, label, color=color)

def main():
    cylinders = parse_cylinders(INPUT_FILE)
    print("Parsed cylinders:", cylinders)  # Debug print
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    colors = ['r', 'g', 'b', 'c', 'm', 'y']
    for i, cyl in enumerate(cylinders):
        color = colors[i % len(colors)]
        label = cyl['name']
        if cyl.get('live', False):
            color = 'orange'
        plot_cylinder(ax, cyl['bot'], cyl['top'], cyl['radius'], color, label=label)
    ax.set_xlabel('X')
    ax.set_ylabel('Y')
    ax.set_zlabel('Z')
    ax.set_title('Input Geometry Visualization')
    plt.savefig("geometry.png")
    print("Plot saved as geometry.png")

if __name__ == "__main__":
    main() 