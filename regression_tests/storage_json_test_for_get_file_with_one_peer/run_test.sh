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




main()
{
        start_metaxes
        get_user_json_uuids 2
        diff_options=" --exclude=cache --exclude=storage.json"
        for i in ${!uj_uuids[@]}; do
                diff_options+=" --exclude=${uj_uuids[$i]}*"
        done
        save_content="Some content for test"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?local=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sleep $TIME
        get_content=$(curl http://localhost:7081/db/get/?id=$uuid)
        kill_metax $p1 $p2
        if [ "$save_content" != "$get_content" ]; then
                echo "GET request failed. get_content=$get_content"
                exit;
        fi

        # Cache the difference of storage2 and storage1/cache files.
        diff=$(diff $test_dir/storage1/cache $test_dir/storage2/ $diff_options)
        if [ -n "$diff" ]; then
                echo "Storage2 and storage1/cache are not the same.$diff"
                exit;
        fi

        # cache size limit specified in GB. Getting cache  size limit from config.xml file
        cache_size_limit=$(grep "<cache_size_limit>" $tst_src_dir/config1.xml | sed 's/ //g;s/<cache_size_limit>//g;s/<\/cache_size_limit>//g' | tr . ,)
        #cache_size_limit=$(($(echo $cache_size_limit | tr . ,)<<30))
        cache_size_limit=$(($cache_size_limit*1024*1024*1024))
        echo "cache_size_limit = $cache_size_limit"

        cache_dir_size=$(wc -c $test_dir/storage1/cache/????????-* | tail -1 | awk {'print $1'})
        cache_size=$(cat $test_dir/storage1/storage.json | jq '.cache_size')
        if [[ $cache_size == 0 || $cache_size != $cache_dir_size || $cache_dir_size -gt $cache_size_limit ]]; then
                echo "Invalid properties in storage.json or storage size greater than storage size limit"
                echo "cache_size = $cache_size, cache_dir_size = $cache_dir_size, cache_size_limit = $cache_size_limit"
                exit
        fi
        echo $PASS > $final_test_result
}

main $@
