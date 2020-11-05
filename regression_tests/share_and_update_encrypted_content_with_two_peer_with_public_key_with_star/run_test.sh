#!/bin/bash

# Test case flow: 
# - Runs and connects three instance of metax with star connection (peer1 -> peer2, peer1 ->3peer3).
# - Sends save encrypted content request to peer1.
# - Sends share request to peer2 and peer3 with public keys.
# - Sends update request to peer1.
# - Sends get requests to peer2 and peer3 for check share request response.
# - Sends update request to peer2
# - Sends get requests to peer1, peer2 and peer3 for check share request resposne.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




save_content="Some content for share test"

main()
{
        npm install ws
        cp $tst_src_dir/websocket_client.js .

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        echo -n $p1 > $test_dir/processes_ids

        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "uuid = $uuid"


        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p3 $p1
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p3 $p2 $p1
                exit;
        fi
        echo -n $p2 $p3 >> $test_dir/processes_ids

        nodejs websocket_client.js $test_dir/data 7091 &
        nodejs websocket_client.js $test_dir/data1 7101 &
        sleep 3

        get_public_key=$(curl http://localhost:7091/config/get_user_public_key) 
        public_key=$(echo $get_public_key | jq -r '.user_public_key')
        echo "public_key=$public_key"

        get_public_key1=$(curl http://localhost:7101/config/get_user_public_key)
        public_key1=$(echo $get_public_key1 | jq -r '.user_public_key')
        echo "public_key1=$public_key1"


        res=$(curl -m 10 -F "key=$public_key" http://localhost:7081/db/share?id=$uuid)

	share_uuid=$(echo $res | jq -r '.share_uuid')
        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')

        res1=$(curl -m 10 -F "key=$public_key1" http://localhost:7081/db/share?id=$uuid)

        share_uuid1=$(echo $res1 | jq -r '.share_uuid')
        key1=$(echo $res1 | jq -r '.key')
        iv1=$(echo $res1 | jq -r '.iv')

        if [ $share_uuid != $share_uuid1 ];then
                echo "Shared uuids are different."
                kill_metax $p1 $p2 $p3
                exit;
        fi

        received=$(curl -m 10 -F "key=$key" -F "iv=$iv" http://localhost:7091/db/accept_share?id=$share_uuid)
        expected='{"share":"accepted"}'
        if [ "$expected" != "$received" ]; then
                echo "Share accept failed from peer2. received=$received"
                kill_metax $p1 $p2 $p3
                exit;
        fi

        received1=$(curl -m 10 -F "key=$key1" -F "iv=$iv1" http://localhost:7101/db/accept_share?id=$share_uuid)
        if [ "$expected" != "$received1" ]; then
                echo "Share accept failed from peer3. received=$received1"
                kill_metax $p1 $p2 $p3;
                exit;
        fi

        expected='{"success": "registered listener for '$share_uuid'"}'
        res=$(curl http://localhost:7091/db/register_listener?id=$share_uuid)
        echo $res
        res1=$(curl http://localhost:7101/db/register_listener?id=$share_uuid)
        echo $res1
        if [ "$res" != "$expected" ] || [ "$res1" != "$expected" ];then
                echo "Failed to register on update of $share_uuid"
                kill_metax $p3 $p2 $p1
                exit
        fi

        # Checking that content was shared
        get=$(curl "http://localhost:7091/db/get?id=$share_uuid")
        get1=$(curl "http://localhost:7101/db/get?id=$share_uuid")
        echo "getting shared content from peer2: $get"
        echo "getting shared content from peer3: $get1"
        if [ "$save_content" != "$get" ] || [ "$save_content" != "$get1" ];then
                echo "Failed to get shared content from peers"
                kill_metax $p1 $p2 $p3
                exit;
        fi

        update_content="Update shared content"

        curl --data "$update_content" http://localhost:7081/db/save/node?id=$share_uuid > /dev/null
        sleep $TIME

        data=$(cat $test_dir/data)
        data1=$(cat $test_dir/data1)

        expected="{\"event\":\"updated\",\"uuid\":\"$share_uuid\"}"
        if [ "$data" != "$expected" ] || [ "$data1" != "$expected" ]; then
                echo "Updated event isn't received $data, $data1"
                kill_metax $p1 $p2 $p3
                exit;
        fi 

        get=$(curl "http://localhost:7091/db/get?id=$share_uuid")
        echo "getting updated content from peer2: $get"
        if [ "$update_content" != "$get" ];then
                echo "Failed to get updated content from peer"
                kill_metax $p1 $p2 $p3
                exit;
        fi
        get1=$(curl "http://localhost:7101/db/get?id=$share_uuid")
        echo "getting updated content from peer2: $get1"
        if [ "$update_content" != "$get1" ];then
                echo "Failed to get updated content from peer"
                kill_metax $p1 $p2 $p3
                exit;
        fi
        rm $test_dir/data
        rm $test_dir/data1
        
        # Send update content from peer2
        update_content1="Update shared content from peer2"
        curl --data "$update_content1" http://localhost:7091/db/save/node?id=$share_uuid > /dev/null
        sleep 2

        data=$(cat $test_dir/data)
        data1=$(cat $test_dir/data1)

        expected="{\"event\":\"updated\",\"uuid\":\"$share_uuid\"}"
        if [ "$data" != "$expected" ] || [ "$data1" != "$expected" ]; then
                echo "Updated event isn't received $data, $data1"
                kill_metax $p1 $p2 $p3
                exit;
        fi 
        # Check that updated all peers.
        get=$(curl "http://localhost:7081/db/get?id=$share_uuid")
        echo "getting last updated content from peer1: $get"
        if [ "$update_content1" != "$get" ];then
                echo "Failed to get last updated content from peer1."
                kill_metax $p1 $p2 $p3
                exit;
        fi

        get1=$(curl "http://localhost:7091/db/get?id=$share_uuid")
        echo "getting last updated content from peer2: $get1"
        if [ "$update_content1" != "$get1" ];then
                echo "Failed to get last updated content from peer2."
                kill_metax $p1 $p2 $p3
                exit;
        fi
        get2=$(curl "http://localhost:7101/db/get?id=$share_uuid")
        echo "getting last updated content from peer2: $get2"
        if [ "$update_content1" != "$get2" ];then
                echo "Failed to get last updated content from peer3"
                kill_metax $p1 $p2 $p3
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3
}

main $@
