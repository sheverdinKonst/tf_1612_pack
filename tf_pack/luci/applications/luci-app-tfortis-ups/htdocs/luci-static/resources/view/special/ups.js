'use strict';
'require view';
'require fs';
'require ui';
'require rpc';
'require poll';

var callUpsStatus = rpc.declare({
	object: 'hw_sensor',
	method: 'getStatus',
	params: ['category'],
	expect: { hw_sys: {} }
});

var ups = E('table', { 'class': 'table' }, [
	E('tr', { 'class': 'tr table-titles' }, [
		E('th', { 'class': 'th col-2 left' }, _('Name')),
		E('th', { 'class': 'th col-10 left' }, _('Value'))
	])
]);

function updateTable(table, ups_stat) {
	var rows = [];

	console.log(ups_stat);

	if(ups_stat.upsModeAvalable){
		rows.push([_("UPS subsystem"),_("Ok")]);
		if(ups_stat.upsPwrSource)
			rows.push([_("Power Source"),_("AC")]);
		else
			rows.push([_("Power Source"),_("Battery")]);
		rows.push([_("Battery Voltage"),ups_stat.upsBatteryVoltage+" V"]);
		rows.push([_("Battery Current" ),ups_stat.RPS_BAT_CURRENT+" mA"]);
		if(ups_stat.upsPwrSource == 0)
			rows.push([_("Remaining Battery Time"),ups_stat.upsBatteryTime]);
		rows.push([_("Hw Version"),ups_stat.RPS_HW_VERS]);
		rows.push([_("Sw Version"),ups_stat.RPS_SW_VERS]);
		rows.push([_("Temperature"),ups_stat.upsBatteryTime+" °C"]);
	}
	else
		rows.push([_("UPS subsystem"),_("Not available")]);


	cbi_update_table(table, rows, E('em', _('No information available')));
}


return view.extend({

	load: function() {
		return Promise.all([
			L.resolveDefault(callUpsStatus("ups"), {}),
		]);
	},

	startPolling: function() {
		poll.add(L.bind(function() {
			return L.resolveDefault(callUpsStatus("ups")).then(function(data) {
				updateTable(ups,data)
			});
		}, this),10);
	},

	render: function(data) {
		updateTable(ups, data);
		var view = E('div', {}, [
			E('h2', _('UPS Statistics')),
			E('div', { 'class': 'cbi-map-descr' }, _('Detailed information about the UPS module')),

			E('div', {}, [
				E('p', {}, _('')),
				ups
			])
		]);
		this.startPolling();
		return view;
	},
	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
