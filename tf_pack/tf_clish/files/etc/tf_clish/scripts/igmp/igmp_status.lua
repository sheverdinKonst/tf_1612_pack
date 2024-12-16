#!/usr/bin/lua

---
--- Generated by Luanalysis
--- Created by sheverdin.
--- DateTime: 2/19/24 12:22 PM
---
---

local tf = require "tf_module"
local igmp = require "igmp_module"

local get_netConfig   = "ubus call uci get \'{\"config\":\"network\"}\'"

local igmp_status = {}

local macReserved_igmp_ipv4 = {
    { "01005E000000", "01005E7FFFFF", "IPv4 Multicast (RFC 1112)" },
}

local macReserved_igmp_ipv6 = {
    { "333300000000", "3333FFFFFFFF", "IPv6 Multicast (RFC 2464)" },
}

local mCastState = {
    m_ipv4  = "ipv4",
    m_ipv6  = "ipv6",
    none    = "none"
}

local function igmp_getRawStatus()
    local dev_mcast = tf.executeCommand("cat /proc/net/dev_mcast")
    return dev_mcast
end

local function addDoubleDot(mac)
    local doubleDot = ""
    doubleDot = doubleDot .. string.sub(mac, 1, 2) .. ":" .. string.sub(mac, 3, 4)  .. ":" .. string.sub(mac, 5, 6) .. ":"
            .. string.sub(mac, 7, 8) .. ":" .. string.sub(mac, 9, 10) .. ":" .. string.sub(mac, 11, 12)
    return doubleDot
end

local function getMatch(mac)
    local macPattern_v4_1 = "01005e"
    local macPattern_v4_2 = "01005E"
    local macPattern_v6 = "3333"
    local state = mCastState.none
    if string.sub(mac, 1, 6) == macPattern_v4_1 or  string.sub(mac, 1, 6) == macPattern_v4_2 then
        state = mCastState.m_ipv4
    elseif string.sub(mac, 1, 4) == macPattern_v6 then
        state = mCastState.m_ipv6
    end
    return state
end

function igmp_status.main_show_igmp_status(portRange)
    local json_output = ""
    local isPrint = 0
    local igmpStatus = igmp_getRawStatus()
    local rawJsonData, ctrl = igmp.getRawJsonData(igmpStatus)
    json_output = igmp.convertTableToJson(rawJsonData, ctrl)
    local igmpTable = tf.decodeToJson(json_output)
    local netConfig = tf.collectJsonTable(get_netConfig)
    local portList = tf.getPortList(netConfig["values"], "switch", "ports")
    for _, port in pairs(portList) do
        local lanNum, _ = tf.getPort(port)
        isPrint = tf.isPrintPortRange(portRange, lanNum)
        if isPrint == 1 then
            isPrint = 0
            io.write("\n>> ", port, ":\n")
            for k, l in pairs(igmpTable.mcast.interfaces[port]) do
                local printData = ""
                printData = printData .. "\t-------------------------------------\n"
                for _, v in pairs(igmp.dev_mcast) do
                    if v.key == "bound_addr" then
                        local mac = igmpTable.mcast.interfaces[port][k][v.key]
                        local state = getMatch(mac)
                        if state == mCastState.m_ipv4 then
                            printData = printData .. v.str .. addDoubleDot(mac) .. " - " .. macReserved_igmp_ipv4[1][3] .. "\n"
                            io.write(printData)
                        elseif state == mCastState.m_ipv6 then
                            printData = printData .. v.str .. addDoubleDot(mac) .. " - " .. macReserved_igmp_ipv6[1][3] .. "\n"
                            io.write(printData)
                        end
                    else
                        printData = printData .. v.str .. igmpTable.mcast.interfaces[port][k][v.key] .. "\n"
                    end
                end
            end
        end
    end
end

return igmp_status
