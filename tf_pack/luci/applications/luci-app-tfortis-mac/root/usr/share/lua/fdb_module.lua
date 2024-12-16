#!/usr/bin/env lua

local util = require "luci.util"

local fdb_module = {}
local jsonInfo  = {}
local result = ""

function fdb_module.getJson()
	local patternEnd = "(.-)\n"
	local patternSpase = " "
	local superline = "["
	local substrings = {}
	local compleat
	local text = luci.util.exec("/usr/sbin/bridge fdb show dynamic")
	for line in text:gmatch(patternEnd) do
		if(string.find(line,"vlan")) then
			--print("Parsed line:", line)
			for substring in line:gmatch("[^" .. patternSpase .. "]+") do
				-- Insert each substring into the table
				table.insert(substrings, substring)
			end
			compleat = "{\"mac\":\"" .. substrings[1] .. "\",\"vid\":\"VLAN" .. substrings[5] .. "\",\"interface\":\"" .. substrings[3] .. "\"}"
			superline = superline .. compleat .. ","
			substrings = {}
		end
	end
	superline = string.sub(superline, 1, -2) --remove last comma
	superline = superline .. "]"
	return superline
end

--if executed as standalone script
if arg[1] == "getJson" then
    print(fdb_module.getJson())
end

return fdb_module
