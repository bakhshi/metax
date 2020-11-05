#!/bin/bash

# Test case flow: 
# - TODO

save_file=$test_dir/save_file
get_file=$test_dir/get_file

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p1=$!
        wait_for_init 7081
        echo -n $p1 > $test_dir/processes_ids

        dd if=/dev/urandom of=$save_file bs=16000000 count=1
        uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        copy_res=$(curl http://localhost:7081/db/copy?id=$uuid)
        echo "Copy request response - $copy_res"
        copy_uuid=$(echo $copy_res | jq -r '.copy_uuid')
        echo "New uuid of copy request - $copy_uuid"
        curl http://localhost:7081/db/get/?id=$copy_uuid --output $get_file
        d=$(diff $save_file $get_file 2>&1)
        if [[ -z $d ]]; then
                echo $PASS > $final_test_result;
        fi
        kill_metax $p1
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
