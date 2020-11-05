#!/bin/bash

# Test case flow:
# - Runs and connects four instances of metax with line connection
#   (peer1->peer2->peer3->peer4).
# - Sends "path" save request for some random generated file 
#   (should be saved as non-encrypted).
# - Waits about 5 second for backup then sends update request.
# - Waits about 15 second, checks for existence of backup,
#   then kills all running metaxes.
# - Runs metaxes with config2.xml, config3.xml and config4.xml configurations.
# - Sends get request for received uuid to the running metaxes.
# - Checks the compliance of received/saved contents.

save_file=$test_dir/save_file
update_file=$test_dir/update_file
get_file=$test_dir/get_file
rm -rf $test_dir/storage1
rm -rf $test_dir/storage2
rm -rf $test_dir/storage3
rm -rf $test_dir/storage4


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




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
                kill_metax $p2 $p3 $p4
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

        diff_options="--exclude=cache --exclude=storage.json"
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done

        echo "diff_options=$diff_options"

        dd if=/dev/urandom of=$save_file bs=16000000 count=1
        uuid=$(curl --data "$save_file" http://localhost:7081/db/save/path?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        dd if=/dev/urandom of=$update_file bs=16000000 count=1
        sleep $TIME
        res=$(curl --data "$update_file" http://localhost:7081/db/save/path?id=$uuid\&enc=0)
        uuid1=$(echo $res | cut -d'"' -f 4)
        if [ "$uuid" == "$uuid1" ]; then
                sleep $TIME
                echo "UUIDs are the same"
                # Check old pieces are deleted or not.
                c1=$(diff -r $test_dir/storage1/ $test_dir/storage2/ $diff_options)
                c2=$(diff -r $test_dir/storage1/ $test_dir/storage3/ $diff_options)
                c3=$(diff -r $test_dir/storage1/ $test_dir/storage4/ $diff_options)

                if [[ -n $c1 || -n $c2 || -n $c3 ]] ; then
                        echo "Storage directories are not the same. c1=$c1, c2=$c2, c3=$c3"
                        kill_metax $p4 $p3 $p2 $p1
                        exit;
                fi

                curl http://localhost:7081/db/get/?id=$uuid --output $get_file
                d1=$(diff $update_file $get_file 2>&1)
                if [[ -n $d1 ]]; then
                        echo "Failed getting file"
                        kill_metax $p4 $p3 $p2 $p1
                        exit;
                fi
                kill_metax $p3 $p4 $p1

                clean_storage_content $test_dir/storage1
                $EXECUTE -f $tst_src_dir/config1.xml &
                p1=$!
                is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
                if [ "0" != "$is_peer_connected" ];then
                        echo "7080 does not connect to peers"
                        kill_metax $p2 $p1
                        exit;
                fi
                echo -n $p1 > $test_dir/processes_ids

                curl http://localhost:7081/db/get/?id=$uuid --output $get_file
                d2=$(diff $update_file $get_file 2>&1)
                if [[ -n $d2 ]]; then
                        echo "Failed getting from peer2"
                        kill_metax $p2 $p1
                        exit
                fi
                kill_metax $p2 $p1
                clean_storage_content $test_dir/storage2
                $EXECUTE -f $tst_src_dir/config3.xml &
                p3=$!
                wait_for_init 7101

                $EXECUTE -f $tst_src_dir/config2.xml &
                p2=$!
                is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
                if [ "0" != "$is_peer_connected" ];then
                        echo "7090 does not connect to peer"
                        kill_metax $p2 $p3
                        exit;
                fi

                $EXECUTE -f $tst_src_dir/config1.xml &
                p1=$!
                is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
                if [ "0" != "$is_peer_connected" ];then
                        echo "7080 does not connect to peers"
                        kill_metax $p3 $p2 $p1
                        exit;
                fi
                echo -n $p3 $p2 $p1 > $test_dir/processes_ids

                curl http://localhost:7081/db/get/?id=$uuid --output $get_file
                d3=$(diff $update_file $get_file 2>&1)
                if [[ -n $d3 ]]; then
                        echo "Failed getting from peer3"
                        kill_metax $p3 $p2 $p1
                        exit
                fi
                kill_metax $p3 $p2 $p1
                clean_storage_content $test_dir/storage3
                start_metaxes

                curl http://localhost:7081/db/get/?id=$uuid --output $get_file
                d4=$(diff $update_file $get_file 2>&1)
                if [[ -n $d4 ]]; then
                        echo "Failed getting from peer4"
                        kill_metax $p4 $p3 $p2 $p1
                        exit
                fi
                echo $PASS > $final_test_result
        fi
        kill_metax $p4 $p3 $p2 $p1
}

main $@
