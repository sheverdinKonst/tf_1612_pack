---
--- Generated by EmmyLua(https://github.com/EmmyLua)
--- Created by sheverdin.
--- DateTime: 3/11/24 4:58 PM
---

require "bit"

local tf          = require "tf_module"
local ar          = require "autorestart_module"
local poe         = require "tf_poe_module"
local i2c         = require "i2c_module"
local snmp_base   = require "snmp_base"

local snmp_module = {}

local function get_ubusSize(ubusStruct)
    local count = 0
    for key, value in pairs(ubusStruct) do
        if type(value) == "table" then
            count = count + 1
        end
        --print("key = " .. key)
    end
    return count
end

local function snmp_getSPFdata(snmp_obj, index)
    local jsonInfo
    local res = ""
    local name = ""
    local status = "OK"

    if tonumber(index) <= 2 then
        if snmp_obj.name == "portSfpIndex" then
            res = index
            --elseif snmp_obj.name == "portSfpTxOutPowerDb" or snmp_obj.name == "portSfpRxOutPowerDb" then
            --    res = "3227"
        else
            name = snmp_obj.name .. "_" .. tostring(index)
            jsonInfo = tf.getUbusDataByName(name)
            res = jsonInfo.hw_sys[name]
        end

        --print("RES = " .. res)
        if (snmp_obj.isZero == "yes") then
            res = tonumber(res) + 1
        end
    elseif tonumber(index) > 2 then
        status = "over_index"
    end
    return res, status
end

local function i2c_handler(mibObj, index)
    local jsonInfo
    local res = ""
    local name = ""
    local status = "OK"

    --print("mibObj.name = " .. mibObj.name)
    --print("index = " .. index)
    if mibObj.name == "inputState" or
        mibObj.name == "inputType" or
        mibObj.name == "inputAlarm" or
        mibObj.name == "inputIndex"
    then
        --print("index = " .. index)
        --if i2c.sensorList == nil then
        --    print("i2c.inputList == nil")
        --end
        --print("#i2c.sensorList = " .. #i2c.sensorList)
        if tonumber(index) > #i2c.sensorList then
            --print("inputState over_range")
            status = "over_index"
            res = nil
        else
            local sList = i2c.sensorList[tonumber(index)]
            --print("name = " .. sList.name)
            name = sList.name
        end
    elseif mibObj.name == "outStatePSW" then
        if tonumber(index) > 1 then
            --print("inputState over_range")
            status = "over_index"
        else
            name = "relay"
        end
    else
        name = mibObj.name
    end

    --print("name = " .. name)
    --print("status = " .. status)
    if status == "OK" then
        --print("status OK = " .. status)
        local list = {}
        if mibObj.name == "inputIndex" then
            res = index
        elseif mibObj.name == "inputType" or
            mibObj.name == "inputAlarm" or
            mibObj.name == "inputState" or
            mibObj.name == "outStatePSW"
        then
            jsonInfo = i2c.showConfig("all")
            local temp = jsonInfo.values
            list = temp[name]
            if mibObj.name == "inputType" then
                res = list["type"]
            elseif mibObj.name == "inputState" then
                res = list["state"]
            elseif mibObj.name == "inputAlarm" then
                res = list["alarm_state"]
            end
        elseif mibObj.name == "outStatePSW" then
            res = list["state"]

            --print("res key = " .. res)

            if (mibObj.isEnum == "yes") then
                --print("isEnum == yes")
                --print("res 1 = " .. res)
                res = mibObj.enum[res]
                --print("res 2 = " .. res)
            end
        else
            --print("status else = " .. status)
            jsonInfo = tf.getUbusDataByName(mibObj.name)
            res = jsonInfo.hw_sys[mibObj.name]
            --print("RES = " .. res)
            if (mibObj.isZero == "isZero") then
                res = tonumber(res) + 1
            end
        end
    end

    --
    --if (snmp_obj.isZero == "yes") then
    --    --print("isZero == yes")
    --end

    return res, status
end

local function snmp_getPoeStatus(mibObj, portNum)
    --print("snmp_getPoeStatus")
    local poeInfoJson = poe.getPoeStatusInfo()
    local res = ""
    local poePorts = poeInfoJson[poe.poe_enum.ports]
    local count = 0
    local status = "OK"
    count = get_ubusSize(poePorts)

    if tonumber(portNum) > count then
       
        status = "over_index"
        return nil, status
    elseif tonumber(portNum) < 1 then
        status = "zero_index"
        return nil, status
    end

    local portStr = "" .. "lan" .. portNum
    --print("portNum = " .. portNum)
    local poePortInfo = poePorts[portStr]
    if (mibObj.name == "portPoeStatusState") then
        local poePortStatus = poePortInfo[poe.portsEnum.status]
        res = poe.statusEnum[poePortStatus]
    elseif (mibObj.name == "portPoeStatusPower") then
        
        if poePortInfo["consumption"] ~= nil then
            res = poePortInfo["consumption"]
            
        else
            res = ""
        end
    elseif (mibObj.name == "portPoeStatusIndex") then
        res = portNum
    end
    return res, status
end

local function snmp_getPoeState(mibObj, portNum)
    local jsonInfo = {}
    local res      = ""
    local status   = "OK"
    --print("portNum = " .. portNum)
    jsonInfo       = poe.getPoeStateInfo("all")

    local temp     = jsonInfo.values
    local count    = 0
    count          = get_ubusSize(temp) - 2
    --print("count = " .. count)
    --print("portNum = " .. portNum)

    if tonumber(portNum) > count then
        status = "over_index"
        return nil, status
    elseif tonumber(portNum) < 1 then
        status = "zero_index"
        return nil, status
    end

    if mibObj.name == "portPoeIndex" then
        res = portNum
    elseif mibObj.name == "portPoeState" then
        portNum = "lan" .. tostring(portNum)
        local list = temp[portNum]
        res = list.poe
        --print("res = " .. res)
    end

    if mibObj.isEnum == "yes" then
        res = res .. "d"
        res = mibObj.enum[res]
    end

    return res, status
end

local function snmp_setPoeState(mibObj, portNum, setParam)
    local jsonInfo = {}
    local res = ""
    local status = "OK"
    local poeState = 0
    if mibObj.name == "portPoeState" then
        jsonInfo    = poe.getPoeStateInfo("all")
        local temp  = jsonInfo.values
        local count = 0
        count       = get_ubusSize(temp) - 2
        if tonumber(portNum) > count then
            status = "failed"
            return nil, status
        elseif tonumber(portNum) < 1 then
            status = "failed"
            return nil, status
        end

        if setParam.valueType ~= "integer" and setParam.valueType ~= "INTEGER" then
            status = "failed"
            return nil, status
        end
        poeState = setParam.value
        poeState = tonumber(poeState)
        if type(poeState) ~= "number" then
            status = "failed"
            print("value not number")
            return nil, status
        else
            if poeState < 1 or poeState > 2 then
                print("value not corrected")
                return nil, status
            end
        end

        poe.managePoE(portNum, setParam.value)
        res = tostring(poeState)
    end
    return res, status
end

local function snmp_setOutPutState(mib_obj, index, setParam)
    if mib_obj.name == "relay" then
        local i2c_relaysetValue = "tf_hwsys_ctrl relay set" .. " " .. setParam.value
        if tonumber(setParam.value) == 0 or tonumber(setParam.value) == 1 then
            if setParam.valueType == "integer" or setParam.valueType == "INTEGER" then
                tf.executeCommand(i2c_relaysetValue)
            end
        end
    end
end

local function snmp_getARconfig(mibObj, portNum)
    local jsonInfo  = {}
    local resConfig = ""
    local status    = "OK"
    
    jsonInfo        = ar.showConfig("all")
    local temp      = jsonInfo.values

    local count     = 0
    count           = get_ubusSize(temp)
    if tonumber(portNum) > count - 2 then
        status = "over_index"
        return nil, status
    elseif tonumber(portNum) < 1 then
        status = "zero_index"
        return nil, status
    end
    local portNum_str = "lan" .. portNum
    local paramTable  = ar.configEnum[mibObj.name]
    local paramName   = paramTable[1]
    local key         = ""
    if mibObj.name ~= "autoRstIndex" then
        --print("paramName = " .. paramName)
        local list = temp[portNum_str]
        if list[paramName] == nil then
            if mibObj.name == "autoReStartTimeOnHour" or mibObj.name == "autoReStartTimeOffHour" or
                mibObj.name == "autoReStartTimeOnMin" or mibObj.name == "autoReStartTimeOffMin" then
                key = "25:61"
            elseif mibObj.name ~= "autoRstDstIP" then
                key = "0"
            elseif mibObj.name == "autoRstDstIP" then
                key = "0.0.0.0"
            end
        else
            key = list[paramName]
        end

        --print("type(temp) = " .. type(key))
        --print("res = " .. key)
        if mibObj.isEnum == "yes" then
            resConfig = mibObj.enum[key]
            --print("resConfig = " .. resConfig)
        else
            resConfig = key
        end
    end

    if mibObj.name == "autoRstIndex" then
        resConfig = portNum
    elseif mibObj.name == "autoReStartTimeOnHour" or mibObj.name == "autoReStartTimeOffHour" then
        local time = tf.get_hour_minutes(resConfig)
        if (time[1] ~= nil) then
            resConfig = time[1]
        else
            resConfig = 255
        end
    elseif mibObj.name == "autoReStartTimeOnMin" or mibObj.name == "autoReStartTimeOffMin" then
        local time = tf.get_hour_minutes(resConfig)
        if (time[2] ~= nil) then
            resConfig = time[2]
        else
            resConfig = 255
        end
    end
    return resConfig, status
end

local function snmp_getARerrorCode(mibObj, portNum)
    local jsonInfo    = {}
    local arPoeStatus = {}
    local testType    = 0
    local status      = "OK"
    portNum           = tonumber(portNum)
    jsonInfo          = ar.showStatus()

    local count       = 0
    count             = get_ubusSize(jsonInfo.port)
   
    if tonumber(portNum) > count then
        status = "over_index"
        return nil, status
    elseif tonumber(portNum) < 1 then
        status = "zero_index"
        return nil, status
    end

    arPoeStatus = jsonInfo.port[portNum]
    local res = ""
    --print("--------------------------------------------")
    if mibObj.name == "arPortIndex" then
        res = portNum
    end
    testType = tonumber(arPoeStatus.test_type) + 1
    local error_Code_in = arPoeStatus.error_Code
    local error_Code_local = tonumber(ar.errorCode[testType][2])
    local error_Code_link = tonumber(ar.errorCode[2][2])
    local error_Code_OK = tonumber(ar.errorCode[6][2])

    --print("test_type  =         " .. testType)
    --print("errorCode[testType]  " .. errorCode[testType][2])
    --print("error_Code_in        " .. error_Code_in)
    --print("logic " .. tostring(bit.band(error_Code_in, error_Code_local)))
    --print("error_Code_link = " .. error_Code_link)
    --print("logic  test link " .. tostring(bit.band(error_Code_in, error_Code_link)))

    if (tonumber(bit.band(error_Code_in, error_Code_local)) == error_Code_local) then
        --print("id         = " .. arPoeStatus[arInfo.id])
        --print("error_Code = " .. arPoeStatus[arInfo.error_Code])
        --print("if 1")
        res = tostring(ar.errorCode[testType][3])
    elseif (tonumber(bit.band(error_Code_in, error_Code_link)) == error_Code_link) then
        --rint("id         = " .. arPoeStatus[arInfo.id])
        --rint("error_Code = " .. arPoeStatus[arInfo.error_Code])
        --rint("if 1")
        res = tostring(ar.errorCode[2][3])
    elseif (tonumber(error_Code_in) == error_Code_OK) then
        --print("if 2")
        res = tostring(ar.errorCode[6][3])
    end
    --print("res = " .. res)
    return res, status
end

local function snmp_getFWVersion(mibObj, index)
    local jsonInfo = {}
    local status = "OK"
    local res = ""
    if tonumber(index) > 1 then
        status = "over_index"
    else
        jsonInfo = tf.collectJsonTable("ubus call system board")
        local list = jsonInfo["release"]
        res = list["version"]
    end
    --print("res = " .. res)
    return res, status
end


function snmp_module.main_handler(mib_obj, index)
    local res = ""
    --print("name = " .. mib_obj.name)
    --print("ubus = " .. mib_obj.ubusType)
    local status = ""
    if mib_obj.ubusType == "i2c" then
        --print("i2c handler")
        res, status = i2c_handler(mib_obj, index)
        --print("res from mib == " .. res)
    elseif mib_obj.ubusType == "poe_status" then
        --print("Get poe status")
        res, status = snmp_getPoeStatus(mib_obj, index)
    elseif mib_obj.ubusType == "poe_config" then
        --print("index = " .. index)
        res, status = snmp_getPoeState(mib_obj, index)
    elseif mib_obj.ubusType == "ar_status" then
        --print("Get auto_restart status")
        res, status = snmp_getARerrorCode(mib_obj, index)
    elseif mib_obj.ubusType == "ar_config" then
        --print("Get auto_restart config")
        --print("name = " .. mib_obj.name .. "  index = " .. index)
        res, status = snmp_getARconfig(mib_obj, index)
    elseif mib_obj.ubusType == "fw" then
        res, status = snmp_getFWVersion(mib_obj, index)
    elseif mib_obj.ubusType == "spf" then
        res, status = snmp_getSPFdata(mib_obj, index)
    else
        res = nil
        status = "failed"
        print("error: ubus type not found")
    end
    --print("status = " .. status)
    return res, status
end

function snmp_module.getRequestType(arg_1)
    local requestType = ""
    if arg_1 == "-n" or arg_1 == "-g" or arg_1 == "-s" then
        requestType = snmp_base.requestType[arg_1]
    else
        requestType = ""
    end
    return requestType
end

function snmp_module.getFirstOidIndex()
    local current_list = snmp_base.main_list
    local count = 0
    for i, row_obj in pairs(current_list) do
        if row_obj[4] == "list" then
            count = i
            break
        end
    end
    return count
end

function snmp_module.get_oidArr(oid)
    local arr = {}
    local in_endIndex = string.len(oid)
    oid = string.sub(oid, 19, in_endIndex)
    --print("oid = " .. oid)
    for num in oid:gmatch("%d+") do
        table.insert(arr, tonumber(num))
        --print("num = " .. num)
    end
    return arr
end

local function tableHandler(oid_arr, row_obj, index)
    local row_table = {}
    local oid = ""
    local table_oid_arr = {}
    local table_oid_index = 0
    local current_oid_index = 0
    local status = "OK"
    row_table = row_obj[3]

    current_oid_index = oid_arr[index]

    oid = row_table[1]

    if type(oid) == "string" then
        table_oid_arr = snmp_module.get_oidArr(oid)
        table_oid_index = table_oid_arr[index]
        if current_oid_index == table_oid_index then
            --print("table_oid_index = " .. table_oid_index)
            --print("oid = " .. oid .. "   name = " .. row_table[2] .. "  type = " .. row_table[4])
        else
            --print("1 --- table index not valid")
            status = "failed"
        end
    else
        for i, j in pairs(oid) do
            if type(j) ~= "table" then
                --print("key = " .. i .. "    value = " .. j)
            end
        end
        print("ERROR oid is a table")
    end
    return row_table, status
end

function snmp_module.get_OidElement(oid_arr)
    local inputOid_arr = oid_arr

    local row_obj = {}
    local obj_arr = {}
    local current_list = snmp_base.main_list
    local current_list_len = #current_list
    local current_oid_index = 0
    local index = 1
    local status = "found"
    while index <= #oid_arr do
        --print(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>")
        current_oid_index = oid_arr[index]
        current_list_len = #current_list
        --print("index = " .. index .. "\t current_oid_index = " .. current_oid_index .. "\t current_list_len = " .. current_list_len)

        if current_oid_index > #current_list then
            status = "oid_over_list"
            break
        elseif current_oid_index == 0 then
            obj_arr[index] = nil
            status = "zero_index"
            break
        else
            row_obj = current_list[current_oid_index]
            --print("row_obj --- >>> oid = " .. row_obj[1] .. "   name = " .. row_obj[2] .. "  type = " .. row_obj[4])
            obj_arr[index] = row_obj
            if row_obj[4] == "not_defined" then
                obj_arr[index] = row_obj
                status = "found"
                break
            elseif row_obj[4] == "table" then
                --print("*** found table ***")
                local row_table = {}
                index = index + 1
                --print("1 --- index = " .. index)

                if index <= #oid_arr then
                    row_table, status = tableHandler(oid_arr, row_obj, index)
                else
                    status = "found"
                    break
                end

                if status == "failed" then
                    --print("2 --- table index not valid")
                    break;
                else
                    obj_arr[index] = row_table
                    index = index + 1
                    --print("2 --- index = " .. index)
                    if index <= #oid_arr then
                        --print("*** found entry ***")
                        row_table, status = tableHandler(oid_arr, row_table, index)
                    else
                        status = "found"
                        break
                    end

                    if status == "failed" then
                        --print("3 -- entry index not valid")
                        break;
                    else
                        -- print("*** found row ***")
                        status = "found"
                        current_list = row_table[3]
                        obj_arr[index] = row_table
                    end
                end
            elseif row_obj[4] == "scalar" then
                --status = "found"
                --print("*** found scalar *** ")
            elseif row_obj[4] == "column" then
                --print("*** found column *** ")
                local oid = row_obj[1]
                local column_oid_arr = snmp_module.get_oidArr(oid)
                --print("#oid_arr = " .. #oid_arr .. "\t #column_oid_arr = " .. #column_oid_arr)
                local delta = #oid_arr - #column_oid_arr
                if delta == 0 then
                    status = "found"
                elseif delta == 1 then
                    local data = {}
                    status = "found"
                    --print("column index = " .. index)
                    index = index + 1
                    data[1] = oid_arr[index]
                    data[2] = row_obj[2]
                    data[3] = ""
                    data[4] = "index"
                    obj_arr[index] = data
                elseif delta > 1 then
                    status = "oid_over"
                end
                break
            elseif row_obj[4] == "list" then
                --print("LIST index = " .. index .. "  len = " .. #oid_arr)
                current_list = row_obj[3]
            end
        end
        index = index + 1
    end
    return obj_arr, status
end

local nodetypeEnum = {
    table       = 1,
    entry       = 2,
    row         = 3,
    column      = 4,
    scalar      = 5,
    empty       = 6,
    not_defined = 7,
}

local function getResult(oid, index)
    local status = ""
    local result = ""
    local mib_obj = snmp_module.getMibObj(oid)
    if mib_obj == nil then
        print("mib_obj == nil")
        status = "in_progress"
    else
        if mib_obj.nodetype == "column" or mib_obj.nodetype == "scalar" then
            result, status = snmp_module.main_handler(mib_obj, index)
        else
            status = "in_progress"
        end
    end
    return result, status, mib_obj
end

local function setValue(oid, index, setParam)
    local status = ""
    local result = ""
    local res = ""
    local mib_obj = snmp_module.getMibObj(oid)
    if mib_obj.nodetype == "column" or mib_obj.nodetype == "scalar" then
        if mib_obj.ubusType == "poe_config" then
            result, status = snmp_setPoeState(mib_obj, index, setParam)
        elseif mib_obj.ubusType == "i2c" then
            result, status = snmp_setOutPutState(mib_obj, index, setParam)
        end
    else
        status = "failed"
    end

    return result, status, mib_obj
end

local function getListResult(oid, index)
    local status = ""
    local result = ""
    local mib_obj = snmp_module.getMibObj(oid)
    if mib_obj == nil then
        print("mib_obj == nil")
        status = "in_progress"
    else
        if mib_obj.nodetype == "scalar" then
            result, status = snmp_module.main_handler(mib_obj, index)
        else
            status = "in_progress"
        end
    end
    return result, status, mib_obj
end

local function splitNextOid(oid_arr)
    local next_oid = ""
    local next_oid_arr = oid_arr
    local last_oid_index = oid_arr[#oid_arr]
    local base_oid = "1.3.6.1.4.1.42019"
    next_oid_arr[#next_oid_arr] = last_oid_index + 1
    --print("next_oid_arr[#next_oid_arr] =  " .. next_oid_arr[#next_oid_arr])
    next_oid = base_oid
    for i = 1, #next_oid_arr do
        next_oid = next_oid .. "." .. next_oid_arr[i]
    end
    --print("splitNextOid next_oid = " .. next_oid)
    return next_oid
end

function snmp_module.SET_requestHandler(obj_arr, setParam)
    local obj_arr_len = #obj_arr
    local last_element = obj_arr[obj_arr_len]

    local result = ""
    local status = "failed"
    local obj = {}

    local oid = ""

    local res_obj = {
        status = "",
        oid    = "",
        mib_obj  = {},
        res    = ""
    }
    if last_element[4] == "index" then

        local prev_element = obj_arr[obj_arr_len - 1]
        local index = last_element[1]
        oid = prev_element[1]
        result, status, obj = setValue(oid, index, setParam)
        res_obj.oid    = oid .. "." .. index
        res_obj.status = status
        res_obj.res    = result
        res_obj.mib_obj  = obj
    elseif last_element[4] == "scalar" then
        result, status, obj = setValue(last_element[1], 0, setParam)
    else
        res_obj.oid    = oid
        res_obj.status = "failed"
        res_obj.res    = ""
    end
    return res_obj
end

function snmp_module.GET_requestHandler(obj_arr)
    --print("GET")
    local obj_arr_len = #obj_arr
    local last_element = obj_arr[obj_arr_len]
    local oid = ""
    local result = ""
    local status = "failed"
    local obj = {}
    local res_obj = {
        status = "",
        oid = "",
        mib_obj = {},
        res = ""
    }
    --print("obj_arr len = " .. #obj_arr)
    --for key, value in pairs(obj_arr) do
    --    print(key .. " oid = " .. value[1] .. " name = " .. value[2] .. "\ttype = " .. value[4])
    --end

    if last_element[4] == "index" then
        local prev_element = obj_arr[obj_arr_len - 1]
        local index = last_element[1]
        oid = prev_element[1]
        --print("oid = " .. oid)
        result, status, obj = getResult(oid, index)
        res_obj.oid = oid .. "." .. index
    elseif last_element[4] == "scalar" then
        oid = last_element[1]
        --print("oid = " .. oid)
        result, status, obj = getResult(oid, 0)
        res_obj.oid = oid
    end
    res_obj.status = status
    res_obj.mib_obj = obj
    res_obj.res = result
    return res_obj
end

function snmp_module.GET_NEXT_requestHandler(obj_arr, input_oid_arr)
    --print("GET_NEXT")
    --print("obj_arr len = " .. #obj_arr)
    --for key, value in pairs(obj_arr) do
    --    print(key .. " oid = " .. value[1] .. " name = " .. value[2] .. "\ttype = " .. value[4])
    --end

    local obj_arr_len = #obj_arr
    local last_element = obj_arr[obj_arr_len]
    local prev_element = obj_arr[obj_arr_len - 1]
    local prev_list = prev_element[3]
    local oid_type = last_element[4]
    local oid = ""
    local oid_arr = {}

    local obj = {}
    local res_obj = {
        status = "",
        oid = "",
        mib_obj = {},
        res = ""
    }
    local result = ""
    local status = "OK"

    --print("oid_type == " .. oid_type)

    if oid_type == "not_defined" then
        oid = last_element[1]
        oid_arr = snmp_module.get_oidArr(oid)
        local last_oid_index = tonumber(oid_arr[#oid_arr])
        --print("#prev_list = " .. #prev_list)
        --print("#last_oid_index = " .. last_oid_index)
        local next_oid = ""
        if last_oid_index <= #prev_list then
            next_oid = splitNextOid(oid_arr)
            status = "in_progress"
            res_obj.oid = next_oid
        elseif last_oid_index > #prev_list then
            status = "oid_over_list"
        end
    elseif oid_type == "list" then
        oid = last_element[1]
        res_obj.oid = oid .. "." .. "1"
        status = "in_progress"
    elseif oid_type == "table" then
        oid = last_element[1]
        res_obj.oid = oid .. "." .. "1"
        status = "in_progress"
    elseif oid_type == "entry" then
        oid = last_element[1]
        res_obj.oid = oid .. "." .. "1"
        status = "in_progress"
    elseif oid_type == "row" then
        oid = last_element[1]
        res_obj.oid = oid .. "." .. "1"
        status = "in_progress"
    elseif oid_type == "column" then
        --print("oid_type == column")
        oid = last_element[1]
        res_obj.oid = oid .. "." .. "1"
        --print("oid_type ==  column --  oid = " .. res_obj.oid)
        status = "found"
        result, status, obj = getResult(oid, "1")
    elseif oid_type == "index" then
        --print("oid_type == index")
        local index = tonumber(last_element[1]) + 1
        oid = prev_element[1]
        --print("oid = " .. oid)
        result, status, obj = getResult(oid, index)
        --print("status = " .. status)
        res_obj.oid = oid .. "." .. index
    elseif oid_type == "scalar" then
        prev_list = prev_element[3]
        --print("prev_list len = " .. #prev_list)
        oid = last_element[1]
        --print("oid = " .. oid)
        oid_arr = snmp_module.get_oidArr(oid)
        local next_oid = oid .. "." .. "0"
        result, status, obj = getResult(oid, 0)
        res_obj.oid = next_oid
    end
    res_obj.res = result
    res_obj.mib_obj = obj
    res_obj.status = status
    return res_obj
end

function snmp_module.getMibObj(oid)
    return snmp_base.mib_list[oid]
end

return snmp_module