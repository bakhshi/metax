#!/bin/bash

# Test case flow: 
# - Runs one instance of metax.
# - Sends "data" save request for a generated file (default it is saved as encrypted).
# - Generates a new random file.
# - Sends "data" update request for received uuid with new generated file
#   (default it is saved as encrypted).
# - Sends get request for received uuid.
# - Checks the compliance of last generated/received files.

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

        dd if=/dev/urandom of=$save_file bs=16000000 count=1
	uuid=$(curl --form "fileupload=@$save_file" http://localhost:7081/db/save/data)
	uuid=$(echo $uuid | cut -d'"' -f 4)
	dd if=/dev/urandom of=$update_file bs=16000000 count=1
	res=$(curl --form "fileupload=@$update_file" http://localhost:7081/db/save/data?id=$uuid)
	uuid1=$(echo $res | cut -d'"' -f 4)
	if [ "$uuid" == "$uuid1" ]; then 
		curl http://localhost:7081/db/get/?id=$uuid1 --output $get_file
		d=$(diff $update_file $get_file 2>&1)
		if [[ -z $d ]]; then
			echo $PASS > $final_test_result
		fi
	fi
	kill_metax $p
}

main $@
