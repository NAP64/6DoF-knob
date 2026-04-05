# 6DoF-knob

A tiny 6DoF knob instance with 3 joysticks.

The goal was to make a keyboard key-sized 6DoF knob, as a part of some keyboard project.

The current status of this 3-joystick approach is abandonded.

## SDK Setup and compile
I'm using a ESP32-S3-DevKitC.

However, this is simple enough you can port to any MC with ADC and USB

To port to a different ESP32 board, find the ADC channel and GPIO pins, and modify the source code.

1. Download and setup ESP-idf:

        git clone https://github.com/espressif/esp-idf
        cd esp-idf
        git checkout v6.0
        git submodule update --init --recursive
        install.sh
        source export.sh

2. Navigate to the 6DoF-knob directory under this repo, connect the device, and run

        idf.py set-target esp32s3
        idf.py build
        idf.py flash

3. You can further fine-tune the code, there's a few deadzone constants and such.
Just build and flush again.

## Assembly
I'm using FJ06K-N joysticks. These are the tinyest joysticks I could find.

I'm also using 2 M3 screws to attach the knob parts and to mount the joystick housing,
as well aa a clamp to fix it to the table.


