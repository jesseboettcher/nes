# PC Context Viewer
#
# Script to display the assembly surrounding the current instruction
# while the NES emulator is running. Emulator and script do not do the
# disassembly of the rom, that must be done as a separate step.
#
# Tested with disassembly from nesgodisasm targeting the ca65 format
# in the ca65 format. https://github.com/retroenv/nesgodisasm

import argparse
import time
import os
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

def read_current_pc(file_path):
    """Read the current PC value from the file."""
    with open(file_path, 'r') as file:
        return file.read().strip()

def find_pc_in_lines(pc, lines):
    """Find the PC in the loaded lines and return the context lines."""

    # Adjust PC format to match the assembly file format
    pc_formatted = f"${pc.upper()}" # Assuming PC is in this format "$XXXX"

    # Find the line with the current PC
    pc_line_index = None
    for i, line in enumerate(lines):
        if pc_formatted in line:
            pc_line_index = i
            break

    if pc_line_index is None:
        return None

    # Extract 10 lines above and below the PC line
    start_index = max(0, pc_line_index - 10)
    end_index = min(len(lines), pc_line_index + 11)

    return lines[start_index:end_index], pc_line_index - start_index

def highlight_and_print_lines(lines, pc_line_index):
    """Print the lines with the PC line highlighted."""

    for i, line in enumerate(lines):
        if i == pc_line_index:
            # Print the PC line in light blue
            print(f"\033[33m{line}\033[0m", end='')
        else:
            print(line, end='')

class PCFileChangeHandler(FileSystemEventHandler):

    def __init__(self, assembly_lines, pc_file_path):
        self.assembly_lines = assembly_lines
        self.pc_file_path = pc_file_path

    def on_modified(self, event):
        if event.src_path == os.path.realpath(self.pc_file_path):
            self.display_context()

    def display_context(self):
        current_pc = read_current_pc(self.pc_file_path)
        result = find_pc_in_lines(current_pc, self.assembly_lines)

        if result:
            context_lines, pc_line_index = result
            os.system('clear')  # Use 'cls' on Windows
            highlight_and_print_lines(context_lines, pc_line_index)
        else:
            os.system('clear')  # Use 'cls' on Windows
            print(f"PC {current_pc} not found in the assembly file.")

def main(assembly_file_path, pc_file_path):
    # Read the assembly file only once

    with open(assembly_file_path, 'r') as file:
        assembly_lines = file.readlines()

    event_handler = PCFileChangeHandler(assembly_lines, pc_file_path)
    observer = Observer()
    observer.schedule(event_handler, path=os.path.dirname(pc_file_path), recursive=False)
    observer.start()

    event_handler.display_context() # show initially, updates will come as PC i schanged

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        observer.stop()
    observer.join()

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Display assembly context around the PC register.")
    parser.add_argument("-a", "--assembly", required=True, help="Path to the assembly file.")
    parser.add_argument("-r", "--register", required=True, help="Path to the file containing the PC register value.")
    
    args = parser.parse_args()

    main(args.assembly, args.register)
