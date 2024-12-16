'use strict';
'require form';
'require fs';
'require view';
'require rpc';
'require poll';
'require ui';

function callAutorestartStatus(){
	return fs.exec_direct('/usr/bin/tf_autorestart_ctrl',['status']).then(function(res){
		return res;
	});
}

function callManualReboot(port){
	return fs.exec_direct('/usr/bin/tf_autorestart_ctrl',['reboot',port]).then(function(res){
		return res;
	});
}


var callGetBuiltinEthernetPorts = rpc.declare({
	object: 'luci',
	method: 'getBuiltinEthernetPorts',
	expect: { result: [] }
});

var portlist;


function renderStatus(data) {
	var renderHTML = "";
	var spanTemp = '<span style="color:%s"><strong>%s<br>%s</strong><br>reboots:%s</span>';

	if (data.status == 'OK') {
		renderHTML += String.format(spanTemp, 'green',_("OK"),"",data.reboot_cnt);
	} else if (data.status != 'test disable'){
		renderHTML += String.format(spanTemp, 'red',  _("Fail"),data.status,data.reboot_cnt);
	}

	return renderHTML;
}

function renderStatistics(data) {
	if(data){
		const obj = JSON.parse(data);
		if (typeof  obj !== 'undefined') {
			for (var i = 0; i < portlist.length; i++){
				var view = document.getElementById("status_"+portlist[i].device);
				if(view != null && obj.port[i]!=null)
					view.innerHTML = renderStatus(obj.port[i]);
			}
		}else{
			console.log("renderStatistics: fail to parse json");
		}
	}
	else
		console.log("renderStatistics: empty string");
}


function renderStartupStatistics(data) {
	if (typeof  data !== 'undefined') {
		const obj = JSON.parse(data);
		if (typeof  obj !== 'undefined') {
			if(obj.programm_error != 0x1){
				ui.addNotification(null, E('p', _('Autorestart daemon: '+obj.programm_msg)), 'danger');
			}
		}
	}
}


return L.view.extend({
	/** @private */
	renderPortList: function(data) {
		portlist = data;
	},

	startPolling: function() {
		poll.add(L.bind(function() {
			return L.resolveDefault(callAutorestartStatus()).then(function(data) {
				renderStatistics(data);
			});
		}, this),2);
	},
	load: function() {
		callGetBuiltinEthernetPorts().then(L.bind(function(data) {
			this.renderPortList(data);
		}, this));
		callAutorestartStatus().then(function(data) {
			renderStartupStatistics(data);
		});
	},


	render: function() {
		var  m, s, o;

		m = new form.Map('tf_autorestart', _('Autorestart Settings'),	_('Configuration of autorestart settings'));
		s = m.section(form.GridSection, 'lan', _('Interfaces'));
		s.anonymous = true;
		s.readonly = true;


		o = s.tab('settings', _('Settings'));

		o = s.taboption('settings',form.DummyValue, 'ifn', 'Interface');
		o.cfgvalue = function(section_id) {
			return getPortName(section_id);
		}

		o = s.taboption('settings',form.ListValue, 'state', _('Сriterion'));
		o.value("disable","Disable");
		o.value("link","Link");
		o.value("ping","Ping");
		o.value("speed","Speed");
		o.editable = true;
		o.width = '10%';

		o = s.taboption('settings',form.Value, 'host', _('IP address'));
		o.placeholder = '0.0.0.0';
		o.datatype    = 'ip4addr';
		o.depends("state","ping");
		o.editable = true;


		o = s.taboption('settings',form.Value, 'min_speed', _('Min Speed'));
		o.datatype = 'range(0,1000000)';
		o.depends('state', 'speed');
		o.placeholder = '0 KBps';
		o.editable = true;
		o.validate = function(section_id, value) {
			if(this.section.formvalue(section_id, 'state') == 'speed'){
				if(!(this.section.formvalue(section_id, 'max_speed'))  && (!value)){
					return _('Speed is not set');
				}
				else if(this.section.formvalue(section_id, 'max_speed') && (!value)){
					return true;
				}
				else if(!(this.section.formvalue(section_id, 'max_speed')) && (value)){
					return true;
				}
				else if(this.section.formvalue(section_id, 'max_speed') < value){
					return _('Max Speed < Min Speed');
				}
			}
			return true;
		}

		o = s.taboption('settings',form.Value, 'max_speed', _('Max Speed'));
		o.datatype = 'range(0,1000000)';
		o.depends('state', 'speed');
		o.placeholder = '0 KBps';
		o.editable = true;
		o.validate = function(section_id, value) {
			if(this.section.formvalue(section_id, 'state') == 'speed'){
				if(!(this.section.formvalue(section_id, 'min_speed'))  && (!value)){
					return _('Speed is not set');
				}
				else if(this.section.formvalue(section_id, 'min_speed') && (!value)){
					return true;
				}
				else if(this.section.formvalue(section_id, 'min_speed') > value){
					return _('Min Speed > Max Speed');
				}
			}
			return true;
		}

		o = s.taboption('settings',form.Flag, 'alarm', _('Scheduled reboot'));
		o.enabled = 'enable';
		o.disabled = 'disable';
		o.editable = true;

		o = s.taboption('settings', form.Value, 'timeUp', _('Time Up'));
		o.datatype = 'timehhmm';
		o.depends('alarm', 'enable');
		o.placeholder = '00:00';
		o.editable = true;
		o.validate = function(section_id, value) {
			if(this.section.formvalue(section_id, 'alarm') == 'enable' && (!value))
				return _('Time is not set');
			return true;
		}

		o = s.taboption('settings', form.Value, 'timeDown', _('Time Down'));
		o.datatype = 'timehhmm';
		o.depends('alarm', 'enable');
		o.placeholder = '00:00';
		o.editable = true;
		o.validate = function(section_id, value) {
			if(this.section.formvalue(section_id, 'alarm') == 'enable' && (!value))
				return _('Time is not set');
			return true;
		}

		o = s.taboption('settings', form.DummyValue, 'status', _('Status'));
		o.editable = true;
		o.width = '20%';
		o.modalonly = false;
		o.render = L.bind(function(self, section, scope) {
			return 	E('td', { class: 'td cbi-value-field', id: 'status_'+section}, _('Collecting data ...'));
		}, this);


		o = s.taboption('settings',form.Button, 'reboot', _('Manual PoE Reboot'));
		o.inputstyle = 'action important';
		o.inputtitle = _('Reboot');
		o.editable = true;
		o.modalonly = false;
		o.onclick = L.bind(function(ev, section_id) {
			console.log(section_id.replace('lan', ''));

			return callManualReboot(section_id);
		}, this);


		return m.render().then(L.bind(function(rendered) {
			this.startPolling();
			return rendered;
		}, this));
	}
});
