#!/bin/bash

# Test case flow: 
# - Runs and connects two instances of metax with config1.xml and config2.xml
#   configurations (the peers have different device/user keys).
# - Sends "node" save request to first metax (config1.xml) for some content
#   (should be saved as non-encrypted).
# - Sends get request to second running metax (config2.xml) for uuid, which
#   has been saved in first metax.
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
        echo -n $p2 $p1 > $test_dir/processes_ids
        save_content="Some content to save"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl http://localhost:7081/db/get?id=$uuid)
        if [ "$save_content" == "$get_content" ]; then
            echo $PASS > $final_test_result
        fi	    
        kill_metax $p1 $p2
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
