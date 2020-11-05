#!/bin/bash

# Test case flow:
# - Runs and connects 4 instances of metax (peer1->peer2, peer3->peer2, peer4->peer2).
# - Sends "node" save request for some content (default it is saved as encrypted)
#   to the metax, running with config2.xml.
# - Sends "node" update request for received uuid to the metax running with config1.xml.
# - Waits 30 second for backups and then kills all running metax processes.
# - For all peers - runs a metax, sends get request for the updated uuid,
#   checks the compliance of received/updated contents, and then kills the process.

rm -rf $test_dir/storage1
rm -rf $test_dir/storage2
rm -rf $test_dir/storage3
rm -rf $test_dir/storage4


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
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

        save_content="Some data to save in peer2"
        uuid=$(curl --data "$save_content" http://localhost:7091/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME

        update_content="Some data to update in peer2"
        uuid1=$(curl --data "$update_content" http://localhost:7091/db/save/node?id=$uuid)
        uuid1=$(echo $uuid1 | cut -d'"' -f 4)
        sleep $TIME

        if [ "$uuid" == "$uuid1" ]; then
                sleep $TIME
                # Check old pieces are deleted or not.
		c1=$(ls $test_dir/storage1/????????-* | grep -v "$str" | expr $(wc -l))
		c2=$(ls $test_dir/storage2/????????-* | grep -v "$str" | expr $(wc -l))
		c3=$(ls $test_dir/storage3/????????-* | grep -v "$str" | expr $(wc -l))
		c4=$(ls $test_dir/storage4/????????-* | grep -v "$str" | expr $(wc -l))
                if [ "$c1" != "$c2" ] || [ "$c1" != "$c3" ] || [ "$c1" != "$c4" ] ; then
                        echo "Old pieces have not been deleted. c1=$c1, c2=$c2, c3=$c3, c4=$c4"
                        kill_metax $p1 $p2 $p3 $p4
                        exit;
                fi

                kill_metax $p1 $p2 $p3 $p4
                clean_storage_content $test_dir/storage2
                start_metaxes

                get_content=$(curl http://localhost:7091/db/get/?id=$uuid)
                if [ "$update_content" != "$get_content" ]; then
                        echo "GET request failed after removing storage2. get_content=$get_content"
                        kill_metax $p1 $p2 $p3 $p4
                        exit;
                fi

                kill_metax $p1 $p2 $p3 $p4
                rm -rf $test_dir/storage2/cache
                clean_storage_content $test_dir/storage1
                start_metaxes

                get_content=$(curl http://localhost:7091/db/get/?id=$uuid)
                if [ "$update_content" != "$get_content" ]; then
                        echo "GET request failed after removing storage1. get_content=$get_content"
                        kill_metax $p1 $p2 $p3 $p4
                        exit;
                fi

                kill_metax $p1 $p2 $p3 $p4
                rm -rf $test_dir/storage2/cache
                clean_storage_content $test_dir/storage3
                start_metaxes

                get_content=$(curl http://localhost:7091/db/get/?id=$uuid)
                if [ "$update_content" != "$get_content" ]; then
                        echo "GET request failed after removing storage3. get_content=$get_content"
                        kill_metax $p1 $p2 $p3 $p4
                        exit;
                fi
                echo $PASS > $final_test_result
        fi
        kill_metax $p1 $p2 $p3 $p4
}

main $@
