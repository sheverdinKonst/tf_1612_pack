
local utils  = require "lldp_utils"
local tf      = require "tf_module"

local s_portCount = 0
local lldp_statistics_module = {}

local statistics_e = {
    name                = "statistics",
    nameif              = "name",
    tx                  = "tx",
    rx                  = "rx",
    rx_discarded_cnt    = "rx_discarded_cnt",
    rx_unrecognized_cnt = "rx_unrecognized_cnt",
    ageout_cnt          = "ageout_cnt",
    insert_cnt          = "insert_cnt",
    delete_cnt          = "delete_cnt",
}

function lldp_statistics_module.parsingStatistics(lldp_statistics)
    --print("---> Statistics")
    s_portCount = s_portCount + 1
    print("--------------------------------")
    print(lldp_statistics[statistics_e.nameif] .. ":")

    for _, value in pairs(lldp_statistics[statistics_e.tx]) do
        print("\tTX" .. ":  \t" .. value[statistics_e.tx])
    end

    for _, value in pairs(lldp_statistics[statistics_e.rx]) do
        print("\tRX" .. ":  \t" .. value[statistics_e.rx])
    end

    for _, value in pairs(lldp_statistics[statistics_e.rx_discarded_cnt]) do
        print("\t\tdiscarded cnt:       " .. "\t" .. value[statistics_e.rx_discarded_cnt])
    end

    for _, value in pairs(lldp_statistics[statistics_e.rx_unrecognized_cnt]) do
        print("\t\tunrecognized cnt:    " .. "\t" .. value[statistics_e.rx_unrecognized_cnt])
    end

    print("\tCounts: ")
    for _, value in pairs(lldp_statistics[statistics_e.ageout_cnt]) do
        print("\t\tageout:              " .. "\t" .. value[statistics_e.ageout_cnt])
    end

    for _, value in pairs(lldp_statistics[statistics_e.insert_cnt]) do
        print("\t\tinsert:              " .. "\t" .. value[statistics_e.insert_cnt])
    end

    for _, value in pairs(lldp_statistics[statistics_e.delete_cnt]) do
        print("\t\tdelete:              " .. "\t" .. value[statistics_e.delete_cnt])
    end

    return s_portCount
end

return lldp_statistics_module

