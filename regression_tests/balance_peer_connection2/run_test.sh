#!/bin/bash

# Test case flow:
# - There are seven Metax instances
# - the connection scheme is:
#   2->5  3->5,6  4->5,6,7
#   1->[2,3,4] - where [x,y,z] means Metax balancer will choose only one
#   of the x or y or z connections
# - first it launches the instances:
#   7, 6, 5, 4, 3, 1 (2 is omitted)
# - checks that 1 is connected to 3, because it is less loaded (2 peers) than 4
#   (3 peers)
# - second it launches the instances:
#   7, 6, 5, 3, 2, 1 (4 is omitted)
# - checks that this time 1 is connected to 2, because it is less loaded (1 peers) than 3
#   (2 peers)


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
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p1
                exit
        fi
        echo -n $p7 $p6 $p5 $p4 $p3 $p1 > processes_ids

	sleep 5
	op=$(curl http://localhost:7081/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 1 ]]; then
		echo "$op"
		echo "number of online peers of 7081 (config1.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p1
		exit
	fi
	a=$(echo $op | jq '.[0]' | jq -r '.address')
	echo $a
	expected="127.0.0.1:7084"
	if [[ "$a" != "$expected" ]]; then
		echo "balancer has chosen wrong peer - $a, should be $expected"
                kill_metax $p7 $p6 $p5 $p4 $p3 $p1
		exit
	fi
	kill_metax $p4 $p1

        $EXECUTE -f $tst_src_dir/config2.xml &
	p2=$!
        is_peer_connected=$(peers_connect 7083 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
		echo "7083 (config2.xml) does not connect to peers"
                kill_metax $p7 $p6 $p5 $p3 $2
                exit;
        fi
	$EXECUTE -f $tst_src_dir/config1.xml &
	p1=$!
        is_http_server_started=$(wait_for_init 7081)
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p7 $p6 $p5 $p3 $p2 $p1
                exit
        fi
        echo -n $p7 $p6 $p5 $p3 $p2 $p1 > processes_ids
	sleep 5
	op=$(curl http://localhost:7081/config/get_online_peers 2> /dev/null)
	l=$(echo $op | jq '. | length')
	if [[ $l -ne 1 ]]; then
		echo "$op"
		echo "number of online peers of 7081 (config1.xml) is wrong - $l"
                kill_metax $p7 $p6 $p5 $p3 $p2 $p1
		exit
	fi
	a=$(echo $op | jq '.[0]' | jq -r '.address')
	echo $a
	expected="127.0.0.1:7082"
	if [[ "$a" != "127.0.0.1:7082" ]]; then
		echo "balancer has chosen wrong peer - $a, should be $expected"
                kill_metax $p7 $p6 $p5 $p3 $p2 $p1
		exit
	fi
        echo $PASS > $final_test_result
	kill_metax $p7 $p6 $p5 $p3 $p2 $p1
}

main $@
