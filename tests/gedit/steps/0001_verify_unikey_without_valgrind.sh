#!/bin/bash
ROOT="$(git rev-parse --show-toplevel)"
BUILDER="/home/$(username)/ibus-unikey/Base/Tools/Builder/build"
CURRENT=$(pwd)

source $ROOT/tests/pipeline/libraries/Logcat.sh
source $ROOT/tests/pipeline/libraries/Package.sh
source $ROOT/tests/pipeline/libraries/Console.sh

SPECs=("Debug" "Sanitize" "Coverage")

if ! rsync_to_test_machine $ROOT --exclude={'pxeboot','vms'}; then
	error "can't rsync $ROOT to /home/$(username)/ibus-unikey"
elif ! exec_on_test_machine "mkdir -p /home/$(username)/ibus-unikey/build"; then
	error "can't mkdir /home/$(username)/ibus-unikey/build"
elif ! exec_on_test_machine "cd ~/ibus-unikey && BUILD='-DCMAKE_INSTALL_PREFIX=/usr' $BUILDER --root /home/$(username)/ibus-unikey --rebuild 0 --mode 0"; then
	error "can't build ibus-unikey with machine $MACHINE"
fi

for SPEC in ${SPECs[@]}; do
	info "start testing with build mode $SPEC"

	if ! exec_on_test_machine_without_output "cd ~/ibus-unikey/build/$SPEC && sudo make install"; then
		error "can't install to /home/$(username)/$SPEC"
	elif ! exec_on_test_machine --add-arg '-Y' "DISPLAY=:0 ibus engine Unikey"; then
		error "can't config ibus to use with Unikey"
	fi

	for CASE in $(ls -1c $SUITE); do
		EXT="${CASE##*.}"
	
		if [ $EXT = "py" ]; then
			if ! copy_to_test_machine $SUITE/$CASE; then
				error "can't copy $SUITE/$CASE to $MACHINE" 
			fi
	
			warning "run test case $CASE"
	
			if ! exec_on_test_machine --timeout 60 "DISPLAY=:0 python ~/$CASE"; then
				error "fail test case $CASE"
			fi
		fi
	done
done
