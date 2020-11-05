#!/bin/bash

# Test case flow:
# - Runs first instance of metax with config2.xml configuration.
# - Sends save request to the first metax.
# - Runs second instance of metax with config1.xml configuration which connects
#   to the first one.
# - Sends websocket connect request to second running metax (config1.xml) by
#   running websocket_client.js nodejs script
# - Registers on delete event of the saved file in the first instance
# - Sends delete request to the first metax.
# - Check that elete event successfully delivered.

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

        res=$(curl -D $test_dir/h http://localhost:7081/db/register_listener?id=$suuid)
        echo "Received $res"
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | grep OK | awk {'print $2'})
        if [[ 200 != $http_code ]]; then
                echo "HTTP code of REGISTER_LISTENER request response is $http_code"
                kill_metax $p1 $p2
                exit;
        fi
        expected='{"success": "registered listener for '$suuid'"}'
        if [[ $res != $expected ]]; then
                echo "Mismatch of expected and received response. Res = $res"
                kill_metax $p2 $p1
                exit
        fi

        expected='{"deleted":"'$suuid'"}'
        res=$(curl "http://localhost:7091/db/delete?id=$suuid")
        if [ "$expected" != "$res" ]; then
                echo "Failed to delete $suuid"
                kill_metax $p1 $p2;
                exit;
        fi
        sleep 2

        data=$(cat $test_dir/data)
        expected="{\"event\":\"deleted\",\"uuid\":\"$suuid\"}"
        if [ "$data" != "$expected" ]; then
                echo "Deleted event isn't received. data is $data"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
