#!/bin/bash

# Test case flow:
# - Runs one instance of metax.
# - Generates a random file.
# - Sends "path" save request for the generated file (should be saved as encrypted).
# - Generates a new random file.
# - Sends "path" update request for the new generated file
#   (should be updated as encrypted).
# - Sends "get" request for received uuid.
# - Checks the compliance of last generated and received files.

save_file=$test_dir/save_file
update_file=$test_dir/update_file
get_file=$test_dir/get_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        dd if=/dev/urandom of=$save_file bs=16000000 count=1
        uuid=$(curl --data "$save_file" http://localhost:7081/db/save/path)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        dd if=/dev/urandom of=$update_file bs=16000000 count=1
        res=$(curl -D $test_dir/h --data "$update_file" http://localhost:7081/db/save/path?id=$uuid)
        echo $res
        http_code=$(cat $test_dir/h | grep HTTP/1.1 | awk {'print $2'})
        if [[ 200 != $http_code ]]; then
                echo "HTTP code of UPDATE request response is $http_code"
                kill_metax $p
                exit;
        fi
        uuid1=$(echo $res | cut -d'"' -f 4)
        if [ "$uuid" != "$uuid1" ]; then
                echo "UPDATE request failed"
                kill_metax $p
                exit;
        fi
        curl http://localhost:7081/db/get/?id=$uuid1 --output $get_file
        d=$(diff $update_file $get_file 2>&1)
        if [[ -n $d ]]; then
                echo "Getting of updated data failed."
                kill_metax $p
                exit;
        fi
        echo $PASS > $final_test_result
        kill_metax $p
}

main $@
