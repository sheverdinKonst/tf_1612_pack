'use strict';
'require ui';
'require rpc';
'require poll';
'require baseclass';

var callNetworkStatus = rpc.declare({
	object: 'luci-rpc',
	method: 'getNetworkDevices',
	expect: {}
});

var callPoeStatus = rpc.declare({
	object: 'poe',
	method: 'info',
	expect: {}
});

var callGetBuiltinEthernetPorts = rpc.declare({
	object: 'luci',
	method: 'getBuiltinEthernetPorts',
	expect: { result: [] }
});
var poestatus;
var portlist;

return baseclass.extend({
	__init__: function() {
		callGetBuiltinEthernetPorts().then(L.bind(function(data) {
			portlist = data;
		}, this));
		callPoeStatus().then(L.bind(function(data) {
			poestatus = data.ports;
		}, this));
		poll.add(L.bind(this.updatePorts, this), 5);
		poll.add(L.bind(this.updatePoEPorts, this), 5);
	},

	updatePoEPorts: function(){
		return callPoeStatus().then(L.bind(function(data) {
			poestatus = data.ports;
			if(poestatus == null)
				console.log("Error: PoE status not received");
		}, this));
	},

	updatePorts: function() {

		return callNetworkStatus().then(L.bind(function(data) {
			var tfortis_element = document.querySelector('.ports-status-tfortis');

			if(tfortis_element != null) {
				var str = '';
				if(data){
					var obj = JSON.parse(JSON.stringify(data));
					if (typeof  obj !== 'undefined') {
						for (var i = 0; i < portlist.length; i++){
							var link = obj[portlist[i].device];
							var poe = poestatus[portlist[i].device];
							var poeClass = "";

							if(poe != null){
								if(poe.status == "Delivering power")
									poe.up = true;
								else
									poe.up = false;
								poeClass = poe.up ? 'poe' : '';
							}


							if(link.up) {
								str += ('<div class="ports-status-tfortis__element active ' + poeClass + '">');

								str += ('<div class="ports-status-tfortis__name">' + getPortName(portlist[i].device) + '</div>');

								str += ('<div class="ports-status-tfortis__status"></div>');

								str += ('</div>');

							} else {

								str += ('<div class="ports-status-tfortis__element ' + poeClass + '">');

								str += ('<div class="ports-status-tfortis__name">' + getPortName(portlist[i].device) + '</div>');

								str += ('<div class="ports-status-tfortis__status"> <span>  </span> </div>');

								str += ('</div>');

							}

						}
						tfortis_element.innerHTML = str;
					}else{
						console.log("renderPoE: fail to parse json");
					}
				}
			}


		}, this));
	}
});
