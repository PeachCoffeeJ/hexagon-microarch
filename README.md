# Microarchitectural Attacks on the Hexagon DSP
This project research if and how the Flush+Reload cache attack, Spectre-PHT and the Spectre-RSB transient execution attack work on the Hexagon DSP.

## Content
Microarch folder contains the code to generate the modules running on the Hexagon DSP for testing the microarchitectural attacks. The modules are developed and built by the Hexagon SDK on the Windows environment.

## Usage
Connect the rooted mobile device, set up the Hexagon SDK environment and run the following scripts:
```
# Build and push the DSP modules into the mobile device.
build&push.cmd

# Capture the DSP log messages and save to the log.txt file.
save_dsp_log.cmd

# Use Flush+Reload method to measure the access time of the DSP cache.
flush_reload.cmd

# Draw the cache access time distribution figure.
fig_plot.cmd

# Capture the DSP log messages in the terminal.
dsp_log.cmd

# Test the wrong execution path issue in the DSP.
wrong_exe_path.cmd
```

## Environment
We use Xiaomi 12 featuring Qualcomm Snapdragon 8 Gen 1 to test the microarchitectural attacks on the Hexagon DSP. We set up our Hexagon SDK environment on Windows 11.

## Status
This project is the part of my master thesis and currently it is under active research and development.
