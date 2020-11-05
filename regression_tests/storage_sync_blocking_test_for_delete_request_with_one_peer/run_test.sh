#!/bin/bash

# Test case flow:
# - Runs and connects two instances of Metax.
# - Sends "node" save request for some content to Metax2.
# - Sends get request for saved content to Metax2.
# - Checks the compliance of received/saved contents.
# - Waits about 10 second for backup.
# - Checks that both storages content are the same (except user json uuids).
# - Kills Metax1.
# - Sends delete request for saved content to Metax2.
# - Checks that storage2 contains only user json uuids.
# - Runs Metax1.
# - Waits about 10 seconds to finish sync process.
# - Checks that both storages contain only user json uuids.
# - Sends "node" save request for some content to Metax2.
# - Sends get request for saved content to Metax2.
# - Checks the compliance of received/saved contents.
# - Sends "node" save request for some content to Metax1.
# - Sends get request for saved content to Metax1.
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
        str=$(echo ${uj_uuids[@]} | sed 's/ /*\\|/g')

        diff_options="--exclude=cache --exclude=storage.json"
        # Save some content to metax2.
        save_content="Some content for test"
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done

        uuid=$(curl --data "$save_content" http://localhost:7091/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7091/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content = $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        # wait for backup
        sleep $TIME

        diff=$(diff $test_dir/storage2 $test_dir/storage1 $diff_options)
        if [ -n "$diff" ]; then
                echo "Failed to update data. diff = $diff"
                kill_metax $p1 $p2
                exit;
        fi

        # Metax1 goes offline
        kill_metax $p1

        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7091/db/delete?id=$uuid)
        if [ "$expected" != "$res" ]; then
                echo "Failed to delete uuid. Delete request response : $res"
                kill_metax $p2
                exit;
        fi

	storage_size2=$(find $test_dir/storage2 -name "????????-*" | grep -v "$str" | expr $(wc -l))
        if [ "0" != "$storage_size2" ]; then
                echo "Failed to delete data. storage_size2 = $storage_size2"
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

        # Sleep for some period to finish sync process.
        sleep $TIME

        # TODO improve the check.
	storage_size1=$(find $test_dir/storage1 -name "????????-*" | grep -v "$str" | expr $(wc -l))
	storage_size2=$(find $test_dir/storage2 -name "????????-*" | grep -v  "$str" | expr $(wc -l))
        if [ "0" != "$storage_size1" ] || [ "0" != "$storage_size2" ]; then
                echo "Failed to sync"
                kill_metax $p1 $p2
                exit;
        fi

        uuid=$(curl --data "$save_content" http://localhost:7091/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7091/db/get/?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed to save in metax2 after sync: $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$save_content" != "$get_content" ]; then
                echo "Failed to save in metax1 after sync: $get_content"
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
