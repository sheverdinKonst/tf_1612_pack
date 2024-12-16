#!/usr/bin/lua

dofile("/etc/tf_clish/scripts/lua/global_function.lua")

local bridgeList = executeCommand("mstpctl showbridgelist")

print("bridgeList = " .. bridgeList)
print("size = " .. #bridgeList .. " type = " .. type(bridgeList))






