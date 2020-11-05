#!/bin/bash

# Test case flow: 
# - Runs and connects four instances of metax (peer1->peer2, peer3->peer2, peer4->peer2).
# - Sends "node" save request for some content (default it is saved as encrypted)
#   to the metax, running with config2.xml.
# - Sends get request for received uuid to the running metaxes.
# - Checks the compliance of received/saved contents.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


check_storage_content()
{
        while read l; do
                if  [[ ! "${uj_uuids[@]}" =~ "${l}" ]]; then
                        return 1;
                fi
        done < $1
        return 0;
}

clean_storage_content()
{
        echo "remove uuids(except user jsons) from $1 directory"
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




main()
{
        start_metaxes
        get_user_json_uuids 4
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')
        save_content="Some data to save in peer2"
        uuid=$(curl --data "$save_content" http://localhost:7091/db/save/node?local=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME

        kill_metax $p1 $p2 $p3 $p4
        ls $test_dir/storage2/????????-* | grep -v "$str" | rev | cut -d '/' -f1 | rev > $test_dir/uuids

        check_storage_content $test_dir/uuids
        if [ "0" != "$?" ];then
                echo "Storage2 should contain only uuids of user json."
                exit;
        fi

        start_metaxes
        get_content=$(curl http://localhost:7091/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed getting data from peer2."
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi

        kill_metax $p1 $p2 $p3 $p4
        rm -rf $test_dir/storage2/cache
        clean_storage_content $test_dir/storage1
        start_metaxes

        get_content=$(curl http://localhost:7091/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed getting data from peer3."
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi

        kill_metax $p1 $p2 $p3 $p4
        rm -rf $test_dir/storage2/cache
        clean_storage_content $test_dir/storage4
        start_metaxes

        get_content=$(curl http://localhost:7091/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed getting data from peer4."
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p1 $p2 $p3 $p4
}

main $@
