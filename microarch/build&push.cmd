adb root
make android BUILD=Debug
make hexagon BUILD=Debug DSP_ARCH=v68
adb shell mkdir -p /vendor/bin/
adb push android_Debug_aarch64/ship/microarch /vendor/bin/
adb shell chmod 777 /vendor/bin/microarch
adb push android_Debug_aarch64/ship/libmicroarch.so /vendor/lib64/
adb shell mkdir -p /vendor/lib/rfsa/dsp/sdk
adb push hexagon_Debug_toolv87_v68/ship/libmicroarch_skel.so /vendor/lib/rfsa/dsp/sdk/