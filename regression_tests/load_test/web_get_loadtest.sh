#!/bin/bash


host=109.75.33.37
metax_port=7081

result_dir=results

if [ -z "$1" ]
then
	echo "Path to folder containing source data must be supplied"
	exit 1
else
	source_dir=$1
fi
#source_dir=get_loadtest_source_data


function launch_parallel_get()
{
        test=$1
        n=$(wc -l $source_dir/uuids_$test | awk '{print $1}')
        echo "Launching $n get requests of $test files in parallel"
        mkdir -p $result_dir/$test
        i=1
        while read l
        do
                curl "http://$host:$metax_port/db/get?id=$l" 2> /dev/null 1>$result_dir/$test/$l &
                pids[$i]=$!
                i=$((i+1))
        done <$source_dir/uuids_$test
}

function wait_for_finish()
{
        FAIL=0
        echo "Waiting for all get requests to finish"
        for pid in ${pids[*]}; do
                wait $pid || FAIL=1
                if [ $FAIL != 0 ]; then
                        echo "Failed get from host:" $host
                        echo "Get request exited with non 0 status"
                        #exit 1;
                fi
        done
}

function check_file_correctness()
{
        test=$1
        echo "Checking correctness of received files of size $test"
        for f in $result_dir/$test/*
        do
                d=$(diff $f $source_dir/$test 2>&1)
                if [[ ! -z $d ]]; then
                        echo "File mismatch on $f get request"  
                        exit 1;
                fi
        done
}

rm -fr $result_dir

# launching parellel get requests of specified size of files
launch_parallel_get 3KB
#launch_parallel_get 3MB

# waiting for all get requests to finish
wait_for_finish

# checking correctness of received files
check_file_correctness 3KB
echo success
#check_file_correctness 3MB
