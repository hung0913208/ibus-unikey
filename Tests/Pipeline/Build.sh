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

if [[ $# -gt 2 ]]; then
	NODE=$3
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

	if [[ $MODE -eq 1 ]] && [[ ${#NODE} -gt 0 ]]; then
		# @NOTE: fetch the latest release of supported distros so we can
       		# use them to verify our ibus-unikey black build before we deliver
		# this to the marketplace.

		for DISTRO in $(ls -1c $ROOT/Tests/Pipeline/Environments); do
			if $ROOT/Tools/Utilities/generate-customized-livecd-image.sh 	\
					--input-type iso 				\
					--output-type nfs				\
					--env $ROOT/Tests/Pipeline/Environments/$DISTRO; then
				info "successful generate $DISTRO's ISO image to start a new simulator"
			else
				error "can't build $DISTRO's ISO image"
			fi
		done

		# @NOTE: it seems the developer would like to test with a 
		# virtual machine, so we should generate file .environment
		# here to contain approviated variables to control steps to
		# build and test Unikey with our LiveCD collection

		cat > $ROOT/.environment << EOF
function start_dhcpd() {
	echo ""
}

function stop_dhcpd() {
	echo ""
}
EOF

		# @NOTE: well now we define how to test with our virtual
		# machines. According the instruction, we should define script
		# Test.sh to control how to test automatically

		cat > $ROOT/Tests/Pipeline/Test.sh << EOF
#!/bin/bash
EOF
		chmod +x $ROOT/Tests/Pipeline/Test.sh

		info "going to test with virtual machine"
	fi
else
	error "Please install git first"
fi

info "Congratulation, you have passed ${SCRIPT}"
