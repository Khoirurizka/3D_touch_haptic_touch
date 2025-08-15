import sys
import time
import math
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

import haptic

def detect_surface(pos):
    """Detect if the position is on or below a virtual plane (e.g., y = 0)."""
    plane_y = 0.0  # Define a plane at y = 0 mm
    if pos[1] <= plane_y:
        # Calculate normal force (simple spring model)
        penetration = plane_y - pos[1]
        spring_constant = 0.1  # N/mm
        force = [0.0, spring_constant * penetration, 0.0]
        return True, force
    return False, [0.0, 0.0, 0.0]

def main():
    # Initialize haptic device
    if haptic.init() != 0:
        print("Failed to initialize haptic device")
        return
    if haptic.start() != 0:
        print("Failed to start haptic servo loop")
        return

    # Set up 3D plot
    plt.ion()  # Interactive mode for real-time updates
    fig = plt.figure(figsize=(8, 8))
    ax = fig.add_subplot(111, projection='3d')
    ax.set_xlabel('X (mm)')
    ax.set_ylabel('Y (mm)')
    ax.set_zlabel('Z (mm)')
    ax.set_xlim(-100, 100)
    ax.set_ylim(-100, 100)
    ax.set_zlim(-100, 100)
    point, = ax.plot([0], [0], [0], 'ro', label='Haptic Position')
    plane = ax.plot_surface(
        np.array([[-100, 100], [-100, 100]]),
        np.array([[-10, -10], [-10, -10]]),
        np.array([[-100, -100], [100, 100]]),
        color='blue', alpha=0.3
    )
    ax.legend()
    ax.view_init(elev=53, azim=142,roll=-122)  # Set initial view perspective

    # Initialize sample and button state
    sample = haptic.Sample()
    last_buttons = -1
    last_sample_count = 0
    
    try:
        print("Starting haptic feedback loop. Press Ctrl+C to stop.")
        while True:
            # Get the latest sample
            haptic.get_sample(sample)
            current_sample_count = haptic.get_sample_count()

            # Process only if new sample is available
            if current_sample_count > last_sample_count:
                pos = sample.pos
                buttons = sample.buttons

                # Detect surface interaction
                on_surface, force = detect_surface(pos)
                haptic.set_force(force)

                # Update 3D plot
                point.set_data([pos[0]], [pos[1]])
                point.set_3d_properties([pos[2]])
                point.set_color('red' if not on_surface else 'green')  # Green when touching surface
                fig.canvas.draw()
                fig.canvas.flush_events()

                # Display button states
                button_states = []
                for i in range(2):
                    state = "Pressed" if buttons & (1 << i) else "Released"
                    button_states.append(f"{i+1}={state}")
                if buttons != last_buttons:
                    print(f"\n[BTN] raw=0x{buttons:X} | {', '.join(button_states)}")
                    last_buttons = buttons

                # Print position and force feedback
                print(f"[POS] x={pos[0]:.2f}, y={pos[1]:.2f}, z={pos[2]:.2f} mm | "
                      f"Force=[{force[0]:.2f}, {force[1]:.2f}, {force[2]:.2f}] N | "
                      f"Sample={current_sample_count}", end='\r')

                last_sample_count = current_sample_count

            # Brief pause to prevent excessive CPU usage
            time.sleep(0.01)

    except KeyboardInterrupt:
        print("\nStopping haptic device...")
    finally:
        haptic.stop()
        plt.close()

if __name__ == "__main__":
    main()