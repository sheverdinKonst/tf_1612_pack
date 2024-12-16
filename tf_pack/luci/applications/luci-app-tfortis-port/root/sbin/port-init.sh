#!/bin/sh
# Licensed to the GNU General Public License v3.0.
logger -p notice "ports config changed"

max=9
for i in `seq 0 $max`
do
	#link settings
	if [[ $(uci -q get port.@lan[$i].state) = "enable" ]]
	then 
		ip link set lan$(($i+1)) up
	else 
		ip link set lan$(($i+1)) down
	fi
		
	#speed/duplex
	SPEED=$(uci -q get port.@lan[$i].speed)
	if [ "$SPEED" = "auto" ]
	then
		ethtool -s lan$(($i+1)) autoneg on		
	else 
		ethtool -s lan$(($i+1)) speed $SPEED autoneg off
	fi
	
	#flow control
	if [[ $(uci -q get port.@lan[$i].flow) = "enable" ]]
	then
		ethtool -A lan$(($i+1)) tx on rx on
	else 
		ethtool -A lan$(($i+1)) tx off rx off
	fi		    
    ethtool -r lan$(($i+1))
	
	#poe settings
	#if [[ $(uci -q get port.@lan[$i].poe) = "enable" ]]
	#then 
	#	uci set poe.port$(($i+1)).enable=1 
	#else 
	#	uci set poe.port$(($i+1)).enable=0
	#fi	
done 

uci commit
/etc/init.d/poe reload

exit 0