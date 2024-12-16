'use strict';
'require form';
'require fs';
'require view';
'require rpc';
'require poll';
'require uci';
'require ui';

var callSfpStatus = rpc.declare({
	object: 'hw_sensor',
	method: 'getStatus',
	params: ['category'],
	expect: { hw_sys: {} }
});

var sfp1 = E('table', { 'class': 'table' }, [
	E('tr', { 'class': 'tr table-titles' }, [
		E('th', { 'class': 'th col-2 left' }, _('Name')),
		E('th', { 'class': 'th col-10 left' }, _('Value'))
	])
]);
var sfp2 = E('table', { 'class': 'table' }, [
	E('tr', { 'class': 'tr table-titles' }, [
		E('th', { 'class': 'th col-2 left' }, _('Name')),
		E('th', { 'class': 'th col-10 left' }, _('Value'))
	])
]);

function updateTable(table, sfp, port) {

	console.log(sfp);


	var rows = [];


	if(port == 0){
		rows.push([_("Present"),sfp.portSfpPresent_1]);
		rows.push([_("Los"),sfp.portSfpSignalDetect_1]);
		rows.push([_("Vendor"),sfp.portSfpVendor_1]);
		rows.push([_("Vendor OUI"),sfp.portSfpOui_1]);
		rows.push([_("Vendor PN"),sfp.portSfpPartNumber_1]);
		rows.push([_("Vendor REV"),sfp.portSfpRevision_1]);
		rows.push([_("Identifier"),sfp.SFP1_IDENTIFIER]);
		rows.push([_("Connector"),sfp.SFP1_CONNECTOR]);
		rows.push([_("Type"),sfp.SFP1_TYPE]);
		rows.push([_("Link length"),sfp.SFP1_LINK_LEN]);
		rows.push([_("Fiber technology"),sfp.SFP1_FIBER_TEC]);
		rows.push([_("Media"),sfp.SFP1_MEDIA]);
		rows.push([_("Speed"),sfp.SFP1_SPEED]);
		rows.push([_("Encoding"),sfp.SFP1_ENCODING]);
		rows.push([_("Wave length"),sfp.SFP1_WAVELEN]);
		rows.push([_("NBR"),sfp.SFP1_NBR]);
		rows.push([_("len9"),sfp.SFP1_LEN9]);
		rows.push([_("len50"),sfp.SFP1_LEN50]);
		rows.push([_("len62"),sfp.SFP1_LEN62]);
		rows.push([_("lenc"),sfp.SFP1_LENC]);
		rows.push([_("Temperature"),sfp.portSfpTemperature_1]);
		rows.push([_("Voltage"),sfp.portSfpVoltage_1]);
		rows.push([_("Current"),sfp.SFP1_CURRENT]);
		rows.push([_("TX Bias"),sfp.portSfpBiasCurrent_1]);
		rows.push([_("TX Power (Db)"),sfp.portSfpTxOutPowerDb_1]);
		rows.push([_("TX Power (mW)"),sfp.portSfpTxOutPower_1]);
		rows.push([_("RX Power (Db)"),sfp.portSfpRxOutPowerDb_1]);
		rows.push([_("RX Power (mW)"),sfp.portSfpRxOutPower_1]);
	}
	if(port == 1){
		rows.push([_("Present"),sfp.portSfpPresent_2]);
		rows.push([_("Los"),sfp.portSfpSignalDetect_2]);
		rows.push([_("Vendor"),sfp.portSfpVendor_2]);
		rows.push([_("Vendor OUI"),sfp.portSfpOui_2]);
		rows.push([_("Vendor PN"),sfp.portSfpPartNumber_2]);
		rows.push([_("Vendor REV"),sfp.portSfpRevision_2]);
		rows.push([_("Identifier"),sfp.SFP2_IDENTIFIER]);
		rows.push([_("Connector"),sfp.SFP_CONNECTOR]);
		rows.push([_("Type"),sfp.SFP2_TYPE]);
		rows.push([_("Link length"),sfp.SFP2_LINK_LEN]);
		rows.push([_("Fiber technology"),sfp.SFP2_FIBER_TEC]);
		rows.push([_("Media"),sfp.SFP2_MEDIA]);
		rows.push([_("Speed"),sfp.SFP2_SPEED]);
		rows.push([_("Encoding"),sfp.SFP2_ENCODING]);
		rows.push([_("Wave length"),sfp.SFP2_WAVELEN]);
		rows.push([_("NBR"),sfp.SFP2_NBR]);
		rows.push([_("len9"),sfp.SFP2_LEN9]);
		rows.push([_("len50"),sfp.SFP2_LEN50]);
		rows.push([_("len62"),sfp.SFP2_LEN62]);
		rows.push([_("lenc"),sfp.SFP2_LENC]);
		rows.push([_("Temperature"),sfp.portSfpTemperature_2]);
		rows.push([_("Voltage"),sfp.portSfpVoltage_2]);
		rows.push([_("Current"),sfp.SFP2_CURRENT]);
		rows.push([_("TX Bias"),sfp.portSfpBiasCurrent_2]);
		rows.push([_("TX Power (Db)"),sfp.portSfpTxOutPowerDb_2]);
		rows.push([_("TX Power (mW)"),sfp.portSfpTxOutPower_2]);
		rows.push([_("RX Power (Db)"),sfp.portSfpRxOutPowerDb_2]);
		rows.push([_("RX Power (mW)"),sfp.portSfpRxOutPower_2]);
	}


	cbi_update_table(table, rows, E('em', _('No information available')));
};


return L.view.extend({


	load: function() {
		L.resolveDefault(callSfpStatus("sfp1")).then(function(data) {
			updateTable(sfp1, data,0);
		});
		L.resolveDefault(callSfpStatus("sfp2")).then(function(data) {
			updateTable(sfp2, data,1);
		});
	},





	render: function(data) {
		var view = E('div', {'id':'cbi-network','class':'cbi-map'}, [
			E('div', { 'class': 'cbi-section' },[

				E('h3', _('SFP Statistics')),
				E('div', { 'class': 'cbi-map-descr' }, _('Detailed information about the connected SFP module(DDM, manufactor, type, etc).')),

				E('div', { }, [
					E('div', { 'data-tab': 'init', 'data-tab-title': _('SFP1') }, [
						E('p', {}, _('')),
						sfp1
					]),
					E('div', { 'data-tab': 'init2', 'data-tab-title': _('SFP2') }, [
						E('p', {}, _('')),
						sfp2
					])
				])
			])

		]);
		return view;
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
