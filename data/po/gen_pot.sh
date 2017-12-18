#!/bin/bash

files=("../../pam/pam.c")

for file in ${files[@]}
do
    xgettext -F -o deepin-fprintd.pot -j -kTr -C --from-code=utf-8 ${file}
done
