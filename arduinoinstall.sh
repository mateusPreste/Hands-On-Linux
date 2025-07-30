#!/bin/bash
#wget --continue https://downloads.arduino.cc/arduino-1.8.19-linux64.tar.xz
#tar -xf arduino-1.8.19-linux64.tar.xz
#mv arduino-1.8.19 ~/Downloads/
touch ~/.local/share/applications/arduino.desktop
cd ~/.local/share/applications/
cat <<EOF >  arduino.desktop
[Desktop Entry]
Name=Arduino
Comment=Arduino IDE
Exec=~/Downloads/arduino-1.8.19/arduino
Icon=arduino-arduinoide
Terminal=false
Type=Application
Categories=Development;
EOF
