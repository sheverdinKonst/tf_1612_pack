'use strict';
'require baseclass';
'require fs';
'require rpc';
'require network';
'require uci';

var callLuciVersion = rpc.declare({
	object: 'luci',
	method: 'getVersion'
});

var callSystemBoard = rpc.declare({
	object: 'system',
	method: 'board'
});

var callSystemInfo = rpc.declare({
	object: 'system',
	method: 'info'
});

var callLldp = rpc.declare({
	object: 'luci',
	method: 'getInitList',
	params: [ 'name' ],
	expect: { '': {} },
	filter: function(res) {
		for (var k in res)
			return +res[k].enabled;
		return null;
	}
});

function render_nets(dev){
	var str=null;
	str = dev[0].sid + '(' +dev[0].getIPAddrs()+')';
	for (var i = 1; i < dev.length; i++)
		str += ', ' + dev[i].sid + '(' +dev[i].getIPAddrs()+')';

	return str;
}

function render_vlans(devs){
	var str = null;
	for(var i=0;i<devs.length; i++){
		if(devs[i].config.type == '8021q'){
			if(str == null)
				str = 'VLAN'+devs[i].config.vid;
			else
				str += ', VLAN'+devs[i].config.vid;
		}
	}
	return str;
}

function render_igmp(){
	if(uci.get('network', 'switch', 'igmp_snooping') == '1')
		return _('Enable');
	else
		return _('Disable');
}

function render_lldp(data){
	if(data == '1')
		return _('Enable');
	else
		return _('Disable');
}

function render_autorestart(){
	var str = null;
	var uciInterfaces = uci.sections("tf_autorestart","lan");
	for(var i=0;i<uciInterfaces.length;i++){
		if(uciInterfaces[i].state != 'disable'){
			if(str == null)
				str = getPortName(uciInterfaces[i]['.name']);
			else
				str += ', '+ getPortName(uciInterfaces[i]['.name']);
		}
	}
	return str;
}

function render_smtp(){
	if(uci.get("smtp","client","state") == '1')
		return _('Enable');
	else
		return _('Disable');
}


function render_stp(){
	if(uci.get("mstpd","global","enabled") == '1')
		return _('Enable');
	else
		return _('Disable');
}

function render_syslog(){
	if('1')
		return _('Enable');
	else
		return _('Disable');
}



return baseclass.extend({
	title: _('Enabled protocols'),

	load: function() {

		return Promise.all([
			L.resolveDefault(callSystemBoard(), {}),
			L.resolveDefault(callSystemInfo(), {}),
			L.resolveDefault(callLuciVersion(), { revision: _('unknown version'), branch: 'LuCI' }),
			L.resolveDefault(network.getNetworks(),{}),
			L.resolveDefault(network.getDevices(),{}),
			L.resolveDefault(callLldp("lldpd"),{}),
			uci.load("tf_autorestart"),
			uci.load("smtp"),
			uci.load("mstpd")
		]);
	},

	render: function(data) {
		var boardinfo   = data[0],
			systeminfo  = data[1],
			luciversion = data[2],
			nets = data[3],
			devs = data[4];


		luciversion = luciversion.branch + ' ' + luciversion.revision;

		var datestr = null;

		if (systeminfo.localtime) {
			var date = new Date(systeminfo.localtime * 1000);

			datestr = '%04d-%02d-%02d %02d:%02d:%02d'.format(
				date.getUTCFullYear(),
				date.getUTCMonth() + 1,
				date.getUTCDate(),
				date.getUTCHours(),
				date.getUTCMinutes(),
				date.getUTCSeconds()
			);
		}

		var fields = [
			_('IP interfaces'),         render_nets(nets),
			_('VLAN'),            		render_vlans(devs),
			_('RSTP'),     				render_stp(),
			_('IGMP'),  				render_igmp(),
			_('LLDP'), 					render_lldp(data[5]),
			_('Autorestart'), 			render_autorestart(),
			_('SMTP'),      			render_smtp(),
			_('Syslog'),           		render_syslog()
		];

		var table = E('table', { 'class': 'table' });

		for (var i = 0; i < fields.length; i += 2) {
			table.appendChild(E('tr', { 'class': 'tr' }, [
				E('td', { 'class': 'td left', 'width': '33%' }, [ fields[i] ]),
				E('td', { 'class': 'td left' }, [ (fields[i + 1] != null) ? fields[i + 1] : '?' ])
			]));
		}

		return table;
	}
});
