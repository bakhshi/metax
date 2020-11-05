#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configuration with no any peer.
# - Sends multiple "save" requests for getting valid uuids and saves that uuids
#   in an array. 
# - Sends multiple "update" requests with valid existing uuids.
# - Checks the compliance of the "save"/"update" request's messages.


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        for ((i=1; i<=100; ++i))
        do
                save=$(curl --data "save content" http://localhost:7081/db/save/node)
                res=$(echo $save | cut -d'"' -f 2)
                if [[ "uuid" != "$res" ]]; then
                        echo "Failed save $i time"
                        echo $FAIL > $final_test_result
                        kill_metax $p
                        exit
                fi

                uuid=$(echo $save | cut -d'"' -f 4)
                uuids[i]="$uuid"
                echo $i "${uuids[i]}"
        done

        for ((i=1; i<=100; ++i))
        do
                update=$(curl -m 10 --data "updated content" "http://localhost:7081/db/save/node?id=${uuids[i]}")
                echo $i $update
                uuid=$(echo $update | cut -d'"' -f 4)
                if [ "${uuids[i]}" != "$uuid" ]; then
                        echo "Failed update $i time"
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
