#!/bin/bash

BIN_DIR=$1
OUT_DIR=$2
ARGS=${@:3}

test_file() {
    file=$1
    echo "$file >>> " >> $OUT_DIR/report_tpcas.txt
    ./${BIN_DIR}/tpcc $ARGS < $file >> $OUT_DIR/report_tpcas.txt 2>> $OUT_DIR/report_tpcas.txt
    echo -e "exit code: $?\n" >> $OUT_DIR/report_tpcas.txt
}

for dir_file in test/* ; do
    if [[ -d $dir_file ]]
    then
        for file in $dir_file/* ; do
            test_file $file
        done
    else
        test_file $dir_file
    fi
done