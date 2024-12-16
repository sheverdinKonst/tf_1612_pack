#!/usr/bin/env lua

local util = require "luci.util"

local arp_module = {}
local jsonInfo  = {}
local result = ""

function arp_module.getJson()
	local patternEnd = "(.-)\n"
	local patternSpase = " "
	local superline = "["
	local substrings = {}
	local compleat
	local arp_raw = luci.util.exec("ip neigh show nud reachable")
	for line in arp_raw:gmatch(patternEnd) do
		--print("Parsed line:", line)
		if(string.find(line,"dev")) then
			for substring in line:gmatch("[^" .. patternSpase .. "]+") do
				-- Insert each substring into the table
				table.insert(substrings, substring)
			end		
			compleat = "{\"ip\":\"" .. substrings[1] .. "\",\"mac\":\"" .. substrings[5] .. "\",\"device\":\"" .. substrings[3] .. "\"}"
			superline = superline .. compleat .. ","
			substrings = {} --clear
		end
	end
	superline = string.sub(superline, 1, -2) --remove last comma
	superline = superline .. "]"
	return superline
end

--if executed as standalone script
if arg[1] == "getJson" then
    print(arp_module.getJson())
end

return arp_module
