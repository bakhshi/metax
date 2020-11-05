#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Sends "data" save request for randomly generated .mp4 file (default it is saved as encrypted).
# - Sends "get" request for received uuid in background.
# - Send kill signal to metax while get request is in process.
# - Checks that metax finishes successfully.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0
input=$test_dir/save.mp4


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        cho "Started save mp4 file $(date +%T)"

        #Generated "mp4" ~190MB file
        dd if=/dev/urandom of=$input bs=1000000 count=200
        uuid=$(curl --form "fileupload=@$input" --header "Metax-Content-Type: video/mp4" "http://localhost:7081/db/save/data")
        uuid=$(echo $uuid | cut -d'"' -f 4)
        echo "Saved uuid = $uuid"
        echo "Started get saved mp4 file $(date +%T)"
        curl http://localhost:7081/db/get?id=$uuid >/dev/null & 
        sleep 3 
        kill $p
        wait $p
        if [ "$EXIT_CODE_OK" != "$?" ]; then
                echo "Metax crashed"
                echo $CRASH > $final_test_result
                exit;
        else
                echo $PASS > $final_test_result
        fi
        rm $input
}

main $@
