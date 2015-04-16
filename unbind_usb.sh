#!/bin/bash
# This script searches for devices being held by usbhid that match a given
# USB VID/PID, and frees them.
#
# Script assumes your user has access to the 'sudo' command, and has permission
# to execute the required unbind request through it.

# usb device VID/PID, without leading zeros.
# Same format as used by /sys/bus/usb/drivers/usbhid/[device-id]/uevent
usb_id="a81/701"

if [ $1 ]; then
    usb_id=$1
fi

hid_devices=/sys/bus/usb/drivers/usbhid/[0-9]*
for d in $hid_devices; do
    if grep -q $usb_id $d/uevent; then
        dev_id=`basename $d`
        sudo sh -c "echo $dev_id > /sys/bus/usb/drivers/usbhid/unbind"
    fi
done
