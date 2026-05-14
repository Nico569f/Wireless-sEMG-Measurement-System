# DataPlotter.py - Animated visualisation of EMG, accelerometer, and gyroscope data
# This script loads a CSV file produced by serial_logger.py and plays back the sensor data
# as a real-time style animation with a sliding time window.
#
# Three stacked subplots display EMG (3 channels, in Volts), acceleration (3 axes, in m/s²),
# and angular rate (3 axes, in deg/s) on a shared time axis. Raw ADC and IMU counts are
# converted to physical units on load. Playback speed and frame step are configurable,
# and a CSV path can be passed as a command-line argument or defaults to TørElektrode.csv.

import sys
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation


class CombinedDataPlotter:
    """Animate EMG (3 ch), Accelerometer (3 ch) and Gyroscope (3 ch)
    in a single figure with shared time axis and sliding time window.
    """

    def __init__(self, x_data, emg_data, accel_data, gyro_data, time_window=2000):
        self.raw_x = x_data
        self.x = x_data - x_data[0]  # Normalise time so it starts at 0 ms
        self.emg = emg_data
        self.accel = accel_data
        self.gyro = gyro_data
        self.time_window = time_window  # Width of the sliding window in ms

        # Estimate ms per sample from the gaps between timestamps;
        # falls back to 1.0 if there's only one data point
        self.sampling_interval = float(np.mean(np.diff(self.x))) if len(self.x) > 1 else 1.0

        # Create figure with 3 stacked subplots (EMG, Accel, Gyro).
        # sharex=True keeps the time axis in sync when the window scrolls.
        self.fig, (self.ax_emg, self.ax_accel, self.ax_gyro) = plt.subplots(3, 1, figsize=(12, 10), sharex=True)

        # --- EMG subplot ---
        emg_labels = ['EMG1', 'EMG2', 'EMG3']
        self.emg_lines = []
        for i in range(self.emg.shape[1]):
            # Create an empty line for each channel; data is filled in per frame
            ln, = self.ax_emg.plot([], [], label=emg_labels[i] if i < len(emg_labels) else f'EMG{i+1}')
            self.emg_lines.append(ln)
        self.ax_emg.set_ylabel('EMG (V)')
        self.ax_emg.legend(loc='upper right')
        self.ax_emg.grid(True, alpha=0.3)

        # --- Accelerometer subplot ---
        accel_labels = ['Accel X', 'Accel Y', 'Accel Z']
        self.accel_lines = []
        for i in range(self.accel.shape[1]):
            ln, = self.ax_accel.plot([], [], label=accel_labels[i])
            self.accel_lines.append(ln)
        self.ax_accel.set_ylabel('Acceleration (m/s²)')
        self.ax_accel.legend(loc='upper right')
        self.ax_accel.grid(True, alpha=0.3)

        # --- Gyroscope subplot ---
        gyro_labels = ['Gyro X', 'Gyro Y', 'Gyro Z']
        self.gyro_lines = []
        for i in range(self.gyro.shape[1]):
            ln, = self.ax_gyro.plot([], [], label=gyro_labels[i])
            self.gyro_lines.append(ln)
        self.ax_gyro.set_ylabel('Gyro (deg/s)')
        self.ax_gyro.set_xlabel('Time (ms)')
        self.ax_gyro.legend(loc='upper right')
        self.ax_gyro.grid(True, alpha=0.3)

        # Fix y-limits up front using the full dataset so the axes don't
        # rescale during playback, which would break blit rendering
        self._set_limits(self.ax_emg, self.emg)
        self._set_limits(self.ax_accel, self.accel)
        self._set_limits(self.ax_gyro, self.gyro)

        self.fig.tight_layout()

    @staticmethod
    def _set_limits(ax, data):
        """Set y-axis limits with 10% padding above and below the data range."""
        if data.size == 0:
            return
        y_min = np.min(data)
        y_max = np.max(data)
        # Use a flat ±1 pad if the signal is constant (avoids zero-height axis)
        pad = (y_max - y_min) * 0.1 if y_max > y_min else 1.0
        ax.set_ylim(y_min - pad, y_max + pad)

    def _update_axis_lines(self, axis, lines, data, start_idx, end_idx):
        """Push the current window slice into each line on a subplot,
        then update the x-axis limits to match the visible time range."""
        for i, ln in enumerate(lines):
            ln.set_data(self.x[start_idx:end_idx], data[start_idx:end_idx, i])
        axis.set_xlim(self.x[start_idx], self.x[end_idx - 1])

    def _window_indices(self, current_time):
        """Return the array indices that bracket the sliding time window
        ending at current_time and starting time_window ms earlier."""
        start_idx = np.searchsorted(self.x, current_time - self.time_window, side='left')
        end_idx   = np.searchsorted(self.x, current_time, side='right')
        return start_idx, end_idx

    def update(self, frame):
        """FuncAnimation callback: called once per frame with the frame index.
        Computes the visible window and refreshes all subplot lines."""
        # Clamp frame to the last valid index in case frame_step overshoots
        current_time = self.x[min(frame, len(self.x) - 1)]
        s, e = self._window_indices(current_time)

        # Only draw if the window contains at least one sample
        if e > s:
            self._update_axis_lines(self.ax_emg,   self.emg_lines,   self.emg,   s, e)
            self._update_axis_lines(self.ax_accel, self.accel_lines, self.accel, s, e)
            self._update_axis_lines(self.ax_gyro,  self.gyro_lines,  self.gyro,  s, e)

        # Return all artist objects so blit=True only redraws what changed
        return self.emg_lines + self.accel_lines + self.gyro_lines

    def animate(self, speed_multiplier=1.0, frame_step=1):
        """Start the animation loop.

        speed_multiplier: how many times faster than real-time to play back.
        frame_step:       number of data samples to advance per animation frame
                          (higher = faster but choppier playback).
        """
        # Derive the timer interval (ms) from the sampling rate and speed factor
        interval = max(1, int(round(self.sampling_interval / max(speed_multiplier, 1.0))))
        self.ani = animation.FuncAnimation(
            self.fig,
            self.update,
            frames=range(0, len(self.x), max(1, int(frame_step))),
            blit=True,            # Only redraw changed artists for performance
            interval=interval,    # Delay between frames in ms
            cache_frame_data=False,  # Don't store every frame; data is computed on the fly
            repeat=False,         # Stop at the end of the recording
        )
        plt.show()


def load_data_from_csv(path):
    """Read a CSV file produced by the logger and convert raw sensor counts
    to physical units using the sensor's full-scale range settings."""
    data = np.loadtxt(path, delimiter=',', skiprows=1)  # skip header row
    x     = data[:, 0]                                  # timestamp column (ms)
    emg   = data[:, 1:4] * (3.3 / 4095.0)              # 12-bit ADC, 3.3 V ref → Volts
    accel = data[:, 4:7] * (2.0 * 9.81 / 32768.0)      # ±2 g range → m/s²
    gyro  = data[:, 7:10] * (2000.0 / 32768.0)         # ±2000 dps range → deg/s
    return x, emg, accel, gyro


def main(csv_path=None):
    # Fall back to the default test file if no path is given on the command line
    if csv_path is None:
        csv_path = 'TørElektrode.csv'
    x, emg, accel, gyro = load_data_from_csv(csv_path)
    # 1000 ms sliding window; play at 10× real-time, advancing 2 samples per frame
    plotter = CombinedDataPlotter(x, emg, accel, gyro, time_window=1000)
    plotter.animate(speed_multiplier=10.0, frame_step=2)


if __name__ == '__main__':
    # Accept an optional CSV path as the first command-line argument
    main(sys.argv[1] if len(sys.argv) > 1 else None)