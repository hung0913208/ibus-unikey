#!/bin/bash
# - File: build.sh
# - Description: This bash script will be run right after prepare.sh and it will
# be used to build based on current branch you want to Tests

PIPELINE="$(dirname "$0" )"
source $PIPELINE/Libraries/Logcat.sh
source $PIPELINE/Libraries/Package.sh

SCRIPT="$(basename "$0")"

if [[ $# -gt 0 ]]; then
	MODE=$2
else
	MODE=0
fi

if [[ $# -gt 0 ]]; then
	TYPE=$3
fi

info "You have run on machine ${machine} script ${SCRIPT}"
info "Your current dir now is $(pwd)"

if [ $(which git) ]; then
	# @NOTE: jump to branch's test suite and perform build
	ROOT="$(git rev-parse --show-toplevel)"
	BUILDER=$ROOT/Base/Tools/Builder/build

	if ! $BUILDER --root $ROOT --debug 1 --rebuild 0 --mode $MODE; then
		exit -1
	fi
else
	error "Please install git first"
fi

info "Congratulation, you have passed ${SCRIPT}"
