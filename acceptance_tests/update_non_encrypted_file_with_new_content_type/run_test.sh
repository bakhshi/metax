#!/bin/bash

# Test case flow:
# - Runs an instance of metax with config.xml configuration.
# - Sends "save" request with "text/plain" content type.
# - Sends "update" request with changing data and content type.
# - Checks the compliance of received/expected messages.
# - Sends "get" request with the saved/updated uuid.
# - Checks the compliance of saved/get data.

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

        dd if=/dev/urandom of=$save_file bs=10 count=1
        save_content_type="text/plain"
        save=$(curl --form "fileupload=@$save_file" -H "Metax-Content-Type:$save_content_type" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $save | cut -d'"' -f 4)

        c1=$(cat $test_dir/storage/$uuid | jq -r '.content_type')
        if [ "$save_content_type" != "$c1" ]; then
                echo "Incorrect content type after save"
                echo $FAIL > $final_test_result
                kill_metax $p
                exit
        fi

        dd if=/dev/urandom of=$update_file bs=10 count=1
        update_content_type="application/json"
        update=$(curl --form "fileupload=@$update_file" -H "Metax-Content-Type:$update_content_type" "http://localhost:7081/db/save/data?enc=0&id=$uuid")
        uuid1=$(echo $update | cut -d'"' -f 4)
        if [ "$uuid1" != "$uuid" ]; then
                echo "Failed to update"
                echo $FAIL > $final_test_result
                kill_metax $p
                exit
        fi

        c2=$(cat $test_dir/storage/$uuid | jq -r '.content_type')
        if [ "$update_content_type" != "$c2" ]; then
                echo "Incorrect content type after update"
                echo $FAIL > $final_test_result
                kill_metax $p
                exit
        fi

        curl http://localhost:7081/db/get?id=$uuid --output $get_file
        d=$(diff $save_file $get_file) 
        if [[ -s $d ]]; then
                echo "Saved data has been changed."
                echo $FAIL > $final_test_result
                kill_metax $p
                exit
        fi

        echo $PASS > $final_test_result
        kill_metax $p
}

main $@

# vim:et:tabstop=8:shiftwidth=8:cindent:fo=croq:textwidth=80:
