#!/bin/bash

ROOT="$(git rev-parse --show-toplevel)"
BUILDER="/home/$(username)/ibus-unikey/Base/Tools/Builder/build"
CURRENT=$(pwd)

source $ROOT/tests/pipeline/libraries/Logcat.sh
source $ROOT/tests/pipeline/libraries/Package.sh
source $ROOT/tests/pipeline/libraries/Console.sh

# @NOTE: restore gnome-keyring-daemon
if ! exec_on_test_machine "sudo chmod +x /usr/bin/gnome-keyring-daemon"; then
	warning "can't enable gnome-keyring-daemon"
fi

# @NOTE: clear everything after testing
exec_on_test_machine 'rm -fr ~/\*.py'
exec_on_test_machine_without_output "sudo apt remove -y google-chrome"
exec_on_test_machine_without_output "sudo apt autoremove"
