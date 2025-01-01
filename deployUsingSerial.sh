GIT_SHA=$(git rev-parse --short HEAD)
GIT_COMMIT_MESSSAGE=$(git log --format=%B -n 1 $GIT_SHA)
BIN_FILE_NAME=fitbit-display-$GIT_SHA
DEVICE_ID=CjvJ39w8

echo $GIT_SHA 
echo $GIT_COMMIT_MESSAGE

set -x

sed 's/newVersion/'$GIT_SHA'/g' version.tmp > version.h

arduino-cli compile --export-binaries --build-property build.project_name=$BIN_FILE_NAME --fqbn esp32:esp32:esp32doit-devkit-v1

SERIAL_PORT="COM5"
python -I "C:\Users\soere\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\3.1.2/tools/upload.py" --chip esp32 --port $SERIAL_PORT --baud "115200" ""  --before default_reset --after hard_reset write_flash 0x0 build/esp32.esp32.esp32doit-devkit-v1/$BIN_FILE_NAME.bin
