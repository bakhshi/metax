#!/bin/bash

# Test case flow: 
# - runs two Metax instances
# - saves data to 1st peer
#       - specifies Metax-Content-Type header during save request
# - shares the data with the 2nd peer
# - gets the saved from the 2nd peer
#       - checks correctnerss of the data
#       - checks that Content-Type is set properly in get request headers
#   

headers_file=$test_dir/headers_file


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
        save_content="A content"
        uuid=$(curl --data "$save_content" -H "Metax-Content-Type: image/svg+xml" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        res=$(curl -m 10 -F "username=friend2" http://localhost:7081/db/share?id=$uuid)
        echo $res
        share_uuid=$(echo $res | jq -r '.share_uuid')
        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')
        received=$(curl -m 10 -F "key=$key" -F "iv=$iv" http://localhost:7091/db/accept_share?id=$uuid)
        expected='{"share":"accepted"}'
        if [ "$expected" != "$received" ]; then
                echo "Share accept failed"
                kill_metax $p1 $p2;
                exit;
        fi
        share_info=$(curl -D $headers_file http://localhost:7091/db/get?id=$share_uuid)
        if [ "$share_info" != "$save_content" ]; then
                echo "received incorrect shared data"
                kill_metax $p2 $p1
                exit;
        fi
        grep "Content-Type: image/svg+xml" $headers_file
        if [ $? -ne 0 ]; then
                echo "Content type error"
                kill_metax $p2 $p1
                exit
        fi

        echo $PASS > $final_test_result
        kill_metax $p1 $p2
}

main $@
