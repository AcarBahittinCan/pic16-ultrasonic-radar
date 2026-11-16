"""
EE321 Ultrasonic Radar - Real-Time Polar Plot GUI (v10)
Orijin ortada, 1. ve 2. bolge
MAX_DISTANCE sabit 50 cm olarak ayarlanmistir.

Renk zones:
    0  < dist < 10  -> KIRMIZI
    10 < dist < 25  -> SARI
    25 < dist < 50  -> YESIL
    dist >= 50      -> gosterilmez

Kullanim:
    python radar_gui.py test
    python radar_gui.py COM9
"""

import sys
import time
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque

# ============================================================
# Ayarlar
# ============================================================
FADE_SWEEPS        = 3
BEAM_TRAIL_ANGLE   = 15
UPDATE_INTERVAL    = 10
FIXED_MAX_DISTANCE = 50
STEP_SIZE          = 2

# Renk esikleri
DIST_RED    = 10
DIST_YELLOW = 25
DIST_GREEN  = 50

# ============================================================
# Veri deposu
# ============================================================
raw_history   = [deque(maxlen=2) for _ in range(361)]
distances     = np.full(361, np.nan)
ages          = np.full(361, 0.0)
current_angle = 0
data_queue    = deque(maxlen=50)


def get_color(distance):
    if distance < DIST_RED:
        return (1.0, 0.0, 0.0)
    elif distance < DIST_YELLOW:
        return (1.0, 1.0, 0.0)
    else:
        return (0.0, 1.0, 0.0)


def parse_line(line):
    try:
        line = line.strip()
        if not line.startswith('A:'):
            return None
        parts     = line.split(',')
        angle_str = parts[0][2:]
        dist_str  = parts[1][2:]
        angle     = int(angle_str)

        if dist_str == '---':
            distance = None
        else:
            distance = int(dist_str)
            if distance >= FIXED_MAX_DISTANCE:
                distance = None

        return (angle, distance)
    except (ValueError, IndexError):
        return None


# ============================================================
# Seri port okuyucu
# ============================================================
def serial_reader(port, baud=9600):
    import serial
    try:
        ser = serial.Serial(port, baud, timeout=1)
        while True:
            try:
                line = ser.readline().decode('ascii', errors='ignore').strip()
                if line:
                    print(f"[UART] {line}")
                    data_queue.append(line)
            except Exception as e:
                print(f"Seri port okuma hatasi: {e}")
    except serial.SerialException as e:
        print(f"Seri port acilamadi ({port}): {e}")
        sys.exit(1)


# ============================================================
# Test modu
# ============================================================
def test_data_generator():
    angle     = 0
    direction = 1
    fake_objects = {
        10: 8,  12: 7,  14: 9,
        50: 18, 52: 20, 54: 17,
        100: 35, 102: 36, 104: 34,
        150: 45, 152: 44,
    }

    while True:
        if angle in fake_objects:
            dist = fake_objects[angle] + np.random.randint(-1, 2)
            data_queue.append(f"A:{angle:03d},D:{dist:03d}")
        else:
            data_queue.append(f"A:{angle:03d},D:---")

        angle += direction * STEP_SIZE
        if angle > 180:
            angle     = 180
            direction = -1
        elif angle < 0:
            angle     = 0
            direction = 1

        time.sleep(0.05)


# ============================================================
# Polar plot
# ============================================================
fig = plt.figure(figsize=(10, 6), facecolor='#0a0a0a')
fig.canvas.manager.set_window_title('EE321 Ultrasonic Radar')
ax = fig.add_subplot(111, polar=True, facecolor='#0a0a0a')

ax.set_theta_zero_location('E')
ax.set_theta_direction(1)
ax.set_thetamin(0)
ax.set_thetamax(180)
ax.set_ylim(0, FIXED_MAX_DISTANCE)

ax.tick_params(colors='#00ff00', labelsize=8)
ax.grid(color='#004400', linewidth=0.5, alpha=0.7)
ax.spines['polar'].set_color('#006600')

angle_labels = ['0°', '30°', '60°', '90°', '120°', '150°', '180°']
ax.set_thetagrids([0, 30, 60, 90, 120, 150, 180], labels=angle_labels)

distance_ticks = [10, 20, 30, 40, 50]
ax.set_rticks(distance_ticks)
ax.set_rlabel_position(0)

for tick in distance_ticks:
    ax.text(np.pi, tick, str(tick), color='#00ff00', fontsize=8,
            ha='center', va='center', backgroundcolor='#0a0a0a')

# Zone cemberleri
theta_full = np.linspace(0, np.pi, 200)
ax.plot(theta_full, np.full(200, DIST_RED),    color='red',    linewidth=0.8, alpha=0.4, linestyle='--')
ax.plot(theta_full, np.full(200, DIST_YELLOW), color='yellow', linewidth=0.8, alpha=0.4, linestyle='--')
ax.plot(theta_full, np.full(200, DIST_GREEN),  color='green',  linewidth=0.8, alpha=0.4, linestyle='--')

beam_line,     = ax.plot([], [], color='#00ff00', linewidth=2, alpha=0.9)
beam_trail,    = ax.plot([], [], color='#00ff00', linewidth=8, alpha=0.15)
object_scatter = ax.scatter([], [], s=[], zorder=5)

ax.set_title('EE321 ULTRASONIC RADAR', color='#00ff00',
             fontsize=14, fontweight='bold', pad=20, fontfamily='monospace')

info_text = fig.text(0.5, 0.02, '', color='#00ff00', fontsize=10,
                     fontfamily='monospace', ha='center')

# Renk aciklamasi
fig.text(0.01, 0.92, '■ < 10cm', color='red',    fontsize=9, fontfamily='monospace')
fig.text(0.01, 0.87, '■ < 25cm', color='yellow', fontsize=9, fontfamily='monospace')
fig.text(0.01, 0.82, '■ < 50cm', color='green',  fontsize=9, fontfamily='monospace')


# ============================================================
# Animasyon guncelleme
# ============================================================
def update(frame):
    global current_angle, distances, ages

    processed = 0
    while data_queue and processed < 10:
        line   = data_queue.popleft()
        result = parse_line(line)
        if result is None:
            continue
        angle, distance = result
        current_angle   = angle

        if distance is not None:
            raw_history[angle].append(distance)
            distances[angle] = float(distance)
            ages[angle]      = 0.0
        else:
            raw_history[angle].clear()
            distances[angle] = np.nan

        processed += 1

    # Ages guncelle
    valid = ~np.isnan(distances)
    ages[valid] += 0.02

    # --- Beam & Trail ---
    beam_rad = np.radians(current_angle)
    beam_line.set_data([beam_rad, beam_rad], [0, FIXED_MAX_DISTANCE])
    trail_angles = np.linspace(beam_rad - np.radians(BEAM_TRAIL_ANGLE), beam_rad, 20)
    beam_trail.set_data(trail_angles, np.full_like(trail_angles, FIXED_MAX_DISTANCE))

    # --- Nesne noktalari ---
    thetas      = []
    radii       = []
    rgba_colors = []
    sizes       = []

    for deg in range(0, 181):
        if not np.isnan(distances[deg]):
            age   = ages[deg]
            alpha = max(0.0, 1.0 - age / FADE_SWEEPS)
            if alpha > 0.05:
                dist    = distances[deg]
                r, g, b = get_color(dist)
                thetas.append(np.radians(deg))
                radii.append(dist)
                rgba_colors.append([r, g, b, alpha])
                sizes.append(max(10, 80 - dist * 0.5))

    if thetas:
        object_scatter.set_offsets(np.column_stack([thetas, radii]))
        object_scatter.set_facecolors(np.array(rgba_colors))
        object_scatter.set_edgecolors(np.array(rgba_colors))
        object_scatter.set_sizes(np.array(sizes))
    else:
        object_scatter.set_offsets(np.empty((0, 2)))
        object_scatter.set_facecolors(np.empty((0, 4)))
        object_scatter.set_edgecolors(np.empty((0, 4)))
        object_scatter.set_sizes(np.array([]))

    info_text.set_text(f"ANG: {current_angle:3d}°  |  RANGE: {FIXED_MAX_DISTANCE}cm")

    return beam_line, beam_trail, object_scatter, info_text


# ============================================================
# Ana program
# ============================================================
if __name__ == '__main__':
    import threading

    if len(sys.argv) < 2:
        print("Kullanim:")
        print("  python radar_gui.py COM9    (seri port)")
        print("  python radar_gui.py test    (test modu)")
        sys.exit(1)

    mode = sys.argv[1]

    if mode.lower() == 'test':
        print(">> Test modu baslatiliyor...")
        t = threading.Thread(target=test_data_generator, daemon=True)
        t.start()
    else:
        print(f">> Seri port aciliyor: {mode}")
        t = threading.Thread(target=serial_reader, args=(mode,), daemon=True)
        t.start()

    ani = animation.FuncAnimation(fig, update, interval=UPDATE_INTERVAL,
                                   blit=False, cache_frame_data=False)
    plt.tight_layout()
    plt.show()