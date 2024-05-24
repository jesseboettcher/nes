import struct
import os
import socket
import time

# Command for compilation of .proto file
# protoc --python_out=. agent_interface.proto

import agent_interface_pb2

button_presses = agent_interface_pb2.ButtonPress()

button_presses.timestamp = int(time.time() * 1000)
button_presses.sequence_number = 4
button_presses.pressed_buttons.extend([True, False, False, False, False, False, False, True])

serialized_data = button_presses.SerializeToString()

sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

sock.connect("/tmp/nes_screenshot_server.sock")

print(len(serialized_data))
header = struct.pack('@I', len(serialized_data))

sock.send(header)
sock.send(serialized_data)


img_size = 0

while img_size == 0:

	data = sock.recv(4)
	img_size = int.from_bytes(data, byteorder='little')

	if img_size == 0:
		continue

	print(f'img size {img_size}')
	img_data = b''

	while len(img_data) < img_size:
		packet = sock.recv(4046)

		if not packet:
			break

		img_data += packet

	screenshot = agent_interface_pb2.Screenshot()
	screenshot.ParseFromString(img_data)

	with open('/Users/jesse/Desktop/received_image.png', 'wb') as f:
		for d in screenshot.png_data:
			f.write(d)

sock.close()