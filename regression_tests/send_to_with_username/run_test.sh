#!/bin/bash

# Test case flow: 
# - Runs and connects two instances of metax with config1.xml and config2.xml
#   configurations.
# - Sends websocket connect request to second running metax (config2.xml) by
#   running websocket_client.js nodejs script.
# - sends a message to the second peer via peer1 with username.
# - if websocket connection is up and the message successfully delivered then
#   the delivered message is checked with the original one
# - Checks the compliance of received/expected messages.
# - Checks the compliance of received/sent data.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



start_metaxes()
{ 
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
}



main()
{
	cd $test_dir
        cp $tst_src_dir/websocket_client.js .
        npm install ws
	start_metaxes
        nodejs websocket_client.js $test_dir/data &
	sleep 2
	content="Some content to send to test"
	res=$(curl -F "username=friend2" -F "data=$content" http://localhost:7081/db/sendto)
	echo "Response is $res"
	expected='{"msg":"Data is delivered successfully"}'
	socketoff='{"warning":"Failed to deliver data to friend for the request with friend2"}'
	if [ "$res" == "$socketoff" ]; then
		echo "The WebSocket is off on receiver side."
		echo $PASS > $final_test_result
		kill_metax $p1 $p2
		exit;
	fi
	sleep 2
	if [ "$res" != "$expected" ]; then
		echo "Failed to deliver data"
		kill_metax $p1 $p2
		exit;
	fi
	sleep 2
	data=$(cat $test_dir/data)
	if [ "$data" != "$content" ]; then
		echo "Received incorrect data"
		kill_metax $p1 $p2
		exit;
	fi 

	echo $PASS > $final_test_result
	kill_metax $p1 $p2
}

main $@
