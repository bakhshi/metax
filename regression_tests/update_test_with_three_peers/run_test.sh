#!/bin/bash

# Test case flow: 
# - Runs and connects four instances of metax (peer2->peer1, peer3->peer1, peer4->peer3).
# - Sends "data" save request for random generated file (default it is saved as encrypted)
#   to the metax, running with config1.xml.
# - Twice sends "update" request for received uuid to the all running metaxes.
# - Checks the compliance of "update"/"save" request's responses.

save_file=$test_dir/save_file
update_file=$test_dir/update_file
get_file=$test_dir/get_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0




start_metaxes()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect"
                kill_metax $p1
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p1 $p2
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7100 does not connect to peers"
                kill_metax $p1 $p2 $p3
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config4.xml &
        p4=$!
        is_peer_connected=$(peers_connect 7111 $tst_src_dir/config4.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7110 does not connect to peers"
                kill_metax $p1 $p2
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

        #dd-"data dublicator". Dublicates random data in save_file
        #according to the given options. 
        dd if=/dev/urandom of=$save_file bs=1000 count=1
        save=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data)
        echo $save
        uuid=$(echo $save | cut -d'"' -f 4)
        for ((i = 1; i <= 2; ++i))
        do
                dd if=/dev/urandom of=$update_file bs=1000 count=1
                update=$(curl -m 10 --form "fileupload=@$update_file" http://localhost:7081/db/save/data?id=$uuid)
                if [ "$save" != "$update" ]; then
                        echo "Failed updating from $port $i time"
                        echo $FAIL > $final_test_result
                        kill_metax $p4 $p3 $p2 $p1
                        exit
                fi
		curl -m 10 http://localhost:7081/db/get/?id=$uuid --output $get_file
		d=$(diff $update_file $get_file 2>&1)
		if [[ -n $d ]]; then
                        echo "Fail to get updated data $i time"
                        echo $FAIL > $final_test_result
                        kill_metax $p4 $p3 $p2 $p1
                        exit
		fi
        done

        sleep $TIME
        d1=$(diff -r $test_dir/storage1/ $test_dir/storage2/ $diff_options)
        d2=$(diff -r $test_dir/storage1/ $test_dir/storage3/ $diff_options)
        d3=$(diff -r $test_dir/storage1/ $test_dir/storage4/ $diff_options)
        if [[ -n $d1 || -n $d2 || -n $d3 ]] ; then
                echo "Storage directories are not the same"
                echo $FAIL > $final_test_result
                kill_metax $p4 $p3 $p2 $p1
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p4 $p3 $p2 $p1
}

main $@
