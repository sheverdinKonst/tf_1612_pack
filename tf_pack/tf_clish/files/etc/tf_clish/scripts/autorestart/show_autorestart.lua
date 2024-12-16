#!/usr/bin/lua

local tf = require "tf_module"

local cmd_get_ar_status = "ubus call autorestart getStatus"
local cmd_get_ar_config = "ubus call uci get '{\"config\":\"tf_autorestart\"}'"

local ar_enum_status =
{
    status       = "Status:           ",
    test_type    = "Test type:        ",
    error_Code   = "Error code:       ",
    reboot_cnt   = "Reboot counts:    ",
    time         = "last reboot time: ",
}

local ar_testType =
{
    "link",
    "Ping",
    "Speed",
    "Time"
}

local function run_show_autorestart_status()
    local arInfo = tf.collectJsonTable(cmd_get_ar_status)
    if arInfo == nil then
    elseif arInfo["port"] == nil then
        print("error: autorestart status is empty")
    else
        arInfo = arInfo["port"]
        for index, list in pairs(arInfo) do
            print("port " .. index)
            local value = list["status"]
            local str = ar_enum_status["status"]
            print("\t" .. str .. "\t" .. value)

            if value ~= "test disable" and value ~= "port disable" then
                value = list["test_type"]
                str = ar_enum_status["test_type"]
                value = tonumber(value)
                local testType = ""
                if value >= 1 and value <= 4 then
                    testType = ar_testType[tonumber(value)]
                elseif value == 0 then
                    testType = "test_disable"
                elseif value >= 5 then
                    testType = "error: autorestart test type not defined"
                end
                print("\t" .. str .. "\t" .. testType)
                value = list["error_Code"]
                str = ar_enum_status["error_Code"]
                print("\t" .. str .. "\t" .. value)

                value = list["reboot_cnt"]
                str = ar_enum_status["reboot_cnt"]
                print("\t" .. str .. "\t" .. value)

                value = list["time"]
                str = ar_enum_status["time"]
                print("\t" .. str .. "\t" .. value)
            end
            print("============================================")
        end
    end
end

local function run_show_autorestart_config()
    local arInfo = tf.collectJsonTable(cmd_get_ar_config)
    arInfo = arInfo["values"]
    local portName = tf.portList.portName
    local arList = {}
    for i = 1, tf.portList.maxLanPorts do
        print("\t------------------------------------------")
        local portIndex = portName
        portIndex = portIndex .. i
        arList = arInfo[portIndex]

        print("port " .. " " .. i)
        print("\t" .. "State:\t" .. arList["state"])

        if arList["state"] == "ping" then
            print("\t\t\t" .. "Host:     \t" .. arList["host"])
        elseif arList["state"] == "speed" then
            print("\t\t\t" .. "max speed:\t" .. arList["max_speed"])
            print("\t\t\t" .. "min speed:\t" .. arList["min_speed"])
        elseif arList["state"] == "time" then
            print("\t\t\t" .. "timeUp:   \t" .. arList["timeUp"])
            print("\t\t\t" .. "timeDown: \t" .. arList["timeDown"])
        end
    end
end

local ar_cmdList =
{
    {cmd = "status", func = run_show_autorestart_status },
    {cmd = "config", func = run_show_autorestart_config },
}

local function run_show_autorestart()
    for _, value in pairs(ar_cmdList) do
        if arg[1] == value.cmd then
            value.func()
        end
    end
end

run_show_autorestart()


