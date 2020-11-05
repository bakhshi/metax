#!/bin/bash

# Test case flow: 
# - Runs and connects three instances of metax (peer1->peer2, peer1->peer3).
# - Sends "path" save request for some content (should be saved as non_encrypted)
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
        $EXECUTE -f $tst_src_dir/config3.xml &
        p3=$!
        is_peer_connected=$(peers_connect 7101 $tst_src_dir/config3.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7101 does not connect to peer"
                kill_metax $p3
                exit;
        fi
        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peer"
                kill_metax $p3 $p2
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
        get_user_json_uuids 3
        #dd-"data dublicator". Dublicates random data in save_file
        #according to the given options.
        dd if=/dev/urandom of=$save_file bs=1000000 count=1
        uuid=$(curl --data "$save_file" http://localhost:7081/db/save/path?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d1=$(diff $save_file $get_file 2>&1)

        kill_metax $p1 $p2 $p3
        clean_storage_content $test_dir/storage1
        start_metaxes

        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d2=$(diff $save_file $get_file 2>&1)

        kill_metax $p1 $p2 $p3
        rm -rf $test_dir/storage1/cache
        clean_storage_content $test_dir/storage2
        start_metaxes

        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        d3=$(diff $save_file $get_file 2>&1)
        if [[ -z $d1 && -z $d2 && -z $d3 ]] ; then
                echo $PASS > $final_test_result
        fi
        kill_metax $p1 $p2 $p3
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
