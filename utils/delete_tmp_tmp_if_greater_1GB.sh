#!/bin/bash
TMP_DIR_SIZE=$(du -cb /tmp/tmp* | tail -1 | cut -f1)
# 1GB = 1073741824 Bytes
if [[ $TMP_DIR_SIZE -gt 1073741824 ]];
then
	rm -rf /tmp/tmp*
else
	echo '/tmp/tmp* size is not greater than 1GB'
fi
