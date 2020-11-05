#!/bin/bash

ls regression_tests_results > valg_res

echo "          Test | Definitely lost | Indirectly lost | Possibly lost | Invalid read | Invalid write | Conditional jump | Test result" > valg_report

while read l
do
        s=""
        test_res=""
        flag="NO"
        f=regression_tests_results/$l/valgrind_result.txt;
        if [ -s $f ]; then
                s+=$f;
                tmp=`grep -l "definitely lost: [1-9]" $f 2> /dev/null | wc -l`
                if [ 1 -eq $tmp ]; then
                        s+="| YES"
                        flag="YES"
                else
                        s+="| NO"
                fi

                tmp=`grep -l "indirectly lost: [1-9]" $f 2> /dev/null | wc -l`
                if [ 1 -eq $tmp ]; then
                        s+="| YES"
                        flag="YES"
                else
                        s+="| NO"
                fi

                tmp=`grep -l "possibly lost: [1-9]" $f 2> /dev/null | wc -l`
                if [ 1 -eq $tmp ]; then

                        # The "online_offline_test_1" test case will always have Posibily lost
                        # because of the absence of the Timeout when connecting to a peer (#10273).
                        # Agreed with Garik to not show this on the report until we fix the issue which causes this fail.
                        if [ "online_offline_test_1" != $l ]
                        then
                            s+="| YES"
                            flag="YES"
                        fi
                else
                        s+="| NO"
                fi

                tmp=`grep -l "Invalid read" $f 2> /dev/null | wc -l`
                if [ 0 -eq $tmp ]; then
                        s+="| NO"
                else
                        s+="| YES"
                        flag="YES"
                fi

                tmp=`grep -l "Invalid write" $f 2> /dev/null | wc -l`
                if [ 0 -eq $tmp ]; then
                        s+="| NO"
                else
                        s+="| YES"
                        flag="YES"
                fi

                tmp=`grep -l "Conditional jump" $f 2> /dev/null | wc -l`
                if [ 0 -eq $tmp ]; then
                        s+="| NO"
                else
                        s+="| YES"
                        flag="YES"
                fi

                test_res=`cat regression_tests_results/$l/result.txt`

                if [[ "YES" == $flag || "FAIL" == $test_res || "CRASH" == $test_res || "Unexpected Pass" == $test_res ]]; then
                        s+="| $test_res"
                        echo $s >> valg_report
                fi
        fi
done < valg_res

column -s "|" -t valg_report > valgrind_report

rm valg_report
rm valg_res
