#!/bin/bash

# Test case flow: 
# - Runs and connects four instances of metax (peer1->peer2->peer3->peer4).
# - Sends "data" save request for random generated file (default it is saved as encrypted)
#   to the metax, running with config1.xml.
# - Sends get request for received uuid to the running metaxes.
# - Checks the compliance of received/saved contents.

save_file=$test_dir/save_file
get_file=$test_dir/get_file


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
        diff_options="--exclude=cache --exclude=storage.json"
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done

        #dd-"data dublicator". Dublicates random data in save_file
        #according to the given options.
        dd if=/dev/urandom of=$save_file bs=1000000 count=1
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d1=$(diff $save_file $get_file 2>&1)

        c1=$(diff -r $test_dir/storage1/ $test_dir/storage2/ $diff_options)
        c2=$(diff -r $test_dir/storage1/ $test_dir/storage3/ $diff_options)
        c3=$(diff -r $test_dir/storage1/ $test_dir/storage4/ $diff_options)
        if [[ -n $c1 || -n $c2 || -n $c3 ]] ; then
                echo "Storage directories are not the same c1=$c1 c2=$c2 c3=$c3"
                kill_metax $p4 $p3 $p2 $p1
                exit;
        fi

        kill_metax $p4 $p3 $p2 $p1
        clean_storage_content $test_dir/storage1
        start_metaxes

        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d2=$(diff $save_file $get_file 2>&1)

        kill_metax $p4 $p3 $p2 $p1
        clean_storage_content $test_dir/storage2
        start_metaxes

        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d3=$(diff $save_file $get_file 2>&1)

        kill_metax $p4 $p3 $p2 $p1
        clean_storage_content $test_dir/storage3
        start_metaxes

        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d4=$(diff $save_file $get_file 2>&1)

        if [[ -z $d1 && -z $d2 && -z $d3 && -z $d4 ]]; then
                echo $PASS > $final_test_result
        fi
        kill_metax $p4 $p3 $p2 $p1
}

main $@
