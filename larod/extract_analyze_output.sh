#!/bin/bash -e

# This is a helper script for analyzing and visualizing the
# output from running this example on an Axis device.

# Usage:
# $1   -----   <IP-ADRESS of device>
# $2   -----   <path to matching labelsfile for classification>
#
#
# Default user:         root
# Default password :    pass

[ $# -lt 2 ] && {
    echo "Usage: <IP-ADRESS of device> <path to matching labelsfile for classification>"
    exit 1
}
# These are the default values for the device under test, change these if needed
USER=root
PASSWORD=pass

#Enable SSH on the camera
result=$(curl --noproxy "$1" -s -u "$USER:$PASSWORD" --anyauth "http://$1/axis-cgi/admin/param.cgi?action=update&Network.SSH.Enabled=yes")
echo "Enabling SSH: $result"

# Extract the output file to your current directory on host
# will prompt for user password
scp $USER@"$1":/usr/local/packages/larod_simple_app/input/veiltail-11457_640_RGB_224x224.bin.out .

# Run the analysis on the output using this command:
# od needs to be installed on host system
# od -A d -t u1 -v -w1 <name of output file> | sort -n -k 2

od -A d -t u1 -v -w1 "veiltail-11457_640_RGB_224x224.bin.out" | sort -n -k 2 > result.txt

while [ -z "$eof" ]; do
    read -r LINE || eof=true   ## detect eof, but have a last round
    if [ "$LINE" ]; then
        #echo "read " $LINE
        index=${LINE%%[[:space:]]*}
        value=${LINE##*[[:space:]]}
    if [ "$value" -gt 0 ] && [ "$value" -lt 256 ]; then
        # convert the line index
        row=$((10#${index}))
        # add one since the index starts from 0
        line=$((row + 1))
        # Get the class from the labels file
        class=$(awk 'NR=='$line'' "$2")
        # Recalculate as a probability percentage
        prob=$(((value / 255) * 100))
        # Print the result
        echo "The model has found that with a probability of $prob % the picture represents a $class"
    fi
  fi
done < result.txt
