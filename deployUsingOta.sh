GIT_SHA=$(git rev-parse --short HEAD)
GIT_COMMIT_MESSSAGE=$(git log --format=%B -n 1 $GIT_SHA)
BIN_FILE_NAME=indoor_hydro-$GIT_SHA
DEVICE_ID=CjvJ39w8

echo $GIT_SHA 
echo $GIT_COMMIT_MESSAGE

set -x

sed 's/newVersion/'$GIT_SHA'/g' version.tmp > version.h

arduino-cli compile --export-binaries --build-property build.project_name=$BIN_FILE_NAME --fqbn esp32:esp32:esp32doit-devkit-v1

python espota.py -d  -i 192.168.1.55 -f build/esp32.esp32.esp32doit-devkit-v1/$BIN_FILE_NAME.bin



#curl --user q:q -F description=hello -F filename=$BIN_FILE_NAME.bin  -F File=@build/esp8266.esp8266.nodemcuv2/$BIN_FILE_NAME.bin https://soeren.herokuapp.com/api/devices/$DEVICE_ID/images

