#!/bin/bash

# Test case flow:
# - Runs one instance of metax which connects to remote peer(172.17.7.65:7080)
# - Generates random file with ~1Gb size.
# - Sends "data" save request for generated file with local=0 parameter (default it is saved as encrypted).
# - Restart killed metax.
# - Sends get request for received uuid to running metax.
# - Checks the compliance of received/saved files.
# - Deletes saved in metax, generated and received files.
# - Checks the uuid in remote peer is deleted or not (if not removes by using rm command)

#BEGIN OF REMOVE
#remove when test will be used with test_case.mk
export project_root=$PWD
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
export test_dir=$project_root/regression_tests_results/save_encrypted_3GB_file_with_one_remote_peer_with_no_local
tst_src_dir=$project_root/regression_tests/save_encrypted_3GB_file_with_one_remote_peer_with_no_local
shell_bin=$project_root/bin/metax_web_api
test_output=$test_dir/test_output
final_test_result=$test_dir/result.txt
remote_storage="~/.leviathan/storage_for_test_environment"
CRASH="CRASH"
FAIL="FAIL"
PASS="PASS"
CONNECT_TO_PEER="sshpass -p bananapi ssh -o StrictHostKeyChecking=no pi@172.17.7.65"

OS=$(uname -s)
if [[ "Darwin" == "$OS" ]]; then
        export TIMEOUT=gtimeout
else
        export TIMEOUT=timeout
fi

rm -rf $test_dir
mkdir -p $test_dir

echo $FAIL > $final_test_result

#END OF REMOVE
#ALSO REMOVE ALL REDIRECTIONS TO test_output


save_file=$test_dir/save_file
get_file=$test_dir/get_file

#UNCOMMENT when test will be used with test_case.mk
#if [ "yes" == "$use_valgrind" ]; then
#export TIME=20
#export EXECUTE="$TIMEOUT -s SIGKILL 1700  valgrind --leak-check=full --log-file=$test_dir/valgrind_result.txt $shell_bin"
#else
export TIME=10
export EXECUTE="$TIMEOUT -s SIGKILL 5000 $shell_bin"
#fi

source $project_root/test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        # Generate ~3GB random file
        dd if=/dev/urandom of=$save_file bs=16000000 count=200

        echo "Time at the test beginning is  $(date +%T)" > $test_output
        # Connects to  172.17.7.65:7080 peer
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uj_uuid=$(decrypt_user_json_info $test_dir/keys | jq '.uuid' | sed -e 's/^"//' -e 's/"$//')

        res=$(curl --data "$save_file" http://localhost:7081/db/save/path?local=0)
        echo "Response after save is $res" >> $test_output
        uuid=$(echo $res | jq '.uuid')
        if [ -z $res ] || [ null == $uuid ]; then
                echo "Failed to save big data" >> $test_output
                rm $save_file
                kill_metax $p
                exit;
        fi
        uuid=$(echo $res | cut -d'"' -f 4)

        curl http://localhost:7081/db/get?id=$uuid --output $get_file
        ls $test_dir/storage/cache/????????-* | cut -d'/' -f 5 | sort > $test_dir/local_list
        d=$(diff $save_file $get_file 2>&1)
        if [[ -z $d ]]; then
                echo "Got saved big file successfully" >> $test_output
                echo $PASS > $final_test_result
        else
                echo "Failed to get big data" >> $test_output
                l=$(cat $test_dir/local_list | tr "\n" ",")
                l+=",$uj_uuid"
                $CONNECT_TO_PEER "rm -f $remote_storage/{$l}"
                rm $save_file
                rm $get_file
                kill_metax $p
                exit
        fi

        expected='{"deleted":"'$uuid'"}'
        res=$(curl http://localhost:7081/db/delete?id=$uuid)
        if [ "$res" != "$expected" ]; then
                echo "Failed to delete big file. Response is $res" >> $test_output
                echo $FAIL > $final_test_result;
        fi

        # Removes uuids from remote peer's storage if failed to delete
        $CONNECT_TO_PEER "ls $remote_storage/????????-* 2>/dev/null | cut -d'/' -f 6 | sort" > $test_dir/remote_storage_list
	d=$(comm -12 $test_dir/local_list $test_dir/remote_storage_list | expr $(wc -l))
        if [ "0" -ne "$d" ]; then
                echo "Failed to delete big file in remote peer" >> $test_output
                echo $d >> $test_output
                l=$(cat $test_dir/local_list | tr "\n" ",")
                l+=",$uj_uuid"
                $CONNECT_TO_PEER "rm -f $remote_storage/{$l}"
                echo $FAIL > $final_test_result;
        fi

        rm $save_file
        rm $get_file
        $CONNECT_TO_PEER "rm -f $remote_storage/$uj_uuid"
        kill_metax $p
        echo "Time at the end of test is   $(date +%T)" >> $test_output
}

main $@
