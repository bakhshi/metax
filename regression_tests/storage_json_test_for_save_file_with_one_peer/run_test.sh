#!/bin/bash

# Test caseflow:
# - Runs and connects two instances of metax (metax1 (size_limit=0.5) -> metax2 (size_limit=0.02) ).
# - Generated 20 MB file
# - Sends "data" save request for generated file (default it is saved as encrypted).
# - Waits about 10 second for backup.
# - Checks that data is saved in metax1.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
save_file=$test_dir/save_file
get_file=$test_dir/get_file


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
        #Generated 30MB random file
        dd if=/dev/urandom of=$save_file bs=10000000 count=3
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "uuid = $uuid"
        sleep $TIME

        curl http://localhost:7081/db/get/?id=$uuid --output $get_file
        kill_metax $p1 $p2
        diff=$(diff $save_file $get_file 2>&1)
        if [ -n "$diff" ]; then
                echo "GET request failed."
                exit;
        fi


        # storage size limit specified in GB. Getting size-limit from config.xml file
        size_limit1=$(grep "<size_limit>" $tst_src_dir/config1.xml | sed 's/ //g;s/<size_limit>//g;s/<\/size_limit>//g' | tr . ,)
        #size_limit1=$(($(echo $size_limit1 | tr . ,)<<30))
        size_limit1=$(($size_limit1*1024*1024*1024))
        echo "---- size_limit1 = $size_limit1 byte ----"

        storage_dir_size1=$(wc -c $(find $test_dir/storage1/????????-* -not -name ${uj_uuids[0]}*) | tail -1 | awk {'print $1'})
        echo "storage_dir_size1 = $storage_dir_size1"
        #storage_size1=$(cat $test_dir/storage1/storage.json | jq '.storage_size' )
        cache_size1=$(cat $test_dir/storage1/storage.json | jq '.cache_size' )

        if [[ $cache_size1 != 0 || $storage_dir_size1 -gt $size_limit1 ]]; then
                echo "Invalid properties in storage1/storage.json or storage size greater than storage size limit"
                echo "cache_size1 = $cache_size1, storage_dir_size1 = $storage_dir_size1"
                exit;
        fi

        # storage size limit specified in GB. Getting size-limit from config.xml file
        size_limit2=$(grep "<size_limit>" $tst_src_dir/config2.xml | sed 's/ //g;s/<size_limit>//g;s/<\/size_limit>//g' | tr . ,)
        size_limit2=$(($size_limit2*1024*1024*1024))
        echo "---- size_limit2 = $size_limit2 byte ----"

        storage_dir_size2=$(wc -c $(find $test_dir/storage2/????????-* -not -name ${uj_uuids[1]}*) | tail -1 | awk {'print $1'})
        echo "storage_dir_size2 = $storage_dir_size2"
        #storage_size2=$(cat $test_dir/storage2/storage.json | jq '.storage_size' )
        cache_size2=$(cat $test_dir/storage2/storage.json | jq '.cache_size' )

        if [[ $cache_size2 != 0 || $storage_dir_size2 -gt $size_limit2 ]]; then
                echo "Invalid properties in storage1/storage.json or storage size greater than storage size limit"
                echo "cache_size2 = $cache_size2, storage_dir_size2 = $storage_dir_size2"
                exit;
        fi
        echo $PASS > $final_test_result
}

main $@
