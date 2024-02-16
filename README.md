# NES Emulator

A hobby project to build an 8-bit Nintendo emulator in C++. It's not intended to be different or
better than any of the other emulators out there. My goal is to get it functional enough to run
The Legend of Zelda with a USB controller.

# Getting Started

You'll need an installation of QT6 and then update the below instructions with your QT installation path.

* `mkdir build`
* `cd build`
* `cmake ../platform -DQt6_DIR=~/Qt/6.6.1/macos/lib/cmake/Qt6`
* `make`

# Milestones

- [x] Pass processor validation tests
- [x] Boot Ice Hockey and Donkey Kong to main menu (ppu background tiles)
- [x] Donkey Kong playable (ppu sprites)
- [ ] Ice Hockey Playable (ppu scrolling)
- [ ] Sound (partially enabled now)
- [ ] Load Zelda (cartridge mapper 1)
- [x] USB controllers (one player)

# Resources

### References
- NES
  - [Nes Dev Wiki](http://nesdev.org/wiki/NES_reference_guide)
- 6502
  - [Programming the 65816 Including the 6502...](https://archive.org/details/0893037893ProgrammingThe65816) - Great book that clearly explains all the addressing modes, operations, instructions, etc…
  - [mass:werk 6502 instruction set reference](https://www.masswerk.at/6502/6502_instruction_set.html) - awesome

### Tests
Very small differences in behavior can be the difference between a game running vs totally borked. My 6502 processor validation was done primarily through two tests suites:
* Tom Harte's [Processor Tests](https://github.com/TomHarte/ProcessorTests/tree/main/6502)
> These are great because they are provided as JSON files where each test case specifies the register and memory state before and after the instruction is executed. That makes it far easier to pinpoint the failure than running a test rom that's been through 60k cycles before indicating a failure. There are ~10k tests for each instruction. Sweet.
* blargg's [instr_test-v5](https://github.com/christopherpow/nes-test-roms/tree/master/instr_test-v5)
> These roms helped me track down a handful of issues that the json Processor Tests did not catch. For failing roms I generated logs at each instruction and compared them to logs from the rom passing in [Nintaco](https://nintaco.com)
