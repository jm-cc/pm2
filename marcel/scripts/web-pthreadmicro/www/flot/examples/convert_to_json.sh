#!/bin/sh

# $1: runtime system
# $2: OpenMP construct
# $3: MonthYear

JSON_FILE_NAME="$1-$2-$3.json"
JSON_LABEL="'""$1""'"

echo "{" > $JSON_FILE_NAME
echo "  label: $JSON_LABEL," >> $JSON_FILE_NAME

DATA=$(cat $3".perfs")
echo $DATA

JSON_DATA="  data: ["
num_data=0
for perf in $DATA
do
    num_data=$(expr $num_data + 1)
    JSON_DATA=$JSON_DATA"[$num_data, $perf], "
done
JSON_DATA=$JSON_DATA"]"

echo $JSON_DATA
echo $JSON_DATA >> $JSON_FILE_NAME
echo "}" >> $JSON_FILE_NAME

