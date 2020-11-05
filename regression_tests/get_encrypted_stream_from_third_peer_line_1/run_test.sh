#!/bin/bash

#Test failes because of bug #9391

# Test case flow:
# - Runs and connects three instances of metax (metax1 -> metax2 -> metax3).
# - Send save encrypted stream request to metax1 (saved in metax2).
# - Send share request with netax3.
# - Send get request for metax3 (getting stream from metax2).
# - Check that get stream request is succeeded or not.

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

save_file=$test_dir/save_file



main()
{
        if [[ "Darwin" == "$target" ]]; then
                echo $SKIP > $final_test_result
                exit
        fi
        start_metaxes
        #Generated ~500KB file
        dd if=/dev/urandom of=$save_file bs=100000 count=5
        echo "du -sh save_file `du -sh $save_file `"
        uuid="283abccd-8c1e-47c1-8795-839abcfede5c"

        get_public_key=$(curl http://localhost:7101/config/get_user_public_key)
        public_key=$(echo $get_public_key | jq -r '.user_public_key')
        echo "public_key=$public_key"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation  -lPocoNet -o exe
        ./exe 7095 $save_file &
        exe_pid=$!
        sleep 1

        curl "http://localhost:7081/db/save/stream?url=127.0.0.1:7095&id=$uuid" &
        sleep 1

        res=$(curl -m 10 -F "key=$public_key" http://localhost:7081/db/share?id=$uuid)
        share_uuid=$(echo $res | jq -r '.share_uuid')
        key=$(echo $res | jq -r '.key')
        iv=$(echo $res | jq -r '.iv')

        received=$(curl -m 10 -F "key=$key" -F "iv=$iv" http://localhost:7101/db/accept_share?id=$uuid)
        expected='{"share":"accepted"}'
        if [ "$expected" != "$received" ]; then
                echo "Share accept failed received=$received"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2 $p3;
                rm $save_file
                exit;
        fi

        curl "http://localhost:7101/db/get?id=$share_uuid" --output $test_dir/get_file

        tail --byte 35 $save_file > $test_dir/save_last_line
        tail --byte 35 $test_dir/get_file > $test_dir/get_last_line
        diff=$(diff $test_dir/save_last_line $test_dir/get_last_line)
        if [ -n "$diff" ];then
                echo "Failed to get encrypted stream from peer3"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2 $p3
                exit;
        fi
        echo $PASS > $final_test_result
        kill $exe_pid
        wait $exe_pid
        kill_metax $p1 $p2 $p3
}

main $@

