#!/bin/bash

# Test case flow:
# - Runs one instance of metax.
# - Generates random file with ~1GB size.
# - Sends "data" save request for generated file (default it is saved as encrypted).
# - Sends "get" request for received uuid.
# - Checks the compliance of received/saved files.
# - Deletes saved in metax, generated and received files.


save_file=$test_dir/save_file
get_file=$test_dir/get_file

if [ "yes" == "$use_valgrind" ]; then
export TIME=20
export EXECUTE="$TIMEOUT -s SIGKILL 1000  valgrind --leak-check=full --log-file=$test_dir/valgrind_result.txt $shell_bin"
else
export TIME=10
export EXECUTE="$TIMEOUT -s SIGKILL 600 $shell_bin"
fi

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


check_and_clean_storage()
{
        # Checks and removes uuids from local storage
        storage_content=$(ls $test_dir/storage/????????-* | grep -v "$uj_uuid" 2>/dev/null)
        if [ -n "$storage_content" ]; then
                echo "Storage content is : $storage_content"
                rm $test_dir/storage/????????-*
                echo $FAIL > $final_test_result;
        fi
}

main()
{
        # Generate ~1GB random file
        dd if=/dev/urandom of=$save_file bs=16000000 count=65

        echo "Time at the test beginning is   $(date +%T)"
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uj_uuid=$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')

        res=$(curl --data "$save_file" http://localhost:7081/db/save/path)
        echo "Response after save is $res"
        uuid=$(echo $res | jq '.uuid')
        if [ -z $res ] || [ null == $uuid ]; then
                echo "Failed to save big data"
                rm $save_file
                kill_metax $p
                exit;
        fi

        uuid=$(echo $res | cut -d'"' -f 4)
        curl http://localhost:7081/db/get?id=$uuid --output $get_file
        d=$(diff $save_file $get_file 2>&1)
        if [[ -z $d ]]; then
                echo "Got saved big file successfully"
                echo $PASS > $final_test_result;
        else
                res=$(cat $get_file)
                echo "Failed to get big file. Response after get is $res"
        fi

        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7081/db/delete?id=$uuid)
        if [ "$res" != "$expected" ]; then
                echo "Failed to delete big file. Response is $res"
                echo $FAIL > $final_test_result;
        fi

        rm $save_file
        rm $get_file
        check_and_clean_storage
        kill_metax $p
        echo "Time at the end of test is  $(date +%T)"
}

main $@
