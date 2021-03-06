#!/bin/bash

# Test case flow: 
# - Runs and connects four instances of metax (peer1->peer2, peer3->peer2, peer4->peer2).
# - Sends "node" save request for some content (should be saved as non-encrypted)
#   to the metax, running with config1.xml.
# - Sends get request for received uuid to the running metaxes.
# - Checks the compliance of received/saved contents.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p2
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config4.xml &
        p4=$!
        is_peer_connected=$(peers_connect 7111 $tst_src_dir/config4.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7111 does not connect to peer"
                kill_metax $p4 $p2
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p4 $p3 $p2
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p4 $p3 $p2 $p1
                exit;
        fi
        echo -n $p4 $p3 $p2 $p1 > $test_dir/processes_ids
}




clean_storage_content()
{
        echo "remove uuids (except user jsons) from $1 directory"
        str=""
        for i in ${!uj_uuids[@]}; do
                str+=" -o -name ${uj_uuids[$i]}"
        done
        find $1 -type f ! \( -name storage.json $str \) -delete
}




main()
{
        start_metaxes
        get_user_json_uuids 4
        save_content="Some data to save in peer1"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        get_content=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo $FAIL > $final_test_result
                echo "Failed getting data from peer1."
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi

        kill_metax $p1 $p2 $p3 $p4
        clean_storage_content $test_dir/storage1
        start_metaxes

        get_content=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo $FAIL > $final_test_result
                echo "Failed getting data from peer2."
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi

        kill_metax $p1 $p2 $p3 $p4
        rm -rf $test_dir/storage1/cache
        clean_storage_content $test_dir/storage2
        start_metaxes

        get_content=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo $FAIL > $final_test_result
                echo "Failed getting data from peer3."
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi

        kill_metax $p1 $p2 $p3 $p4
        rm -rf $test_dir/storage1/cache
        clean_storage_content $test_dir/storage3
        start_metaxes

        get_content=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo $FAIL > $final_test_result
                echo "Failed getting data from peer4."
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3 $p4
}

main $@
