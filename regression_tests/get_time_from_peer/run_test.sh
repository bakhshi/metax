#!/bin/bash

# Test case flow:
# - Runs one instance of metax and connects to Gyumri's instance
# - Requests a file from localhost which is retrieved from Gyumri peer
# - Checks the following criteria
#       - getting the whole file shouldn't exceed 0.2 seconds

if [ "yes" == "$use_valgrind" ]; then
        echo $SKIP > $final_test_result
        exit;
fi

get_file=$test_dir/get_file

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        pid=$!
        is_http_server_started=$(wait_for_init 7081)
        echo -n $pid > $test_dir/processes_ids
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $pid
                exit
        fi
        sleep 5
        op=$(curl http://localhost:7081/config/get_online_peers 2> /dev/null)
        echo "get_online_peers request response is $op"
        l=$(echo $op | grep -o "address" | expr $(wc -l))
        if [[ 1 -ne $l ]]; then
                echo "online peers count should be 1"
                kill_metax $pid
                exit
        fi

        # This uuid exists in Gyumri and expect to find from there
        uuid=28749ace-b534-4185-a5c5-b36cd7713288
        { time curl http://localhost:7081/db/get/?id=$uuid -o $get_file 2> /dev/null; } 2>$test_dir/time_log
        d=$(diff $get_file $tst_src_dir/out.gold 2>&1)
        if [[ ! -z $d ]]
        then
                echo "getting file failed"
                kill_metax $pid
                exit
        fi

        a=$(cat $test_dir/time_log | grep real | awk '{print $2}')
        t1=$(echo $a | cut -d'm' -f1)
        t2=$(echo $a | cut -d'm' -f2 | cut -d's' -f1)
        t1=$(bc <<< "scale=3; 60*$t1")
        t=$(bc <<< "scale=3; $t1 + $t2")
        echo the get time is $t
        echo time limit is 0.2s
        if (( $(bc <<<"$t< .2") )) ; then
                echo $PASS > $final_test_result
        fi
        kill_metax $pid
}

main $@
