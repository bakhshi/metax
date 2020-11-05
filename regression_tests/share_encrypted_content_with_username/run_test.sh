#!/bin/bash

# Test case flow: 
# - Runs and connects two instance of metax with config1.xml and config2.xml
#   configurations.
# - Sends websocket connect request to second running metax (config2.xml) by
#   running websocket_client.js nodejs script.
# - Sends "node" save request to first metax (config1.xml) for some content
#   (should be saved as encrypted).
# - Sends share request to the second running metax with username.
# - Checks the compliance of received/expected messages.
# - Checks the compliance of received/shared uuids.
# - Checks the correctness of origin uuid.
# - Sends get request to the second running metax to receive the shared content.
# - Checks the compliance of saved/shared contents.

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
	start_metaxes
	save_content="Some content for share test"
	uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
	uuid=$(echo $uuid | cut -d'"' -f 4)
	res=$(curl -m 10 -F "username=friend2" http://localhost:7081/db/share?id=$uuid)
        echo "res = $res"
        share_uuid=$(echo $res | jq -r '.share_uuid')
        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')
        if [ "$share_uuid" != "$uuid" ]; then
                echo "Share uuid is not same"
                kill_metax $p1 $p2;
                exit;
        fi
        if [[ $key == null ]] || [[ $iv == null ]]; then
                echo "Key and iv are not returned"
                kill_metax $p1 $p2;
                exit;
        fi
        if [[ "$key" == "" ]] || [[ "$iv" == "" ]]; then
                echo "Key and iv are not empty"
                kill_metax $p1 $p2;
                exit;
        fi
        received=$(curl -m 10 -F "key=$key" -F "iv=$iv" http://localhost:7091/db/accept_share?id=$uuid)
        expected='{"share":"accepted"}'
        if [ "$expected" != "$received" ]; then
                echo "Share accept failed"
                kill_metax $p1 $p2;
                exit;
        fi
        share_info=$(curl http://localhost:7091/db/get?id=$share_uuid)
        if [ "$share_info" != "$save_content" ]; then
                echo "received incorrect shared data"
                echo "share_info=$share_info"
                echo "save_content=$save_content"
                kill_metax $p1 $p2;
                exit;
        fi

	echo $PASS > $final_test_result
        kill_metax $p1 $p2;
}

main $@
