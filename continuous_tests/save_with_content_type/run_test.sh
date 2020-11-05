#!/bin/bash

# Test case flow: 
# - runs one Metax instance
# - saves file/data using all three save APIs (data/path/node)
#       - Passes Metax-Content-Type header to save requests
# - gets saved file/data and checks their correctness, checks that Content-Type
#   of get requests are set and correct.

get_file=$test_dir/logo.svg
save_file=$tst_src_dir/leviathan_logo.svg
headers_file=$test_dir/headers_file


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0



main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        uuid=$(curl --form "fileupload=@$save_file" -H "Metax-Content-Type: image/svg+xml" http://localhost:7081/db/save/data)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        curl --output $get_file -D $headers_file http://localhost:7081/db/get/?id=$uuid
        d=$(diff $save_file $get_file 2>&1)
        if [[ ! -z $d ]]
        then
                echo "Save request failed"
                kill_metax $p
                exit
        fi
        grep "Content-Type: image/svg+xml" $headers_file
        if [ $? -ne 0 ];
        then
                echo "Content type error"
                kill_metax $p
                exit
        fi

        uuid=$(curl --data "$save_file" -H "Metax-Content-Type: image/svg+xml" http://localhost:7081/db/save/path)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        curl --output $get_file -D $headers_file http://localhost:7081/db/get/?id=$uuid
        d=$(diff $save_file $get_file 2>&1)
        if [[ ! -z $d ]]
        then
                echo "Save data failed"
                kill_metax $p
                exit;
        fi
        grep "Content-Type: image/svg+xml" $headers_file
        if [ $? -ne 0 ];
        then
                echo "Content type error"
                kill_metax $p
                exit
        fi

        save_content="A content"
        uuid=$(curl --data "$save_content" -H "Metax-Content-Type: image/svg+xml" http://localhost:7081/db/save/node)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        get_content=$(curl -D $headers_file http://localhost:7081/db/get?id=$uuid)
        if [ "$get_content" != "$save_content" ]; then
                echo "Save data failed"
                kill_metax $p
                exit;
        fi
        grep "Content-Type: image/svg+xml" $headers_file
        if [ $? -ne 0 ];
        then
                echo "Content type error"
                kill_metax $p
                exit
        fi

        echo $PASS > $final_test_result
        kill_metax $p;
}

main $@
