#!/bin/bash

ls regression_tests_results > valg_res

echo "<!DOCTYPE html>
<html>
<head>
<title></title>
</head>
<body>" > valgrind_report.html

echo "<table><tr><th>Test</th><th>Definitely lost | </th><th>Indirectly lost | </th><th>Possibly lost | </th><th>Invalid read | </th><th>Invalid write | </th><th>Conditional jump | </th><th>Test result</th></tr>" >> valgrind_report.html

while read l
do
        s_html=""
        test_res=""
        flag="NO"
        f=regression_tests_results/$l/valgrind_result.txt;
        if [ -s $f ]; then
                s_html+="<tr><td><a href=$f>$l</a></td>";
                tmp=`grep -l "definitely lost: [1-9]" $f 2> /dev/null | wc -l`
                if [ 1 -eq $tmp ]; then
                        s_html+="<td style=\"color:red\">YES</td>"
                        flag="YES"
                else
                        s_html+="<td>NO</td>"
                fi

                tmp=`grep -l "indirectly lost: [1-9]" $f 2> /dev/null | wc -l`
                if [ 1 -eq $tmp ]; then
                        s_html+="<td style=\"color:red\">YES</td>"
                        flag="YES"
                else
                        s_html+="<td>NO</td>"
                fi

                tmp=`grep -l "possibly lost: [1-9]" $f 2> /dev/null | wc -l`
                if [ 1 -eq $tmp ]; then

                        # The "online_offline_test_1" test case will always have Posibily lost
                        # because of the absence of the Timeout when connecting to a peer (#10273).
                        # Agreed with Garik to not show this on the report until we fix the issue which causes this fail.
                        if [ "online_offline_test_1" != $l ]
                        then
                            s_html+="<td style=\"color:red\">YES</td>"
                            flag="YES"
                        fi
                else
                        s_html+="<td>NO</td>"
                fi

                tmp=`grep -l "Invalid read" $f 2> /dev/null | wc -l`
                if [ 0 -eq $tmp ]; then
                        s_html+="<td>NO</td>"
                else
                        s_html+="<td style=\"color:red\">YES</td>"
                        flag="YES"
                fi

                tmp=`grep -l "Invalid write" $f 2> /dev/null | wc -l`
                if [ 0 -eq $tmp ]; then
                        s_html+="<td>NO</td>"
                else
                        s_html+="<td style=\"color:red\">YES</td>"
                        flag="YES"
                fi

                tmp=`grep -l "Conditional jump" $f 2> /dev/null | wc -l`
                if [ 0 -eq $tmp ]; then
                        s_html+="<td>NO</td>"
                else
                        s_html+="<td style=\"color:red\">YES</td>"
                        flag="YES"
                fi

                test_res=`cat regression_tests_results/$l/result.txt`
                if [[ "YES" == $flag || "FAIL" == $test_res || "CRASH" == $test_res || "Unexpected Pass" == $test_res ]]; then
                        s_html+="<td>$test_res</td></tr>"
                        echo $s_html >> valgrind_report.html
                fi
        fi
done < valg_res

echo "</table>" >> valgrind_report.html
echo "</body>
</html>" >> valgrind_report.html

rm valg_res
