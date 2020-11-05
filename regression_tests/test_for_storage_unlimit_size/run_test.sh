#!/bin/bash

# Test case flow:
# - Runs one instance of metax (storage_size_limit = -1).
# - Generates 100MB random file.
# - Sends "data" save request for generated file with enc=0 parameter three times.
# - Checks that save request succedded.

save_file=$test_dir/save_file

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids
        uj_uuid=("$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')")
        echo user json $uj_uuid

        #Generated 100MB random file
        dd if=/dev/urandom of=$save_file bs=10000000 count=10
        for i in {1..5};do
                res=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data?enc=0)
                echo res=$res
                uuid=$(echo $res | cut -d'"' -f 4)
                echo $uuid >> $test_dir/uuids
        done

        while read l
        do
                curl http://localhost:7081/db/get/?id=$l --output $test_dir/get_file
                diff=$(diff $save_file $test_dir/get_file 2>&1)
                if [ -n "$diff" ]; then
                        echo "GET file from $l failed."
                        kill_metax $p
                        exit;
                fi
        done < $test_dir/uuids

        rm $save_file $test_dir/get_file
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@
