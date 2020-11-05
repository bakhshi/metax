#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Generates random file with ~1.5GB size.
# - Sends "data" save request for generated file (default it is saved as encrypted).
# - Sends get request (the request should not be finished)
# - Kill metax
# - Check that metax was finished successfully.

save_file=$test_dir/save_file

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0

main()
{
        echo "Time at the test beginning is $(date +%T)"
        # Generate ~1.5GB random file
        dd if=/dev/urandom of=$save_file bs=16000000 count=100
        echo "Time file generated is $(date +%T)"

        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        res=$(curl --data "$save_file" "http://localhost:7081/db/save/path?enc=0")
        uuid=$(echo $res | cut -d'"' -f 4)
        if [ -z $res ] || [ null == $uuid ]; then
                echo "Failed to save file"
                rm $save_file
                kill_metax $p
                exit;
        fi
        echo "uuid = $uuid"

        echo "Started get saved file $(date +%T)"
        curl "http://localhost:7081/db/get?id=$uuid" > /dev/null &
        sleep 5
        kill $p
        wait $p
        if [ "$EXIT_CODE_OK" != "$?" ]; then
                echo "Metax crashed"
                echo $CRASH > $final_test_result
        else
                echo $PASS > $final_test_result
        fi
        rm $save_file

}

main $@
