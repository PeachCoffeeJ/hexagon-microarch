make hexagon BUILD=Debug DSP_ARCH=v68
adb push hexagon_Debug_toolv87_v68/ship/libmicroarch_skel.so /vendor/lib/rfsa/dsp/sdk/
adb shell "export LD_LIBRARY_PATH=/vendor/lib64/ DSP_LIBRARY_PATH=/vendor//lib/rfsa/dsp/sdk; /vendor/bin//microarch -r 0 -d 3 -n 1000 -U 1"