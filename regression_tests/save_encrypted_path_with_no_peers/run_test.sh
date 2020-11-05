#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Generates random file.
# - Sends "path" save request for generated file (default it is saved as encrypted).
# - Sends get request for received uuid.
# - Checks the compliance of generated and received files.

save_file=$test_dir/save_file
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
	curl http://localhost:7081/db/get/?id=$uuid --output $get_file
	d=$(diff $save_file $get_file 2>&1)
	if [[ -z $d ]]; then
		echo $PASS > $final_test_result
	fi
	kill_metax $p
}

main $@
