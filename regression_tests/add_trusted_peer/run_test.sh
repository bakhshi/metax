#!/bin/bash

# Test case flow:
# - launches Metax instance 2 with strict verification mode
# - launches Metax instance 1 connecting to instance 2, but 1 isn't trusted by 2
# - checks that connection is dropped
# - adds 1 as trusted peer to 2
# - launches 1 connecting to instance 2
# - checks that connection is confirmed
# - removes 1 from 2 trusted peers list
# - launches 1 connecting to instance 2
# - checks that connection is dropped


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        rm -f $test_dir/trusted_peers2.json
        $EXECUTE -f $tst_src_dir/config2.xml &
	p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p2 $p1
                exit
        fi
        echo -n $p2 $p1 > processes_ids
	sleep 5
	op=$(curl http://localhost:7091/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 0 ]]; then
		echo "online peers list should be empty"
                kill_metax $p2 $p1
		exit
	fi

	kill_metax $p1
	dpbk1=$(cat $test_dir/keys1/device/public.pem)
	curl -F "key=$dpbk1" http://localhost:7091/config/add_trusted_peer
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7081 does not connect to peers"
                kill_metax $p2 $p1
                exit
        fi

	kill_metax $p1
	curl -F "key=$dpbk1" http://localhost:7091/config/remove_trusted_peer
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                exit
        fi
	sleep 5
	op=$(curl http://localhost:7091/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 0 ]]; then
		echo "online peers list should be empty also this time"
                kill_metax $p2 $p1
		exit
	fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2;
}

main $@
