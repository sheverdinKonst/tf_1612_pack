#!/usr/bin/lua

---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by sheverdin.
--- DateTime: 6/17/24 12:43 PM
---
---
package.path = "/etc/tf_clish/scripts/vlan/?.lua;" .. package.path

local tf            = require "tf_module"
local vlan          = require "vlan_module"

local vlanRange_in     = ""
local cmd_in           = ""
local taggedType_in    = ""
local portList_in      = ""
local device_in        = "switch"

local if_deleted_list = {}

local portRange = {}
local vlanRange = {}

local function add_tagged(vlanArr, cmd)
    vlan.add_tagged(vlanArr, vlanRange, portRange, "add")
end

local function add_untagget(vlanArr, cmd)
    if vlanRange[3] == "range" then
        print("error: for this operation choose only one Vlan id")
    else
        vlan.add_untagged(vlanArr, vlanRange[1], portRange, cmd)
    end
end

local function add_not_memb(vlanArr, _)
    vlan.add_notMemberPorts(vlanArr, vlanRange, portRange)
end

local cmdAdd_List =
{
    tagged     = { cmd = "tagged",      func = add_tagged },
    untagged   = { cmd = "untagged",    func = add_untagget },
    not_memb   = { cmd = "mot_member",  func = add_not_memb },
}

local function vlan_add(vlanArr, _)
    local cmdList = cmdAdd_List[taggedType_in]
    cmdList.func(vlanArr, "add")
end

local function vlan_edit(vlanArr, _)
    --print("taggedType_in = " .. taggedType_in)
    local cmdList = cmdAdd_List[taggedType_in]
    cmdList.func(vlanArr, "edit")
end

local function vlan_delete(vlanArr)
    for vlan_num, vlan_list in pairs(vlanArr) do
        local index = tonumber(vlan_num)
        local vlanRangeMin = tonumber(vlanRange[1])
        local vlanRangeMax = tonumber(vlanRange[2])

        if index >= vlanRangeMin and index <= vlanRangeMax then
            if vlan_list.mngtVlan == nil then
                print("\nwarning:\n \t\tThis operation may result in LOSS OF COMMUNICATION with the switch")
                print("\t\tBefore saving the changes, make sure they are correct !!!")
                vlan.delete(vlan_list.id)
            else
                if vlan_list.mngtVlan == "yes" then
                    print("\nwarning:\n \t\tVLAN *" .. index .. "* is Management VLAN and can not be deleted !!!")
                    if type(vlan_list.ifname) == "table" then
                        print("\t\tinterface ")
                        for _, ifname in pairs(vlan_list.ifname) do
                            print("\t\t\t*" .. ifname .. "* based on VLAN *" .. index .. "*")
                        end
                    else
                        print("\t\tinterface *" .. vlan_list.ifname .. "* based on VLAN *" .. index .. "*")
                    end
                end
            end
        end
    end
end

local function vlan_param(vlanArr)
    for vlan_num, vlan_list in pairs(vlanArr) do
        local index = tonumber(vlan_num)
        local vlanRangeMin = tonumber(vlanRange[1])
        local vlanRangeMax = tonumber(vlanRange[2])
        local str = taggedType_in
        if cmd_in == "name" then
            cmd_in = "descr"
        end
        if index >= vlanRangeMin and index <= vlanRangeMax then
            vlan.set_param(vlan_list.id, cmd_in, str)
        end
    end
end

local function vlan_mngt_vlan(vlanArr)
    local vlan_id = arg[1]
    local vlanList = {}
    local ifList = tf.getIfList(vlan.cmd_vlan_list.cmd_netIfInfo)
    local status = "error"
    local ifName = taggedType_in

    for _, if_name in pairs(ifList) do
        if ifName == if_name then
            status = "OK"
            break
        end
    end

    if status == "OK" then
        if vlanArr[vlan_id] == nil then
            print("error: vlan *" .. vlan_id .. "* not presented")
            status = "error"
        else
            vlanList = vlanArr[vlan_id]
            status = "OK"
        end
    end

    if status == "error" then
        return 0
    else
        vlan.mngt_vlan(vlanList, vlan_id, ifName)
    end
end

local function vlan_show(vlanArr)
    local vlanId = arg[2]
    vlan.showVlanInfo(vlanArr, vlanId)
end

local cmdVlanList =
{
    add       = { cmd = "add",       func = vlan_add },
    edit      = { cmd = "edit",      func = vlan_edit },
    delete    = { cmd = "delete",    func = vlan_delete },
    name      = { cmd = "name",      func = vlan_param },
    state     = { cmd = "state",     func = vlan_param },
    mngt_vlan = { cmd = "mngt_vlan", func = vlan_mngt_vlan },
    show      = { cmd = "show",      func = vlan_show },
}

local function getVlanArr()
    local netInfo = tf.collectJsonTable(vlan.cmd_vlan_list.cmd_netInfo)
    local vlanArr = vlan.getBridgeVlanList(netInfo["values"])

    if (vlanArr == nil) then
        print("error: config bridge-vlan")
    end
    local ifList = tf.getIfList(vlan.cmd_vlan_list.cmd_netIfInfo)
    --tf.printarray(vlanArr)
    for _, if_name in pairs(ifList) do
        local cmd = vlan.cmd_vlan_list.cmd_deviceInfo_1 .. if_name .. vlan.cmd_vlan_list.cmd_deviceInfo_2
        local marked = "no"
        vlanArr, marked = vlan.getMngmntVlan(cmd, vlanArr, if_name)
        if marked == "yes" then
            table.insert(if_deleted_list, if_name)
        end
    end
    if if_deleted_list == nil then
        print("deleted list is empty")
    elseif if_deleted_list ~= nil and #if_deleted_list > 0 then
        print("info: the system has not saved configuration")
        print("\t - For show changes  *changes show*")
        print("\t - For save configuration *changes save*")
        print("\t - For revert configuration *changes revert <config_type>*")
    end
    return vlanArr
end

local function run_config_vlan()
    if arg[1] == "vlan_id" then
        vlanRange_in     = arg[2]
        cmd_in           = arg[3]
        taggedType_in    = arg[4]
        portList_in      = arg[5]

        portRange = tf.checkPortRange(portList_in)
        vlanRange = tf.checkPortRange(vlanRange_in)

        local vlanArr = getVlanArr()
        local res = ""
        if cmd_in == "add" then
            if vlanRange[3] == "range" then
                local vlanRangeMin = tonumber(vlanRange[1])
                local vlanRangeMax = tonumber(vlanRange[2])

                for i = vlanRangeMin, vlanRangeMax do
                    if vlanArr[tostring(i)] == nil then
                        res = vlan.uci_addVlan(tostring(i))
                    end
                end
            elseif vlanRange[3] == "once" then
                if vlanArr[tostring(vlanRange[1])] == nil then
                    res = vlan.uci_addVlan(vlanRange[1])
                end
            end
        end

        if res == "OK" then
            --print("vlan added - OK")
            vlanArr = getVlanArr()
        end

        if portRange ~= nil and vlanRange ~= nil then
            local cmdList = cmdVlanList[cmd_in]
            cmdList.func(vlanArr)
        end
    elseif arg[1] == "mngt_vlan" then
        if #arg[2] > 16 then
            print("error: interface name is very long")
            return
        end
        local vlanArr = getVlanArr()
        vlanRange_in = arg[3]
        vlanRange = tf.checkPortRange(vlanRange_in)
        if vlanRange[3] == "once" then
            vlan.mngtVlan_handler(vlanArr, vlanRange[1], arg[2], arg[4], if_deleted_list)
        else
            print("error: vlan id not corrected")
            return
        end
    end
end

run_config_vlan()
