#!/bin/bash

# Test case flow:
# - Runs one instance of metax.
# - Sends "sendmail" request.
# - Checks that message was sent successfully.

source ../../test_utils/functions.sh

# wait command exit code when metax was successfully finished (i.e. without Seg. fault or Assertion)
EXIT_CODE_OK=0


main()
{
        $EXECUTE -f $tst_src_dir/config.xml &
        p=$!
        wait_for_init 7081
	echo -n $p > $test_dir/processes_ids

	success=$(curl \
		-F "to=marina@leviathan.am,Zarine Grigoryan <zarine@leviathan.am>" \
		-F "cc=ani@leviathan.am, ,lena@leviathan.am " \
		-F "bcc=Garik Yesayan <garik@leviathan.am>" \
		-F "from=No reply <no-reply@leviathan.am>" \
		-F "password=no-replyLeviathan" \
		-F "subject=Metax mail sender" \
		-F "message=Hi, this is Metax mail sender" \
		-F "server=mail.leviathan.am" \
		-F "port=587" \
		http://localhost:7081/sendemail)

	echo $success
        success=$(echo $success | cut -d':' -f2 | cut -d'}' -f1)

        if [ "1" != "$success" ]; then
                echo "Sending email by metax failed"
                kill_metax $p
                exit;
        fi

        echo $PASS > $final_test_result
        kill_metax $p
}

main $@

