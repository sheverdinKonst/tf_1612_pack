#!/bin/bash

echo "--->> set_useraccount_delete.sh <<---"

name=${1}

echo "user name:  " "${name}"
uci show rpcd > a.txt
userfound=0

while IFS= read -r line; do
    if [[ $line == *"username"* ]]; then
        username=$(echo "$line" | awk -F "'" '{print $2}')
        if [[ $username == "$name" ]]; then
            index=$(echo "$line" | awk -F "[" '{print $2}' | awk -F "]" '{print $1}')
            userfound=1
            echo "Index: $index"
          else
            userfound=0
        fi
    fi
done < "a.txt"

if [[ "${userfound}" -eq 1 ]]; then
  rpcdstr="rpcd.@login[""${index}""]"
  echo "rpcdstr:" "${rpcdstr}"
  rpcdInfo=$(uci show "${rpcdstr}")
  first_line=$(echo "${rpcdInfo}" | head -n 1)
  IFS='=' read -ra parts <<< "$first_line"
  cmd="${parts[0]}"
  uci del "${cmd}"
  uci show rpcd
else
     echo "User not found"
fi