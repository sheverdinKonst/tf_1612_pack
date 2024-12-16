#!/bin/bash

echo "--->> set_useraccount_add.sh ver 2.0<<---"

name=${1}
rule=${2}
#timeout=${3}
readsym="*"
writesym=""
psw1="\$"
psw2="p\$"

index=256
id=0
paswd="${psw1}""${psw2}""${name}"
echo "user name:  " "${name}"
echo "passsword:  " "${paswd}"
echo "admin rule: " "${rule}"

uci show rpcd > a.txt

while IFS= read -r line; do
    if [[ $line == *"username"* ]]; then
        username=$(echo "$line" | awk -F "'" '{print $2}')
        if [[ $username == "$name" ]]; then
            index=$(echo "$line" | awk -F "[" '{print $2}' | awk -F "]" '{print $1}')
            echo "Index: $index"
        fi
    fi
done < "a.txt"

if [[ "${rule}" == "full" ]]; then
  echo "Ruele: full->" "${rule}"
  writesym="*"
fi

if [[ $index == 256 ]]; then
   echo "add new user"
   echo "user name:  " "${name}"
   echo "passsword:  " "${paswd}"
   echo "admin rule: " "${rule}"

   echo  "readsym  = " "${readsym}"
   echo  "writesym = " "${writesym}"

   id=$(uci add rpcd login)
   echo "id = ${id}"
   uci set rpcd.@login[-1].username="${name}"
   uci set rpcd.@login[-1].password="${paswd}"
   #uci set rpcd.@login[-1].timeout="${timeout}"
   uci add_list rpcd.@login[-1].read="${readsym}"
   if [[ -n "${writesym}" ]]; then
    uci add_list rpcd.@login[-1].write="${writesym}"
   fi
   #adduser "${name}" -H
else
  echo "ERROR: A USER with this name already exists"
fi
