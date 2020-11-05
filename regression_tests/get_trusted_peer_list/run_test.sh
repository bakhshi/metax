#!/bin/bash

# Test case flow:
# - launches Metax instance 2 with strict verification mode
# - launches Metax instance 1 connecting to instance 2, but 1 isn't trusted by 2
# - gets trusted peers list
# - checks that list is empty
# - adds 1 as trusted peer to 2
# - gets trusted peers list
# - checks that 1 is trusted by 2
# - kills then launches 2 connecting
# - gets trusted peers list
# - checks that 1 is trusted by 2


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
	p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        is_http_server_started=$(wait_for_init 7081)
        echo -n $p2 $p1 > $test_dir/processes_ids

        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p2 $p1
                exit
        fi
	gtpl=$(curl http://localhost:7091/config/get_trusted_peer_list 2> /dev/null)
        echo "get_trusted_peer_list request response is $gtpl"
	l=$(echo $gtpl | jq '. | length')
	if [[ 0 -ne $l ]]; then
		echo "trusted peers list should be empty"
                kill_metax $p2 $p1
		exit
	fi

        dpbk1=$(cat $test_dir/keys1/device/public.pem)
	curl -F "key=$dpbk1" http://localhost:7091/config/add_trusted_peer
	gtpl=$(curl http://localhost:7091/config/get_trusted_peer_list 2> /dev/null)
        echo ""
        echo "get_trusted_peer_list request response is $gtpl"
	l=$(echo $gtpl | jq '. | length')
	if [[ 0 -eq $l ]]; then
		echo "trusted peers list should not be empty"
                kill_metax $p2 $p1
		exit
	fi

	kill_metax $p2

	$EXECUTE -f $tst_src_dir/config2.xml &
	p2=$!
        is_http_server_started=$(wait_for_init 7091)
        echo -n $p2 $p1 > $test_dir/processes_ids
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7091"
                kill_metax $p2 $p1
                exit
        fi

	gtpl=$(curl http://localhost:7091/config/get_trusted_peer_list 2> /dev/null)
        echo ""
        echo "get_trusted_peer_list request response is $gtpl"
	l=$(echo $gtpl | jq '. | length')
	if [[ 0 -eq $l ]]; then
		echo "trusted peers list should not be empty"
                kill_metax $p2 $p1
		exit
	fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2;
}

main $@
