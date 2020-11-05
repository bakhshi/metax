#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configuration with several peers.
# - Sends multiple "save" requests consecutively to the running metax (default
#   it is saved as encrypted).
# - Checks whether the "save" request succeed.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        is_http_server_started=$(wait_for_init 7081)
        echo -n $p > $test_dir/processes_ids
        if [ "0" != "$is_http_server_started" ];then
                echo "htpp server isn't started - 7081"
                kill_metax $p
                exit
        fi
        sleep 5
        op=$(curl http://localhost:7081/config/get_online_peers 2> /dev/null)
        echo "get_online_peers request response is $op"

        l=$(echo $op | grep -o "address" | expr $(wc -l))
        echo "online peers count is $l"
        if [[ 6 -ne $l ]]; then
                echo "online peers count should be 6"
                kill_metax $p
                exit
        fi
        for ((i=1; i<=100; ++i))
        do
                save=$(curl --data "save content" http://localhost:7081/db/save/node)
                res=$(echo $save | cut -d'"' -f 2)
                echo $i $save
                if [[ "uuid" != "$res" ]]; then
                        echo "Save failed in $i request"
                        echo $FAIL > $final_test_result
                        kill_metax $p
                        exit
                fi
        done

        echo $PASS > $final_test_result
	kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
