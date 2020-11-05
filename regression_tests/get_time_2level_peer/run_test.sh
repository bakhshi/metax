#!/bin/bash

# Test case flow: 
# - Runs one instance of metax and connects to Gyumri
# - Gets a file that exists only in Vanadzor
# - Measures the overall time of getting the whole file from Vanadzor through
#   Gyumri
# - Checks the get time to not exceed 0.7s

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
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $pid
                exit;
        fi
	echo -n $pid > $test_dir/processes_ids
        uuid=35309175-a1b0-4896-ba14-3549ea90f5ce

        # clean cache of direct peer
        peer=metax@172.17.10.14
        pass='metax'

        curl http://localhost:7081/db/get?id=$uuid 2>/dev/null 1> $get_file

        cat $test_dir/leviathan.log | grep test_log > $test_dir/test_log
        ge=$(cat $test_dir/test_log | grep "\!get_request_response\!" | cut -d'!' -f 5)
        gb=$(cat $test_dir/test_log | grep "\!get_request\!" | cut -d'!' -f 7)
        gt=$(expr $ge - $gb)
        echo "Overall get time: $gt microseconds"

        echo $gt

        d=$(diff $get_file $tst_src_dir/leviathan_log.svg 2>&1)
        if [[ -z $d ]] && [[ "$gt" -lt 700000 ]]; then echo $PASS > $final_test_result; fi
	kill_metax $pid
}

main $@
