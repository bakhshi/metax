#!/bin/bash

# Test case flow: 
# - Runs fist instance of metax with config2.xml configuration.
# - Saves a content on the above instance
# - Runs second instance of metax with config1.xml configuration which connects
#   to the first one
# - Sends websocket connect request to second running metax (config1.xml) by
#   running websocket_client.js nodejs script
# - Registers on update event of the saved file in the first instance
# - Sends update request of the saved file to the first instance
# - if websocket connection is up and the update event successfully delivered
#   then the delivered message is checked


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
                kill_metax $p1 $p2
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids

        nodejs websocket_client.js $test_dir/data &
        sleep 2

        res=$(curl http://localhost:7081/db/register_listener?id=$suuid)
        echo $res
        res=$(echo $res | jq -r 'keys[]')
        echo $res
        if [[ $res != success ]]
        then
                echo "Failed to register on update of $suuid"
                kill_metax $p2 $p1
                exit
        fi
        
        uuuid=$(curl --data "update content" "http://localhost:7091/db/save/node?enc=0&id=$suuid")
        echo $uuuid
        uuuid=$(echo $uuuid | cut -d'"' -f 4)
        echo $uuuid
        if [ "$uuuid" != "$suuid" ]; then
                echo "Failed to update content"
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

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
