#!/bin/bash

# Test case flow: 
# - Runs and connects two instances of metax with each other (metax1 -> metax2).
# - Send save stream request to metax1. 
# - Send recording start to metax2.
# - Send get request to metax2 and check get request response.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

save_file=$test_dir/save_file

main()
{
        if [[ "Darwin" == "$target" ]]; then
                echo $SKIP > $final_test_result
                exit
        fi
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
                kill_metax $p2 $p1
                exit;
        fi
        echo -n $p1 $p2 > $test_dir/processes_ids
        #Generated ~500KB file
        dd if=/dev/urandom of=$save_file bs=100000 count=5
        echo "du -sh save_file `du -sh $save_file `"
        uuid="283abccd-8c1e-47c1-8795-839abcfede5c"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation -lPocoNet -o exe
        ./exe 7095 $save_file &
        exe_pid=$!
        sleep 1
        curl "http://localhost:7081/db/save/stream?url=127.0.0.1:7095&id=$uuid&enc=0"&
        cpid=$!
        sleep 1

        ruuid=$(curl "http://localhost:7091/db/recording/start?livestream=$uuid")
        ruuid=$(echo $ruuid | cut -d'"' -f 4)
        echo ruuid = $ruuid

        wait $cpid
        # Sleep for get recorded stream.
        sleep 1
        response=$(curl "http://localhost:7091/db/recording/stop?livestream=$uuid&recording=$ruuid")
        echo "recording stop response $response"
        sleep 2

        curl "http://localhost:7091/db/get?id=$ruuid" --output $test_dir/get_file
        cat $test_dir/get_file
        rec_url=$(grep "/db/get?id=" $test_dir/get_file)
        echo "getting recorded livestream by $rec_url url"

        curl "http://localhost:7091$rec_url" --output $test_dir/get_rec_file

        tail --byte 35 $save_file > $test_dir/save_last_line
        tail --byte 35 $test_dir/get_rec_file > $test_dir/get_last_line
        diff=$(diff $test_dir/save_last_line $test_dir/get_last_line)
        if [ -n "$diff" ];then
                echo "The recording does not match"
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2
                exit;
        fi

        echo $PASS > $final_test_result
        kill $exe_pid
        wait $exe_pid
        kill_metax $p1 $p2
}

main $@
