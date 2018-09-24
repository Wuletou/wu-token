#!/usr/bin/env bash

CONFIG_FILENAME="config.h"
BUILD_DIR=build
CPP_FILENAME=wu-token.cpp
ABI_FILENAME=wu-token.abi
WAST_FILENAME=wu-token.wast

ARGUMENT_LIST=(
	"EXCHANGE"
)

opts=$(getopt \
	--longoptions "$(printf "%s:," "${ARGUMENT_LIST[@]}")" \
	--name "$(basename "$0")" \
	--options "" \
	-- "$@"
)

function usage() {
	echo "Usage: ./compile.sh [ARGS]"
	echo "--EXCHANGE - account of Exchange contract"
	echo "Example:"
	echo "./compile.sh --EXCHANGE wuletexchacc"
}

function check_input() {
	for field in ${ARGUMENT_LIST[@]}; do
		if [[ -z "${!field}" ]]; then
			>&2 echo "missing --$field"
			err=1
		fi
	done
	if [[ -n "$err" ]]; then
		usage
		exit 0
	fi
}

function out() {
	echo "#define EXCHANGE $EXCHANGE" > ${CONFIG_FILENAME}
}

function compile() {
	mv ${BUILD_DIR}/${ABI_FILENAME} .
	rm -rf ${BUILD_DIR}
	mkdir ${BUILD_DIR}
	mv ${ABI_FILENAME} ${BUILD_DIR}
	eosiocpp -o ${BUILD_DIR}/${WAST_FILENAME} ${CPP_FILENAME}
}

eval set --$opts
while [[ $# -gt 0 ]]; do
	case "$1" in
		--EXCHANGE)
			EXCHANGE=$2
			shift 2
			;;
		*)
			break
			;;
	esac
done

check_input
out
compile
