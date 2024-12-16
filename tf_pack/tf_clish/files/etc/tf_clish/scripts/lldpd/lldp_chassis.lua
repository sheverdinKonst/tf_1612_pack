

local utils = require "lldp_utils"
local chassis_module = {}

function chassis_module.parsingChassis(chassis_s)
    --print(chassis_s[utils.chassis_e.name].value .. ":")
    local id_type  = utils.chassis_e.id_t.type
    local id_value = utils.chassis_e.id_t.value
    local chassis_table = chassis_s.chassis

    for i, chassis_struct in pairs(chassis_table) do
        local id_arr = chassis_struct[utils.chassis_e.id]

        for id_i, id_table in pairs(chassis_struct[utils.chassis_e.id]) do
            print("\tid: " .. "\t\t" .. id_table[id_type] .. "\t" .. (id_table[id_value]))
        end

        for _, struct in pairs(chassis_struct[utils.chassis_e.descr]) do
            print("\tdescription: " .. "\t" .. tostring(struct["value"]))
        end

        local capability_type    = utils.chassis_e.capability_t.type
        local capability_enabled = utils.chassis_e.capability_t.enabled
        print("\t" .. utils.chassis_e.capability .. ":")
        for _, struct in pairs(chassis_struct[utils.chassis_e.capability]) do
            if (struct[capability_enabled] == true) then
                print("\t\t" .. struct[capability_type])
            end
        end
        print("\tmgmt ip: ")
        for _, struct in pairs(chassis_struct[utils.chassis_e.mgmt_ip]) do
            print("\t\t" .. struct["value"])
        end
        print("\tmgmt iface: ")
        for _, struct in pairs(chassis_struct[utils.chassis_e.mgmt_iface]) do
            print("\t\t" .. tostring(struct["value"]))
        end
    end
end

return chassis_module

