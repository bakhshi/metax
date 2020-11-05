#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax.
# - Sends "node" save request for some content with enc=0 parameter.
# - Waits about 10 second for backup then kills all metaxes.
# - Runs metax with config2.xml configuration.
# - Sends get request for received uuid to running metax.
# - Checks the compliance of received/saved contents.



source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



clean_storage_content()
{
        echo "remove uuids (except user jsons) from $1 directory"
        str=""
        for i in ${!uj_uuids[@]}; do
                str+=" -o -name ${uj_uuids[$i]}"
        done
        find $1 -type f ! \( -name storage.json $str \) -delete
}




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
        get_user_json_uuids 2
        save_content="Some content for test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid\&cache=0)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content=$get_content"
                kill_metax $p1 $p2
                exit;
        fi

        copy_res=$(curl http://localhost:7081/db/copy?id=$uuid)
        copy_uuid=$(echo $copy_res | jq -r '.copy_uuid')
        orig_uuid=$(echo $copy_res | jq -r '.orig_uuid')
        if [ "$orig_uuid" != "$uuid" ]; then
                echo "Incorrect copy request response : $copy_res"
                kill_metax $p1 $p2
                exit;
        fi
        sleep $TIME

        get_content=$(curl http://localhost:7081/db/get/?id=$copy_uuid\&cache=0)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed to get copied data from peer1. get_content=$get_content"
                kill_metax $p1 $p2
                exit;
        fi

        kill_metax $p1 $p2
        clean_storage_content $test_dir/storage1
        start_metaxes
        expected="{\"error\":\"Getting file failed: uuid=$copy_uuid.\"}"
        res=$(curl http://localhost:7081/db/get/?id=$copy_uuid)
        if [ "$expected" != "$res" ]; then
                echo "Could not copy data only one time. res=$res, expected=$expected"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
