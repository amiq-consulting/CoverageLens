#!/bin/bash
OLD_RUN=cdns

COMP=g++

if [ $# -lt 1 ]; then
	echo "Usage: ./compile.sh {mentor|cadence} [path_to_vendor_install]";
	exit;
fi

if [ $# -gt 2 ]; then
	echo "Usage: ./compile.sh {mentor|cadence} [path_to_vendor_install]";
	exit;
fi

if [ "$1" == "cadence" ]; then

	if [ ! "$OLD_RUN" = "cdns" ]; then
		make clean;
	fi

	if [ $# -eq 2 ]; then
		sed -i "1s|.*|"NC_INST_DIR=$2"|" Makefile;
	fi
	
	make -f Makefile link_cdns VENDOR=NCSIM CC="$COMP \${CDNS_OPT}"
	
	sed -i "2s/.*/"OLD_RUN=cdns"/" $0;
	
	
	
elif [ "$1" == "mentor" ]; then

	if [ ! "$OLD_RUN" = "mti" ]; then
		make clean;
	fi
	
	if [ $# -eq 2 ]; then
		sed -i "2s|.*|"QUESTA_INST_DIR=$2"|" Makefile;
	fi
	
	make link_mti VENDOR=QUESTA CC=$COMP;
	
	sed -i "2s/.*/"OLD_RUN=mti"/" $0;

else
	echo "Usage: ./compile.sh [mentor|cadence]";
fi
