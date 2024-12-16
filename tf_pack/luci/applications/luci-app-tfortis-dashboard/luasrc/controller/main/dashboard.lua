module("luci.controller.main.dashboard",package.seeall)

function index()
  entry({"admin"},firstchild(),"dashboard",100).dependent=false
  entry({"admin", "dashboard"}, alias("admin", "dashboard"), translate("dashboard"), 20).acl_depends={ "luci-app-tfortis-dashboard" }
  entry({"admin", "dashboard"},cbi("main/cbi_dashboard"),translate("dashboard"),20)
end
