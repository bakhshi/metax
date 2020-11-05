#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Generates random file with ~1.5GB size.
# - Sends "data" save request for generated file with enc=0
# - Kills metax when save request is in prcoess
# - Checks that metax was finished successfully.

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

        curl --form "fileupload=@$save_file" "http://localhost:7081/db/save/data?enc=0" &
        sleep 1
        kill $p
        echo "before wait $(date +%T)"
        wait $p
        if [ "$EXIT_CODE_OK" != "$?" ]; then
                echo "Metax crashed"
                echo $CRASH > $final_test_result
        else
                echo "Time at the end of test is $(date +%T)"
                echo $PASS > $final_test_result
        fi
        rm $save_file
}

main $@
