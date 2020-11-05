#!/bin/bash

# Test case flow: 
# - TODO


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p1=$!
        wait_for_init 7081
        echo -n $p1 > $test_dir/processes_ids

        save_content="Some content to save"
        uuid=$(curl --data "$save_content" http://localhost:7081/db/save/node?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        copy_res=$(curl http://localhost:7081/db/copy?id=$uuid)
        echo "Copy request response - $copy_res"
        copy_uuid=$(echo $copy_res | jq -r '.copy_uuid')
        echo "New uuid of copy request - $copy_uuid"
        get_content=$(curl http://localhost:7081/db/get?id=$copy_uuid)
        if [ "$save_content" == "$get_content" ]; then
            echo $PASS > $final_test_result
        fi
        kill_metax $p1
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
