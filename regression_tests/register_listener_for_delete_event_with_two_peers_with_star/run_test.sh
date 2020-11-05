#!/bin/bash

# Test case flow:
# - Runs first instance of metax with config2.xml configuration.
# - Saves a content on the above instance.
# - Runs two instances of metax ( metax1 [storage_class = 4] -> metax, metax3 [storage_class = 4] -> metax ).
# - Sends websocket connect request to running metax1 and metax3 by
#   running websocket_client.js nodejs script.
# - Sends register_listener request to metax1 and metax3.
# - Sends delete request of the saved file to the first instance.
# - Check that update event successfully delivered.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0







main()
{
        npm install ws
        cp $tst_src_dir/websocket_client.js .
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        suuid=$(curl --data "save content" http://localhost:7091/db/save/node?enc=0)
        suuid=$(echo $suuid | cut -d'"' -f 4)

        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p3 $p2
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p3 $p2 $p1
                exit;
        fi
        echo -n $p3 $p2 $p1 > $test_dir/processes_ids
        get_user_json_uuids 3
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

	storage1_size=$(find $test_dir/storage1 -name "????????-*" | grep -v "$str" | expr $(wc -l))
	storage3_size=$(find $test_dir/storage3 -name "????????-*" | grep -v "$str" | expr $(wc -l))
        if [ "0" != "$storage1_size" ] || [ "0" != "$storage3_size" ]; then
                echo "storage1 storage3 should be empty."
                kill_metax $p1 $p2 $p3
                exit;
        fi

        nodejs websocket_client.js $test_dir/data 7081 &
        nodejs websocket_client.js $test_dir/data1 7101 &
        sleep 2

        expected='{"success": "registered listener for '$suuid'"}'
        res=$(curl http://localhost:7081/db/register_listener?id=$suuid)
        if [ "$res" != "$expected" ];then
                echo "Failed to register Peer1: Mismatch of expected and received response. Res = $res"
                kill_metax $p2 $p1 $p3
                exit;
        fi

        res=$(curl http://localhost:7101/db/register_listener?id=$suuid)
        if [ "$res" != "$expected" ];then
                echo "Failed to register Peer2: Mismatch of expected and received response. Res = $res"
                kill_metax $p2 $p1 $p3
                exit;
        fi

        expected='{"deleted":"'$suuid'"}'
        res=$(curl "http://localhost:7091/db/delete?id=$suuid")
        if [ "$expected" != "$res" ]; then
                echo "Failed to delete $suuid"
                kill_metax $p1 $p2 $p3
                exit;
        fi

        sleep 2
        data=$(cat $test_dir/data)
        expected="{\"event\":\"deleted\",\"uuid\":\"$suuid\"}"
        if [ "$data" != "$expected" ]; then
                echo "Peer1 did not receive deleted event."
                kill_metax $p1 $p2 $p3
                exit;
        fi 

        data1=$(cat $test_dir/data1)
        expected="{\"event\":\"deleted\",\"uuid\":\"$suuid\"}"
        if [ "$data" != "$expected" ]; then
                echo "Peer2 did not receive deleted event."
                kill_metax $p1 $p2 $p3
                exit;
        fi 

        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3
}

main $@
