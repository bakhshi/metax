#!/bin/bash

# Test caseflow:
# - Runs and connects two instances of metax.
# - Sends "node" save request for some content (default it is saved as encrypted).
# - Waits about 10 second for backup.
# - Gets data from metax1 to ensure that data was saves correctly.
# - Remove metax1 storage and gets data again.
# - Checks that data is saved in cache.
# - Removes data from metax2's storage and gets again.

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

        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p2 $p1 > $test_dir/processes_ids
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
        get_user_json_uuids 2
        diff_options="--exclude=storage.json"
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done
        save_content="Some content for test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content=$get_content"
                kill_metax $p1 $p2
                exit;
        fi

        # Remove storage1 content and get data from peer.
        kill_metax $p1 $p2
        clean_storage_content $test_dir/storage1
        start_metaxes

        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed after removing storage1. get_content=$get_content"
                kill_metax $p1 $p2
                exit;
        fi

        # Check the difference of storage2 and storage1/cache files.
        d=$(diff $test_dir/storage1/cache $test_dir/storage2/ $diff_options)
        expected="Only in $test_dir/storage2/: cache"
        if [ "$expected" != "$d" ]; then
                echo "Storage2 and storage1/cache are not the same.$d"
                kill_metax $p1 $p2
                exit;
        fi

        # Remove storage2 content and get data from cache
        kill_metax $p1 $p2
        clean_storage_content $test_dir/storage2
        start_metaxes
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed after cleaning storage2."
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
