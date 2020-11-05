#!/bin/bash

# Test case flow: 
# - Runs and connects two instance of metax with config1.xml and config2.xml
#   configurations.
# - Sends "node" save request to first metax (config1.xml) for some content
#   (should be saved as non-encrypted).
# - Sends share request to the second running metax with username.
# - Checks the compliance of received/expected messages.
# - Checks the compliance of received/shared uuid and keys.
# - Sends db/accept_share request to metax running with config2.xml and
#   checks the response.
# - Sends get request to the second running metax to receive the shared
#   content.
# - Checks the compliance of saved/shared contents.


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
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
        save_content="Some content for share test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        res=$(curl -m 10 -F "username=friend2" http://localhost:7081/db/share?id=$uuid)
        echo $res
        share_uuid=$(echo $res | jq -r '.share_uuid')
        echo $share_uuid
        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')
        if [ "$share_uuid" != "$uuid" ]; then
                echo "Share uuid is not the same"
                kill_metax $p1 $p2;
                exit;
        fi
        if [[ $key == null ]] || [[ $iv == null ]]; then
                echo "Key and iv are not returned"
                kill_metax $p1 $p2;
                exit;
        fi
        if [[ "$key" != "" ]] || [[ "$iv" != "" ]]; then
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
                kill_metax $p1 $p2;
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2;
}

main $@
