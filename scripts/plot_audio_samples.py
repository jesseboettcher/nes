import matplotlib.pyplot as plt
import numpy as np

square_pulse_1_path = '/private/tmp/Square_Pulse_1.log'
square_pulse_2_path = '/private/tmp/Square_Pulse_2.log'
triangle_path = '/private/tmp/Triangle.log'

# Read the file and convert each line to an integer
with open(square_pulse_1_path, 'r') as file:
    square_pulse_1_samples = [int(line.strip()) if line.strip() != "-" else np.nan for line in file]

with open(square_pulse_2_path, 'r') as file:
    square_pulse_2_samples = [int(line.strip()) if line.strip() != "-" else np.nan for line in file]

with open(triangle_path, 'r') as file:
    triangle_samples = [int(line.strip()) if line.strip() != "-" else np.nan for line in file]

# Plotting
plt.figure(figsize=(10, 4))  # Set figure size
plt.plot(np.array(square_pulse_1_samples), label='Square_Pulse_1')
plt.plot(np.array(square_pulse_2_samples), label='Square_Pulse_2')
plt.plot(np.array(triangle_samples), label='Triangle')
plt.title('Audio Samples Plot')
plt.xlabel('Sample Index')
plt.ylabel('Sample Value')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
