NC_INST_DIR=/apps/cadence/INCISIV_15.20.006/
QUESTA_INST_DIR=/apps/mentor/10.6b-64bit/questasim/

NCSIM_LIB_PATH = $(NC_INST_DIR)/tools/lib
QUESTA_LIB_PATH = $(QUESTA_INST_DIR)/linux_x86_64

NCSIM_INCLUDES = -I ${NC_INST_DIR}/tools/include/
NCSIM_LINKS    = -L ${NCSIM_LIB_PATH} -lucis -lcdsCommon_sh -ldl

QUESTA_INCLUDES = -I ${QUESTA_INST_DIR}/include
QUESTA_LINKS    = -L $(QUESTA_LIB_PATH) -lucis -lucdb -lm -ldl 
QUESTA_STATIC   = ${QUESTA_INST_DIR}/linux_x86_64/libucis.a ${QUESTA_INST_DIR}/linux_x86_64/libucdb.a

COMMON_OBJ = ./build/common/ucis_callbacks.o ./build/common/check_file_parser.o ./build/common/parser_utils.o ./build/common/query_data.o ./build/common/excl_tree.o ./build/common/top_tree.o ./build/common/iterator.o ./build/common/arg_parser.o ./build/common/formatter.o ./build/main.o
CDNS_OBJ = ./build/cdns/vp_refine_parser.o ./build/cdns/vplan_parser.o
MTI_OBJ= ./build/mti/exclusion_parser.o

EXEC= coverage_lens

.PHONY: all dir build_common build_cdns build_mti link_cdns link_mti help run clean doc

all: help

./build/common/%.o: ./src/common/%.cpp
	${CC} -std=c++11 -c -o "$@" "$^" -I./includes -D${VENDOR}

./build/cdns/%.o: ./src/cdns/%.cpp
	${CC} -std=c++11 -c -o "$@" "$^" -I./includes -D${VENDOR}

./build/mti/%.o: ./src/mti/%.cpp
	${CC} -std=c++11 -c -o "$@" "$^" -I./includes -D${VENDOR}

./build/main.o: ./src/main.cpp
	${CC} -std=c++11 -c -o "$@" "$^" -I./includes -D${VENDOR}

dir:
	@if [ ! -d "./build" ]; then mkdir -p build; mkdir -p build/common;	mkdir -p build/cdns; mkdir -p build/mti; fi	

build_common: dir ${COMMON_OBJ}

build_cdns: build_common ${CDNS_OBJ}

build_mti: build_common ${MTI_OBJ}

link_cdns: build_cdns
	@${CC} ${NCSIM_LINKS} ${COMMON_OBJ} ${CDNS_OBJ} -o ${EXEC} ${NCSIM_INCLUDES} -D${VENDOR}

link_mti: build_mti
	@${CC} ${QUESTA_LINKS} ${COMMON_OBJ} ${MTI_OBJ} ${QUESTA_STATIC} -o ${EXEC} ${QUESTA_INCLUDES}

help:
	@cat doc/help && echo;

run:
	@export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${NCSIM_LIB_PATH}:${QUESTA_LIB_PATH}; ./$(EXEC) $(ARGS)

doc:
	@cd doc; doxygen doxy

rmdoc:
	@rm -rf ./doc/doc

clean:
	@rm -rf $(EXEC) *.log *.o ./build cl_report.html cl_report
