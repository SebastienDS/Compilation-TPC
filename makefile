.PHONY: all run test clean clear_utils compile_asm run_asm

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj
OUT_DIR = out

ARGS = 

EXE = ${BIN_DIR}/tpcc

CFLAGS = -W -Wall -I ${SRC_DIR} -g -Wno-unused-parameter -Wno-unused-variable

SRC = $(wildcard ${SRC_DIR}/*.c) ${OBJ_DIR}/tpcas.tab.c ${OBJ_DIR}/lex.yy.c
OBJ_ = $(SRC:${SRC_DIR}/%.c=${OBJ_DIR}/%.o)
OBJ = $(OBJ_:${OBJ_DIR}/%.c=${OBJ_DIR}/%.o)


ASM_FILENAME = bin/_anonymous


all: ${BIN_DIR} ${OBJ_DIR} ${OUT_DIR} clear_utils ${EXE}

run: all
	./${EXE} ${ARGS}

${EXE}: ${OBJ}
	gcc $^ -o $@ ${CFLAGS}

${OBJ_DIR}/lex.yy.c: ${SRC_DIR}/tpcas.lex
	flex -o $@ $<

${OBJ_DIR}/tpcas.tab.c: ${SRC_DIR}/tpcas.y
	bison -o $@ -d $< --report=all

${OBJ_DIR}/%.o: ${SRC_DIR}/%.c
	gcc -c $< -o $@ ${CFLAGS}

${OBJ_DIR}/%.o: ${OBJ_DIR}/%.c
	gcc -c $< -o $@ ${CFLAGS}

test: all
	rm -rf ${OUT_DIR}/report_tpcas.txt
	./test_tpcas.sh ${BIN_DIR} ${OUT_DIR} ${ARGS}
	cat ${OUT_DIR}/report_tpcas.txt

clean: 
	rm -rf ${BIN_DIR} ${OBJ_DIR} ${OUT_DIR}

${OBJ_DIR}: 
	mkdir -p ${OBJ_DIR}

${BIN_DIR}: 
	mkdir -p ${BIN_DIR}

${OUT_DIR}: 
	mkdir -p ${OUT_DIR}

clear_utils:
	rm -rf ${OBJ_DIR}/utils.o

compile_asm: all
	nasm -g -f elf64 -F dwarf -o $(OBJ_DIR)/utils.o $(SRC_DIR)/utils.asm
	nasm -g -f elf64 -F dwarf -o $(ASM_FILENAME).o $(ASM_FILENAME).asm

	gcc -Wall -g -fPIC -o $(ASM_FILENAME) $(OBJ_DIR)/utils.o $(ASM_FILENAME).o -nostartfiles -no-pie
	
run_asm: compile_asm
	./${ASM_FILENAME}