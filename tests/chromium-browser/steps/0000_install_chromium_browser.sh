#!/bin/bash
ROOT="$(git rev-parse --show-toplevel)"

source $ROOT/tests/pipeline/libraries/Logcat.sh
source $ROOT/tests/pipeline/libraries/Console.sh

GCHROME="https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb"

function install_google_chrome() {
	if ! exec_on_test_machine_without_output "wget $GCHROME"; then
		error "can't download $GCHROME"
	elif ! exec_on_test_machine_without_output "sudo dpkg -i ./$(basename $GCHROME)"; then
		error "can't install google-chrome"
	fi
}

function install_chromium() {
	if ! exec_on_test_machine "sudo apt install -y chromium-browser"; then
		if ! install_google_chrome; then
			error "can't install chromium-browser"
		fi
	fi
}

install_chromium
