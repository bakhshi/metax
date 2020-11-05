#!/bin/bash

# Test case flow:
# - Runs and connects two instances of metax with each other (peer1 -> peer2).
# - Send save stream request to metax1 (enc = 0).
# - Send record an existing livestream in metax2 storage.
# - Sends stop recording livestream
# - Check that stop recording request was succeeded.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



save_file=$test_dir/save_file
binary_file=$test_dir/binary_file



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
        echo -n $p2 $p1 > $test_dir/processes_ids

        #Generated ~300MB file
        dd if=/dev/urandom of=$binary_file bs=100000 count=3
        echo "du -sh save_file `du -sh $save_file `"
        xxd -b $binary_file > $save_file
        uuid="283abccd-8c1e-47c1-8795-839abcfede5c"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation -lPocoNet -o exe
        ./exe 7095 $save_file &
        exe_pid=$!
        sleep 1
        curl "http://localhost:7081/db/save/stream?url=127.0.0.1:7095&id=$uuid&enc=0"&
        sleep 1

        ruuid=$(curl "http://localhost:7091/db/recording/start?livestream=$uuid")
        ruuid=$(echo $ruuid | cut -d'"' -f 4)
        echo "ruuid=$ruuid"
        sleep $TIME

        response=$(curl "http://localhost:7091/db/recording/stop?livestream=$uuid&recording=$ruuid")
        echo "recording stop response $response"
        expected="{\"status\":\"success\"}"

        if [ "$response" != "$expected" ];then
                echo "Failed to stop livestream recording."
                kill $exe_pid
                wait $exe_pid
                kill_metax $p1 $p2
                exit;
        fi

        sleep 2
        curl "http://localhost:7091/db/get?id=$ruuid" --output $test_dir/get_file
        cat $test_dir/get_file
        rec_url=$(grep "/db/get?id=" $test_dir/get_file)
        echo "getting recorded livestream by $rec_url url"

        curl "http://localhost:7091$rec_url" --output $test_dir/get_rec_file

        tail --byte 35 $test_dir/get_rec_file > $test_dir/get_last_line
        grep $save_file -q -f $test_dir/get_last_line
        if [ "0" != "$?" ]; then
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
