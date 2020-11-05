#!/bin/bash

# Test case flow: 
# - Runs one instance of metax - metax1.
# - Sends "node" save request for some content (it is saved non encrypted).
# - Gets data from metax1 to ensure that data was saved correctly.
# - Runs another instance of metax - metax2.
# - Gets data from metax2 with no cache to ensure that data was got correctly.
# - Kill metax1
# - Gets data from metax2 to ensure that data is not accessible due to the
#   previous get with no cache attribute.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0






main()
{
        $EXECUTE -f $tst_src_dir/config1.xml &
        p1=$!
        is_peer_connected=$(peers_connect 7081 $tst_src_dir/config1.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7080 does not connect to peers"
                kill_metax $p1
                exit;
        fi
        echo -n $p1 > $test_dir/processes_ids

        save_content="Some content for test"
        uuid=$(curl --data "$save_content" "http://localhost:7081/db/save/node?enc=0")
        uuid=$(echo $uuid | cut -d'"' -f 4)
       
        get_content=$(curl "http://localhost:7081/db/get/?id=$uuid")
        if [ "$save_content" != "$get_content" ]; then
                echo "Getting saved data by metax1 failed"
                kill_metax $p1
                exit;
        fi

        $EXECUTE -f $tst_src_dir/config2.xml &
        p2=$!
        is_peer_connected=$(peers_connect 7091 $tst_src_dir/config2.xml)
        if [ "0" != "$is_peer_connected" ];then
                echo "7090 does not connect to peers"
                kill_metax $p2
                exit;
        fi
        echo -n $p2 >> $test_dir/processes_ids
        # get data before metax1 kill 
        get_content=$(curl "http://localhost:7091/db/get/?id=$uuid&cache=0")
        if [ "$save_content" != "$get_content" ]; then
                echo "Getting saved data by metax2 failed"
                kill_metax $p1 $p2
                exit;
        fi

        # check cache when metax1 is killed
        kill_metax $p1
        
        get_content=$(curl "http://localhost:7091/db/get/?id=$uuid&cache=0")
        if [ "$save_content" == "$get_content" ]; then
                echo "Data was saved in cache"
                kill_metax $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p2
}

main $@

