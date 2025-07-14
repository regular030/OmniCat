| Title            | Author      | Description                        | Total Time Spent |
|------------------|-------------|------------------------------------|------------------|
| Keyboard Saturn  | Kunshpreet  | Simple Keyboard with OLED screen   | 50h              |

January: Research
Started looking at how to make my keyboard (looking at YouTube videos, etc)
Created a plan
Make the keyboard a 100% layout
Add an OLED screen to display extra info
Started thinking about what components to use (what type of switches,e.g., red, brown, blue, etc.)
Started looking at the pinout of the Pi Pico (my microcontroller)
Total time spent: 5h

May 17-20: PCB making
started creating the PCB
Found out that the PI Pico doesn't have enough pins for my matrix for the keys
Thought about using Japanese matrices, but instead I opted to use the MCP23017_SO since I want to learn how to solder SMD components
After creating the schematic, I posted it on Slack to get some feedback image
Then I started creating the PCB itself
Routing was hard, so for the first time, I used Vias image Total time spent: 15h
May 20-22: Making the chassis
started creating the chassis
Since I'm using somewhat custom spacing, it took a long time to make the top part of my keyboard image image
Once done, I started making the bottom part
After that, I started seeing how I was going to print out the chassis
JLC charges 100$ for the chassis
So instead, I made the chassis printable by splitting it into 3 parts and joining them via these standoffs that I made image Total time spent: 8h
May 22-23: Creating BOM
BOM done, please check the read me Total time spent: 4h
May 23: Updating Readme and other things
Readme updated Total time spent: 2h
May 23 - 24: Firmware
Started making firmware
took too long setting up QMK (like 5 hours in one day)
Found out the MCP23017_SO is way too slow since it's using the I2C connection protocol
Instead im using the SN74HC165N, which uses the spi protocal which is 100x faster
replaced it in the PCB
made the keyboard layout for my keyboard in QMK May 24th:
Took almost 5 hours to make the keyboard work, not even including the custom matrix
Not good with command-based apps, so I need to work on this in the future
made a custom matrix to read from the SN74HC165N
TOOK A LONG TIME JUST TO LEARN HOW TO DO IT
made the OLED work
Here are some code issues that I had today:
setPinOutput(MATRIX_COL_PINS[c]) failed because the macro received 21 arguments instead of 1, likely because MATRIX_COL_PINS[c] expands improperly.
writePinHigh(MATRIX_COL_PINS[c]) and writePinLow(MATRIX_COL_PINS[col]) also failed due to the same macro argument issue.
Macros like gpio_set_pin_output_push_pull, gpio_write_pin_high, and gpio_write_pin_low were not recognized as functions; they're macros and not meant to handle complex input.
matrix_scan_change is used without declaration, leading to an implicit function declaration error.
All warnings were treated as errors (-Werror flag), causing the build to fail on these issues. Total time spent: 15h
May 28: Created Palm Rest:
Created a palm rest on the keyboard
Added the 3mf file for the print
image Total time spent: 2h

May xx - xx: Firmware Pt2:
TODO: Create the UI for the OLED