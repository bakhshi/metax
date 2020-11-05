#!/bin/bash

#Test failes because of bug #9391

# Test case flow:
# - Runs and connects two instances of metax with each other (metax1 -> metax2).
# - Send save stream request to metax1.
# - Send get request to metax2.
# - Kill two metaxes
# - Check that metaxes were finished successfully.

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
        echo -n $p2 $p1 > $test_dir/processes_ids

        #Generated ~300KB file
        dd if=/dev/urandom of=$save_file bs=100000 count=3
        echo "du -sh save_file `du -sh $save_file `"

        g++ -std=c++11 $tst_src_dir/livestream_maker.cpp -lPocoFoundation  -lPocoNet -o exe
        ./exe 7095 $save_file &
        sleep 1
        exe_p=$!
        curl "http://localhost:7081/db/save/stream?url=127.0.0.1:7095&id=283abccd-8c1e-47c1-8795-839abcfede5c" &
        sleep 3
        curl "http://localhost:7091/db/get?id=283abccd-8c1e-47c1-8795-839abcfede5c" > /dev/null &
        sleep 5

        kill $exe_p
        wait $exe_p
        # kill metax-es and check exit code
        kill $p1
        wait $p1
        c1="$?"
        kill $p2
        wait $p2
        c2="$?"
        if [[ "$EXIT_CODE_OK" != "$c1" || "$EXIT_CODE_OK" != "$c2" ]]; then
                echo "Metax crashed: c1=$c1, c2=$c2"
                echo $CRASH > $final_test_result
        else
                grep -q "Assertion" $test_dir/test_output
                if [[ "$EXIT_CODE_OK" == "$?" ]]; then
                        echo $FAIL > $final_test_result
                else
                        echo $PASS > $final_test_result
                fi
        fi
        rm $save_file
}

main $@
