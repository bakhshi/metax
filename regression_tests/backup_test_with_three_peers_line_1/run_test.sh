#!/bin/bash

# Test case flow: 
# - Runs and connects four instances of metax (peer1->peer2->peer3->peer4).
# - Sends "node" save request for some content (should be saved as non-encrypted)
#   to the metax, running with config1.xml.
# - Sends get request for received uuid to the running metaxes.
# - Checks the compliance of received/saved contents.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config4.xml &
        p4=$!
        is_peer_connected=$(peers_connect 7111 $tst_src_dir/config4.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7111 does not connect to peer"
                kill_metax $p4
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p4 $p3
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
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
	echo $str

        save_content="Some data to save in peer1"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        c1=$(ls $test_dir/storage1/????????-* | grep -v "$str" | expr $(wc -l))
	c2=$(ls $test_dir/storage2/????????-* | grep -v "$str" | expr $(wc -l))
	c3=$(ls $test_dir/storage3/????????-* | grep -v "$str" | expr $(wc -l))
	c4=$(ls $test_dir/storage4/????????-* | grep -v "$str" | expr $(wc -l))
	f1=$(grep "uuid, copy count - 1" $test_dir/leviathan1.log | expr $(wc -l))
	f2=$(grep "uuid, copy count - 2" $test_dir/leviathan1.log | expr $(wc -l))
	f3=$(grep "uuid, copy count - 3" $test_dir/leviathan1.log | expr $(wc -l))
	f4=$(grep "uuid, copy count - 4" $test_dir/leviathan1.log | expr $(wc -l))
        if [ "1" != "$f1" ] || [ "1" != "$f2" ] || [ "1" != "$f3" ] || [ "1" != "$f4" ] ; then
                echo "Failed to backup data f1=$f1,f2=$f2,f3=$f3,f4=$f4"
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi
        if [ "$c1" != "$c2" ] || [ "$c1" != "$c3" ] || [ "$c1" != "$c4" ] ; then
                echo "Failed to backup data c1=$c1,c2=$c2,c3=$c3,c4=$c4"
                kill_metax $p1 $p2 $p3 $p4
                exit;
        fi
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        kill_metax $p1 $p2 $p3 $p4
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed getting data from peer1. get_content=$get_content"
                exit;
        fi

        clean_storage_content $test_dir/storage1
        start_metaxes
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        kill_metax $p1 $p2 $p3 $p4
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed getting data from peer2. get_content=$get_content"
                exit;
        fi

        clean_storage_content $test_dir/storage2
        rm -rf $test_dir/storage1/cache
        start_metaxes
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        kill_metax $p1 $p2 $p3 $p4
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed getting data from peer3. get_content=$get_content"
                exit;
        fi

        clean_storage_content $test_dir/storage3
        rm -rf $test_dir/storage1/cache
        start_metaxes
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        kill_metax $p1 $p2 $p3 $p4
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed getting data from peer4. get_content=$get_content"
                exit;
        fi
        echo $PASS > $final_test_result
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
