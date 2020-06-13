#!/bin/bash
ROOT="$(git rev-parse --show-toplevel)"
BUILDER="/home/$(username)/ibus-unikey/Base/Tools/Builder/build"
CURRENT=$(pwd)

source $ROOT/tests/pipeline/libraries/Logcat.sh
source $ROOT/tests/pipeline/libraries/Package.sh
source $ROOT/tests/pipeline/libraries/Console.sh

if ! exec_on_test_machine "sudo chmod -x /usr/bin/gnome-keyring-daemon"; then
	warning "can't disable gnome-keyring-daemon"
elif ! exec_on_test_machine "sudo killall gnome-keyring-daemon"; then
	warning "can't kill all process \`gnome-keyring-daemon\`"
fi

