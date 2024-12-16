module("luci.controller.special.teleport",package.seeall)

function index()
  entry({"admin","special"},firstchild(),translate("Special"),60).dependent=false
  entry({"admin", "special", "teleport"},cbi("special/cbi_teleport"),translate("Teleport"),30)
end