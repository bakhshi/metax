#!/bin/bash

# Test case flow:
# - Runs one instance of metax with config2.xml configuration.
# - Saves a content on the above instance
# - Runs second instance of metax with config1.xml configuration(metax1[storage_class=4] -> metax2)
# - Sends websocket connect request to metax1 by running websocket_client.js nodejs script
# - Registers on update event of the saved file in the first instance
# - Sends update request of the saved file to the first instance
# - Checks delivery of update notification by websocket.
# - Sends unregister listener request for previously registered uuid.
# - Checks that unregister listener request is successfully finished.
# - Sends delete request of the saved file to the first instance.
# - Checks that there is no message in websocket with update notification.

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
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p2
                exit;
        fi
        suuid=$(curl --data "save content" http://localhost:7091/db/save/node?enc=0)
        suuid=$(echo $suuid | cut -d'"' -f 4)
        sleep 1

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi

        echo -n $p2 $p1 > $test_dir/processes_ids
        nodejs websocket_client.js $test_dir/data 7081 &
        sleep 2

        res=$(curl http://localhost:7081/db/register_listener?id=$suuid)
        expected='{"success": "registered listener for '$suuid'"}'
        if [ "$res" != "$expected" ];then
                echo "Failed to register on update of $suuid"
                kill_metax $p2 $p1
                exit
        fi

        res=$(curl --data "update content" "http://localhost:7091/db/save/node?enc=0&id=$suuid")
        uuid=$(echo $res | cut -d'"' -f 4)
        if [ "$uuid" != "$suuid" ]; then
                echo "Failed to update content. Res=$res"
                kill_metax $p1 $p2;
                exit;
        fi
        sleep 2
        data=$(cat $test_dir/data)
        expected="{\"event\":\"updated\",\"uuid\":\"$suuid\"}"
        if [ "$data" != "$expected" ]; then
                echo "Updated event isn't received"
                kill_metax $p1 $p2
                exit;
        fi

        res=$(curl -D $test_dir/h http://localhost:7081/db/unregister_listener?id=$suuid)
        echo "Received $res"
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        if [[ 200 != $http_code ]]; then
                echo "HTTP code of UNREGISTER_LISTENER request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi
        expected='{"success": "listener unregistered for '$suuid'"}'
        if [[ "$res" != "$expected" ]]; then
                echo "Failed to unregister Peer .Res=$res"
                kill_metax $p2 $p1
                exit
        fi

        expected='{"deleted":"'$suuid'"}'
        res=$(curl "http://localhost:7091/db/delete?id=$suuid")
        if [ "$expected" != "$res" ]; then
                echo "Failed to delete content"
                kill_metax $p1 $p2;
                exit;
        fi
        sleep 2

        data=$(cat $test_dir/data)
        expected="{\"event\":\"deleted\",\"uuid\":\"$suuid\"}"
        if [ "$data" == "$expected" ]; then
                echo "Deleted event should not be notify."
                kill_metax $p1 $p2
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
