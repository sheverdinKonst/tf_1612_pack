#!/usr/bin/lua

print("*************** show port  ver. 0.2 ***************")
local uci = require("luci.model.uci").cursor()
dofile("/etc/tf_clish/scripts/lua/global_function.lua")


local function show_port_config()
    uci:foreach("port", "lan",
    function(s)
        print(s[".name"] .. ":")
        print("\t state \t" .. s["state"])
        print("\t speed \t" .. s["speed"])
        print("\t flow  \t" .. s["flow"])
        print("\t poe   \t" .. s["poe"])
        print("----------------------------------------")
    end)
end


local function run_port_show_status()

end

local function run_port_show_config()

end

local cmd_ioList =
{
    show   = { cmd = "status", func = run_port_show_status },
    config = { cmd = "config", func = run_port_show_config },
}

local function run_main_port()
    local cmd = arg[1]
    local cmd_list = cmd_ioList[cmd]
    cmd_list.func()
end

run_main_port()

