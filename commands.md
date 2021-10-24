# Common commands 
## Get idf.py
. $HOME/esp/esp-idf/export.sh

## Setting for new project
idf.py set-target esp32

## Building menu config
idf.py menuconfig
idf.py menuconfig --help

## Building the project
idf.py build

## Flash onto the device /dev/tty
idf.py -p PORT [-b BAUD] flash

## Monitor the device /dev/tty
idf.py -p /dev/ttyUSB0 monitor