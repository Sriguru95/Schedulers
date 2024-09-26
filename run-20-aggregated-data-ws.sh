#!/bin/bash

readonly DEFAULT=10485760 # 10*1024*1024
readonly MAX=1073741823

gcc -Wall -o quicksort quicksort.c sched_ws.c

run_twenty_times() {
    local n="$1"
    local global_output_file="output_wsl_ws.txt" #rename conditionally

    echo "===================START========================" >> "$global_output_file"
    echo "------------------------------------------------" >> "$global_output_file"
    echo "------------------------------------------------" >> "$global_output_file"
    echo "executing 20 iterations for array of $n elements" >> "$global_output_file"
    for t in {1..8}; do
        local output_file="wsl_ws_arr_size_${n}_with_${t}_threads"
        echo "execution for $t threads for array of $n elements" >> "$output_file"
        for i in {1..20}; do
            result=$(./quicksort -t "$t" -n "$n")
            echo $result >> "$output_file"
            echo $result >> "$global_output_file"
        done
    done
    echo "------------------------------------------------" >> "$global_output_file"
    echo "------------------------------------------------" >> "$global_output_file"
    echo "====================END=========================" >> "$global_output_file"
}

run_twenty_times 10000
run_twenty_times "$DEFAULT"
run_twenty_times "$MAX"
