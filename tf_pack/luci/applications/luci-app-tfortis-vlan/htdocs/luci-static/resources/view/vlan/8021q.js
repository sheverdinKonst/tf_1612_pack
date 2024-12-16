'use strict';
'require view';
'require dom';
'require poll';
'require fs';
'require ui';
'require uci';
'require form';
'require network';
'require firewall';
'require tools.widgets as widgets';
'require tools.network as nettools';
'require rpc';

var callRename = rpc.declare({
	object: 'uci',
	method: 'rename',
	params: ['config','section','name'],
	expect: { result: [] }
});


var cbiTagValue = form.Value.extend({
	renderWidget: function(section_id, option_index, cfgvalue) {
		var widget = new ui.Dropdown(cfgvalue || ['-'], {
			'-': E([], [
				E('span', { 'class': 'hide-open', 'style': 'font-family:monospace' }, [ '—' ]),
				E('span', { 'class': 'hide-close' }, [ _('Not Member', 'VLAN port state') ])
			]),
			'u': E([], [
				E('span', { 'class': 'hide-open', 'style': 'font-family:monospace' }, [ 'U' ]),
				E('span', { 'class': 'hide-close' }, [ _('Untagged', 'VLAN port state') ])
			]),
			't': E([], [
				E('span', { 'class': 'hide-open', 'style': 'font-family:monospace' }, [ 'T' ]),
				E('span', { 'class': 'hide-close' }, [ _('Tagged', 'VLAN port state') ])
			]),
			'*': E([], [
				E('span', { 'class': 'hide-open', 'style': 'font-family:monospace' }, [ '*' ]),
				E('span', { 'class': 'hide-close' }, [ _('Is Primary VLAN', 'VLAN port state') ])
			])
		}, {
			id: this.cbid(section_id),
			sort: [ '-', 'u', 't', '*' ],
			optional: false,
			multiple: true,
			validate: L.bind(this.validate, this, section_id, option_index)
		});

		var field = this;

		widget.toggleItem = function(sb, li, force_state) {
			var lis = li.parentNode.querySelectorAll('li'),
				toggle = ui.Dropdown.prototype.toggleItem;

			toggle.apply(this, [sb, li, force_state]);

			if (force_state != null)
				return;

			switch (li.getAttribute('data-value'))
			{
				case '-':
					if (li.hasAttribute('selected')) {
						for (var i = 0; i < lis.length; i++) {
							switch (lis[i].getAttribute('data-value')) {
								case '-':
									break;

								case '*':
									toggle.apply(this, [sb, lis[i], false]);
									lis[i].setAttribute('unselectable', '');
									break;

								default:
									toggle.apply(this, [sb, lis[i], false]);
							}
						}
					}
					break;

				case 't':
				case 'u':
					if (li.hasAttribute('selected')) {
						for (var i = 0; i < lis.length; i++) {
							switch (lis[i].getAttribute('data-value')) {
								case li.getAttribute('data-value'):
									break;

								case '*':
									lis[i].removeAttribute('unselectable');
									break;

								default:
									toggle.apply(this, [sb, lis[i], false]);
							}
						}
					}
					else {
						toggle.apply(this, [sb, li, true]);
					}
					break;

				case '*':
					if (li.hasAttribute('selected')) {
						var section_ids = field.section.cfgsections();

						for (var i = 0; i < section_ids.length; i++) {
							var other_widget = field.getUIElement(section_ids[i]),
								other_value = L.toArray(other_widget.getValue());

							if (other_widget === this)
								continue;

							var new_value = other_value.filter(function(v) { return v != '*' });

							if (new_value.length == other_value.length)
								continue;

							other_widget.setValue(new_value);
							break;
						}
					}
			}
		};

		var node = widget.render();

		node.style.minWidth = '2em';

		if (cfgvalue == '-')
			node.querySelector('li[data-value="*"]').setAttribute('unselectable', '');

		return E('div', { 'style': 'display:inline-block' }, node);
	},

	cfgvalue: function(section_id) {
		var ports = L.toArray(uci.get('network', section_id, 'ports'));

		for (var i = 0; i < ports.length; i++) {
			var s = ports[i].split(/:/);

			if (s[0] != this.port)
				continue;

			var t = /t/.test(s[1] || '') ? 't' : 'u';

			return /\x2a/.test(s[1] || '') ? [t, '*'] : [t];
		}

		return ['-'];
	},

	write: function(section_id, value) {
		var ports = [];

		for (var i = 0; i < this.section.children.length; i++) {
			var opt = this.section.children[i];

			if (opt.port) {
				var val = L.toArray(opt.formvalue(section_id)).join('');

				switch (val) {
					case '-':
						break;

					case 'u':
						ports.push(opt.port);
						break;

					default:
						ports.push('%s:%s'.format(opt.port, val));
						break;
				}
			}
		}

		//проверяем, что нетегированный порт не встречается в других VLAN
		var section_ids = this.section.cfgsections();
		var changed_ports = L.toArray(ports);
		for (var i = 0; i < section_ids.length; i++) {
			if(section_ids[i] != section_id){
				var other_ports = L.toArray(uci.get('network', section_ids[i], 'ports'));
				for (var j = 0; j < changed_ports.length; j++) { //проходимся по первому масиву
					for (var k = 0; k< other_ports.length; k++) { // ищем соотвествия во втором массиве
						if(changed_ports[j] === other_ports[k] && (changed_ports[j].indexOf(':t')<=0) ){
							ui.addNotification(null, E('p', _('Untagged port '+changed_ports[j]+' is present in other VLAN')), 'danger');
							return false;
						}
					}
				}
			}
		}

		uci.set('network', section_id, 'ports', ports.length ? ports : null);
	},
	validate: function(section_id, value,option_index) {
		return true;
	},
	remove: function() {}
});


function rename_bridge_vlan(){
	var bridge_vlan = uci.sections('network', 'bridge-vlan');
	for(var i = 0; i<bridge_vlan.length; i++){
		if(bridge_vlan[i]['.anonymous'] && bridge_vlan.at(i).vlan != null){
			L.resolveDefault(callRename("network",bridge_vlan[i]['.name'],'vlan'+bridge_vlan.at(i).vlan));
		}
	}
}



//для каждого bridge-vlan создаём device
function device2bridge_vlan_mapping(){
	var bridge_vlan = uci.sections('network', 'bridge-vlan');
	var devices = uci.sections('network', 'device');
	var dev_found;
	var br_found;
	var dev_section_id;



	//пробегаемся по bridge_vlan проверяем и добавляем device
	for(var i = 0; i<bridge_vlan.length; i++){
		//проверяем, что bridge-vlan имеет имя
		dev_found = false;
		for(var k = 0; k<devices.length; k++){
			if(devices[k].type == '8021q' ){
				if(devices[k].vid == bridge_vlan.at(i).vlan){
					dev_found = true;
				}
			}
		}
		if(dev_found == false){
			dev_section_id = uci.add('network', 'device');
			uci.set('network', dev_section_id, 'name', getDefaultSwitch()+'.'+bridge_vlan.at(i).vlan);
			uci.set('network', dev_section_id, 'type', '8021q');
			uci.set('network', dev_section_id, 'ifname', getDefaultSwitch());
			uci.set('network', dev_section_id, 'vid', bridge_vlan.at(i).vlan);
			uci.set('network', dev_section_id, 'macaddr', getDefaultMAC(getDefaultSwitch()+'.'+bridge_vlan.at(i).vlan));
		}




	}
	//удаляем device, которые не встречаются ни в одном bridge_vlan
	for(var k = 0; k<devices.length; k++){
		if(devices[k].type == '8021q' ){
			br_found = false;
			for(var i = 0; i<bridge_vlan.length; i++){
				if(devices[k].vid == bridge_vlan.at(i).vlan){
					br_found = true;
				}
			}
			if(br_found == false){
				uci.remove('network',devices.at(k)['.name']);
			}
		}
	}
}



return view.extend({
	load: function() {
		var script = E('script', { 'type': 'text/javascript' });
		script.src = L.resource('tf_common.js');
		document.querySelector('head').appendChild(script);
		return Promise.all([
			network.getDevice('switch'),
			uci.load('network'),
			network.getDevices()

		]);
	},

	handleSave: function(data) {
		var tasks = [];
		document.getElementById('maincontent')
			.querySelectorAll('.cbi-map').forEach(function(map) {
			tasks.push(DOM.callClassMethod(map, 'save'));
		});
		device2bridge_vlan_mapping();
		return Promise.all(tasks);

	},

	render: function(data) {
		var  m, s, o;
		var ports = data[0].getPorts();
		var interfaces = uci.sections('network', 'interface');
		var devices = uci.sections('network', 'device');
		var dev_section_id;

		m = new form.Map('network', _('802.1Q VLAN Settings'),	_('Configuration of VLAN'));



		s = m.section(form.GridSection, 'interface', _('Managment VLAN'));
		s.anonymous = false;
		s.load = function() {
			rename_bridge_vlan();
			return Promise.all([
				network.getNetworks()
			]).then(L.bind(function(data) {
				this.networks = data[0];
			}, this));
		};
		s.cfgsections = function() {
			return this.networks.map(function(n) { return n.getName() })
				.filter(function(n) {
					if(uci.get('network',n).device != null){
						if(uci.get('network',n).device.indexOf('switch')==0)
							return n;
					}
				});
		};

		o = s.option(form.Value, 'vlan', _('VLAN ID'));
		o.datatype = 'range(1, 4094)';
		o.editable = true;
		o.optional = true;
		o.width = 10;

		o.cfgvalue = function(section_id) {
			for(var k = 0; k<interfaces.length; k++){
				if(interfaces.at(k)['.name'] == section_id){
					var device = interfaces.at(k)['device'];
				}
			}
			for(var i = 0; i<devices.length; i++){
				if(devices[i].type == '8021q' ){
					if(device == devices[i].name)
						return devices[i].vid;
				}
			}
		};
		o.write = function(section_id,value) {
			for(var k = 0; k<interfaces.length; k++){
				if(interfaces.at(k)['.name'] == section_id){
					var device = interfaces.at(k)['device'];
				}
			}
			for(var i = 0; i<devices.length; i++){
				if(devices[i].type == '8021q' ){
					if(device == devices[i].name){
						uci.set('network',devices[i]['.name'], 'vid', value);
					}
				}
			}
		}


		s = m.section(form.GridSection, 'bridge-vlan', _('VLAN List'));
		s.anonymous = true;
		s.tab('settings', _('Settings'));
		s.handleAdd = function(ev) {
			var  max_vlan_id = 0;
			var section_id = null;
			var section_ids = this.cfgsections();

			var sections = uci.sections('network', 'device');
			var device = sections[0].name;

			for (var i = 0; i < section_ids.length; i++) {
				var vid = +uci.get('network', section_ids[i], 'vlan');
				if (vid > max_vlan_id)
					max_vlan_id = vid;
			}

			section_id = uci.add('network', 'bridge-vlan');
			uci.set('network', section_id, 'device', device);
			return this.map.save(null, true);
		};



		o = s.taboption('settings', form.Value, 'vlan', _('VLAN ID'));
		o.datatype = 'range(1, 4094)';
		o.editable = true;
		o.validate = function(section_id, value) {
			var section_ids = this.section.cfgsections();

			for (var i = 0; i < section_ids.length; i++) {
				if (section_ids[i] == section_id)
					continue;

				if (uci.get('network', section_ids[i], 'vlan') == value)
					return _('The VLAN ID must be unique');
			}

			if(!value)
				return _('The VLAN ID must not be empty');
			return true;
		};


		o = s.taboption('settings',form.ListValue, 'state', 'State');
		o.value('enable','enable');
		o.value('disable','disable');
		o.editable = true;


		o = s.taboption('settings', form.Value, 'descr', _('Description'));
		o.editable = true;


		for (var i = 0; i < ports.length; i++) {
			o[i] = s.taboption('settings',cbiTagValue, ports[i].device,getPortName(ports[i].device));
			o[i].port = ports[i].device;
			o[i].editable = true;
		}


		s.addremove = true;
		return m.render();
	}
});
