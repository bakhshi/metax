#!/bin/bash

# Test case flow:
# - There are seven Metax instances being launched with the following order:
#   7, 6, 5 4, 3, 2, 1
# - the connection scheme is:
#   2->3  3->4  5->6  6->7
#   1->2,[3,4],5,[6,7] - where [x,y] means Metax balancer will choose only one
#   of the x or y connections
# - after all instances are running sends get_online_peers request to instance 1
# - checks that it has only 4 connections, because from 3 and 4 only one will
#   stay connected and from 6 and 7 only one will stay connected


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        $EXECUTE -f $tst_src_dir/config7.xml &
	p7=$!
        is_peer_connected=$(peers_connect 7093 $tst_src_dir/config7.xml)
        if [ "0" != "$is_peer_connected" ];then
		echo "7093 (config7.xml) does not connect to peers"
                kill_metax $p7
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config6.xml &
	p6=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config6.xml)
        if [ "0" != "$is_peer_connected" ];then
		echo "7091 (config6.xml) does not connect to peers"
                kill_metax $p7 $p6
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config5.xml &
	p5=$!
        is_peer_connected=$(peers_connect 7089 $tst_src_dir/config5.xml)
        if [ "0" != "$is_peer_connected" ];then
		echo "7089 (config5.xml) does not connect to peers"
                kill_metax $p7 $p6 $p5
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config4.xml &
	p4=$!
        is_peer_connected=$(peers_connect 7087 $tst_src_dir/config4.xml)
        if [ "0" != "$is_peer_connected" ];then
		echo "7087 (config4.xml) does not connect to peers"
                kill_metax $p7 $p6 $p5 $p4
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config3.xml &
	p3=$!
        is_peer_connected=$(peers_connect 7085 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
		echo "7085 (config3.xml) does not connect to peers"
                kill_metax $p7 $p6 $p5 $p4 $p3
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
	p2=$!
        is_peer_connected=$(peers_connect 7083 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
		echo "7083 (config2.xml) does not connect to peers"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2
                exit;
        fi
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
                exit
        fi
        echo -n $p7 $p6 $p5 $p4 $p3 $p2 $p1 > processes_ids

	sleep 6
	op=$(curl http://localhost:7081/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 4 ]]; then
		echo "number of online peers of 7081 (config1.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
		exit
	fi
	op=$(curl http://localhost:7083/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 2 ]]; then
		echo "number of online peers of 7083 (config2.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
		exit
	fi
	op=$(curl http://localhost:7085/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 2 ]]; then
		echo "number of online peers of 7085 (config3.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
		exit
	fi
	op=$(curl http://localhost:7087/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 2 ]]; then
		echo "number of online peers of 7087 (config4.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
		exit
	fi
	op=$(curl http://localhost:7089/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 2 ]]; then
		echo "number of online peers of 7089 (config5.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
		exit
	fi
	op=$(curl http://localhost:7091/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 2 ]]; then
		echo "number of online peers of 7091 (config6.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
		exit
	fi
	op=$(curl http://localhost:7093/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 2 ]]; then
		echo "number of online peers of 7093 (config7.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
		exit
	fi
        echo $PASS > $final_test_result
	kill_metax $p7 $p6 $p5 $p4 $p3 $p2 $p1
}

main $@
