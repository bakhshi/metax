#!/bin/bash

# Test case flow: 
# - runs two connected Metax peers 1 and 2
# - saves encrypted data in peer 1
# - shares whith username of peer 2
# - gets appropriate error message while public key of peer 2 is not available
#   for peer 1
# - add peer 2 public key using add_peer API
# - shares the saved uuid again
# - share successes
# - peer 2 gets and checks the save data


source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config_wo_sitemap.xml &
	p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        file_name=index.html
        uuid=$(curl --form "fileupload=@$tst_src_dir/$file_name" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sed -e "s/%UUID%/$uuid/" $tst_src_dir/sitemap_template.json > $test_dir/sitemap.json
        uuid=$(curl --form "fileupload=@$test_dir/sitemap.json" http://localhost:7081/db/save/data?enc=0)
        uuid=$(echo $uuid | cut -d'"' -f 4)
        sed -e "s/%SITEMAP_UUID%/$uuid/" $tst_src_dir/config_wo_sitemap.xml > $test_dir/config_with_sitemap.xml
        kill_metax $p;

        $EXECUTE -f $test_dir/config_with_sitemap.xml &
	p=$!
        wait_for_init 7081
        echo -n $p > $test_dir/processes_ids

        curl http://127.0.0.1:7081/$file_name > $test_dir/$file_name
        diff=$(diff $tst_src_dir/$file_name $test_dir/$file_name)
        if [ "" != "$diff" ]
        then
                echo "Original and Metax stored index.html files differ"
                kill_metax $p;
                exit
        fi

        echo $PASS > $final_test_result
        kill_metax $p;
}

main $@
