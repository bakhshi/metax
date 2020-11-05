#!/bin/bash

# Test case flow: 
# - Generates random big file.
# - Runs one instance of metax.
# - Sends "path" save request for generated file (unencrypted)
# - Sends "get" request for received uuid.
# - While the get request is in process sends "node" save request for a small
#   text
# - Checks the saved small text is completed within 1 seconds in parallel with
#   big file get

if [ "yes" == "$use_valgrind" ]; then
        echo $SKIP > $final_test_result
        exit;
fi

save_file=$test_dir/save_file
get_file=$test_dir/get_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        # Generate ~1.5GB random file
        dd if=/dev/urandom of=$save_file bs=16000000 count=100

        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uuid=$(curl --data "$save_file" http://localhost:7081/db/save/path?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        kill_metax $p

        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        curl http://localhost:7081/db/get?id=$uuid > /dev/null &
        sleep 4

        save_content="save content"
        uuid=$(curl -m 1 --data "$save_content" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)

        if [[ $uuid =~ [0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12} ]]; then
                echo "Save succeeded during big file get"
        else
                echo "Failed to save small file during big file get in 1 seconds"
                kill_metax $p
                rm $save_file
                exit
        fi
        echo $PASS > $final_test_result;
        rm $save_file
        kill_metax $p
}

main $@
