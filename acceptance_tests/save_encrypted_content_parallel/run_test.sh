#!/bin/bash

# Test case flow:
# - Runs n instance of metax.
# - Sends multiple "node" encrypted save requests to Metax instance in parallel
# - Waits for the save requests to finish
# - Checks the result of each save request by getting the returned uuids and
#   comparing with the original saved content

save_content="Some content for test"

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        save_content="content"
        n=30
        i=1
        while [ $i -le $n ]
        do
                (curl --data $save_content "http://localhost:7081/db/save/node?enc=1" 2> /dev/null) > $test_dir/save_res$i 2>&1 &
                pids[$i]=$!
                i=$((i+1))
        done
        f=0
        for pid in ${pids[*]}; do
                wait $pid || f=1
                if [ $f != 0 ]; then
                        echo "Save failed"
                        kill_metax $p
                        exit
                fi
        done
        i=1
        while [ $i -le $n ]
        do
                uuid=$(cat $test_dir/save_res$i | cut -d'"' -f 4)
                get=$(curl http://localhost:7081/db/get/?id=$uuid 2> /dev/null)
                echo $get
                if [ "$save_content" != "$get" ]
                then
                        echo "Save and get content mismatch"
                        kill_metax $p
                        exit
                fi
                i=$((i+1))
        done
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@
