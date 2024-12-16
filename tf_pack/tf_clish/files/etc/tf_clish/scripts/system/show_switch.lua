#!/usr/bin/lua

package.path = "/etc/tf_clish/scripts/vlan/?.lua;" .. package.path

local tf          = require "tf_module"
local utils       = require "utils"
local vlan        = require "vlan_utils"
local vlan_module = require "vlan_module"

local lanInfoMain = {}

local cmdList = {
    cmd_systemBoard       = "ubus call system board",
    cmd_systemBootloader  = "ubus call uci get \'{\"config\":\"system\",\"section\":\"tfortis\", \"option\":\"bootloader\"}\'",
    cmd_systemDescription = "ubus call uci get '{\"config\":\"system\",\"section\":\"tfortis\"}'",
    cmd_systemTimeInfo    = "ubus call system info",
    cmd_netIfInfo         = "ubus call network.interface dump",
    cmd_netInfo           = "ubus call uci get \'{\"config\":\"network\"}'",
    cmd_netLanInfo        = "ubus call uci get \'{\"config\":\"network\", \"section\":\"",
    cmd_netSwitchInfo     = "ubus call uci get \'{\"config\":\"network\",\"section\":\"switch\",\"option\":\"macaddr\"}\'",
}

local function printBootloaderVersion()
    local bootVer = tf.collectJsonTable(cmdList.cmd_systemBootloader)
    for key, value in pairs(bootVer) do
        if (key == "value") then
            print("\tBootloader Version:" .. "\t" .. value)
        end
    end
end

local function printSyStemBoard()
    print("System Info:")
    local systemBoard = tf.collectJsonTable(cmdList.cmd_systemBoard)

    local value = systemBoard["board_name"]
    print("\t" .. "Device type:      " .. "\t" .. value)

    value = systemBoard["hostname"]
    print("\t" .. "Host name:        " .. "\t" .. value)

    local release = systemBoard["release"]
    value = release["version"]
    print("\t" .. "Firmware Version: " .. "\t" .. value)
    --printBootloaderVersion()
end

local function printSyStemDescription()
    local systemInfo = tf.collectJsonTable(cmdList.cmd_systemDescription)
    systemInfo = systemInfo.values

    if systemInfo["description"] ~= nil then
        print("\t" .. "Device Description: " .. "\t" .. systemInfo["description"])
    else
        print("\t" .. "Device Description: " .. "\t" .. "----- not set -------")
    end
    if systemInfo["contacts"] ~= nil then
        print("\t" .. "Device Contact:     " .. "\t" .. systemInfo["contacts"])
    else
        print("\t" .. "Device Contact:     " .. "\t" .. "----- not set -------")
    end
    if systemInfo["location"] ~= nil then
        print("\t" .. "Device Location:    " .. "\t" .. systemInfo["location"])
    else
        print("\t" .. "Device Location:    " .. "\t" .. "----- not set -------")
    end
end

local function printSystemInfo()

    print("Time Info:")
    local time = tf.executeCommand("date +\"%d %B %Y %T\"")
    local timeInfo = tf.collectJsonTable(cmdList.cmd_systemTimeInfo)
    local timeZoneInfo = tf.collectJsonTable(cmdList.cmd_systemDescription)
    if timeZoneInfo.values == nil then
        print("error: system config is wrong")
        return
    end
    timeZoneInfo = timeZoneInfo.values
    timeZoneInfo = timeZoneInfo["zonename"]
    local timeZone = tonumber(string.match(timeZoneInfo, "-?%d+"))
    local timeZone_sec = timeZone * 3600
    if timeZone >= -12 and timeZone <= 12 then

    else
        print("error: timezone is wrong")
    end

    if time ~= nil then
        string.gsub(time, "\n", "")
        io.write("\tSystem Time: ", "\t\t", time)
    end

    local uptime = timeInfo["uptime"]
    local date = utils.convertSeconds(tonumber(uptime))
    print("\t" .. "System Uptime:" .. "\t\t" .. date.days .. " days and " .. date.hours .. ":" .. date.minutes .. ":" .. date.sec .. " (HH:MM:SS)")
end

local netList =
{
    device  = "\tDevice: \t",
    ipaddr  = "\t\tIP Address:      \t",
    netmask = "\t\tSubnet Mask:     \t",
    proto   = "\t\tProto:           \t",
    gateway = "\t\tDefault Gateway: \t"
}

local function setNetParam(key, list)
    local data = {}
    if list[key] ~= nil then
        data.value = list[key]
        data.str   = netList[key]
    else
        data.str   = netList[key]
        data.value = "----- not set -------"
    end
    return data
end

local function getNetLanInfo()
    print("Net Info:")
    local ifList = tf.getIfList(cmdList.cmd_netIfInfo)
    local lanInfo_arr = {}
    for i, ifanme in pairs(ifList) do
        local cmd = cmdList.cmd_netLanInfo .. ifanme .. "\"}'"
        local lanInfo = tf.collectJsonTable(cmd)
        local lanInfo_data = {}

        for key, _ in pairs(netList) do
            lanInfo_data[key] = setNetParam(key, lanInfo.values)
        end
        lanInfo_arr[ifanme] = lanInfo_data
    end
    lanInfoMain["if_list"] = lanInfo_arr
end

local function getNetMacInfo()
    local
    macInfo = tf.collectJsonTable(cmdList.cmd_netSwitchInfo)
    local data = {}
    data.str   = "\tMAC Address:       \t\t"
    if macInfo.value ~= nil then
        data.value = macInfo.value
    else
        data.value = "----- not set -------"
    end
    lanInfoMain["mac"] = data
end

local function printNetInfo()
    for i, if_list in pairs(lanInfoMain["if_list"]) do
        io.write(tostring(if_list["device"].str),  if_list["device"].value,  "\n")
        io.write(tostring(if_list["proto"].str),   if_list["proto"].value,   "\n")
        io.write(tostring(if_list["ipaddr"].str),  if_list["ipaddr"].value,  "\n")
        io.write(tostring(if_list["netmask"].str), if_list["netmask"].value, "\n")
        io.write(tostring(if_list["gateway"].str), if_list["gateway"].value, "\n")
    end
    io.write(tostring(lanInfoMain["mac"].str), lanInfoMain["mac"].value, "\n")
end

local function printVlanId()
    print(" ")
    local netInfo = tf.collectJsonTable(cmdList.cmd_netInfo)
    local vlanArr = vlan_module.getBridgeVlanList(netInfo["values"])
    local vlanId_str = ""
    if (vlanArr ~= nil) then
        for vlan_id, _ in pairs(vlanArr) do
            if string.len(vlanId_str) == 0 then
                vlanId_str = tostring(vlan_id)
            else
                vlanId_str = vlanId_str .. ", " .. tostring(vlan_id)
            end
        end
        print("\tVlan ID:             \t\t" .. vlanId_str)
    else
        print("\tVlan ID:             \t\t" .. "Vlan not configured correctly")
    end
end

local function run_show_switch()
    print(" -------------------------------------------------------------- ")
    printSyStemBoard()
    printSyStemDescription()
    printSystemInfo()
    getNetLanInfo()
    getNetMacInfo()
    printNetInfo()
    printVlanId()
    print(" -------------------------------------------------------------- ")
end

run_show_switch()
