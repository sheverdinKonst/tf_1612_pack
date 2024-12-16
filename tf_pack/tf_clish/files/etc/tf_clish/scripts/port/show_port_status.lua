#!/usr/bin/lua

print("*************** show port status ver. 0.2 ***************")
local uci = require("luci.model.uci").cursor()
dofile("/etc/tf_clish/scripts/lua/global_function.lua")

local json = require("json")
local portInfo = {}
local poeInfo = {}

function show_port_status()
    uci:foreach("port", "lan",
    function(s)
        print(s[".name"] .. ":")
        local portNum = s[".name"]
        local linkState = "down"

        print("\t state         \t\t" .. s["state"])
        if (portInfo[portNum].up) then
            linkState = "up"
        end

        print("\t link          \t\t"..  linkState)
        print("\t mtu           \t\t" .. portInfo[portNum].mtu)

        for key , value in pairs(portInfo[portNum].link) do
            if (key == "speed") then
                 print("\t speed        \t\t" .. portInfo[portNum].link.speed)
            end
            if (key == "duplex") then
                print("\t duplex        \t\t" .. portInfo[portNum].link.duplex)
            end
        end

        local _, num = separateLettersAndNumbers(portNum)

        if (num < 9 ) then
            for key, value in pairs(poeInfo.ports[portNum]) do
                if (key == "status" ) then
                    print("\t PoE " .. key .. "     \t".. value)
                end
                if (key == "consumption") then
                    print("\t PoE " ..  key .. "    \t" .. value .. "[W]")
                end
            end
        end

        print("----------------------------------------")
    end)
end

portInfo = collectJsonTable("ubus call luci-rpc getNetworkDevices")
poeInfo = collectJsonTable("ubus call poe info")

show_port_status()
