import matplotlib.pyplot as plt
import numpy as np

# Replace 'your_file.txt' with the path to your file containing the audio samples
file_path = '/private/tmp/Square_Pulse_2.log'

# Read the file and convert each line to an integer
with open(file_path, 'r') as file:
    samples = [int(line.strip()) if line.strip() != "-" else np.nan for line in file]

# Convert the list of samples to a numpy array for efficient handling
samples_array = np.array(samples)

# Plotting
plt.figure(figsize=(10, 4))  # Set figure size
plt.plot(samples_array, label='Audio Sample')
plt.title('Audio Samples Plot')
plt.xlabel('Sample Index')
plt.ylabel('Sample Value')
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()
