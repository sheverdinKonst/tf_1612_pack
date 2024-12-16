'use strict';
'require form';
'require fs';
'require view';
'require rpc';
'require poll';
'require uci';


var callGetSensorStatus = rpc.declare({
	object: 'hw_sensor',
	method: 'getStatus',
	params: ['category'],
	expect: { hw_sys: {} }
});


function renderStatus(data) {
	if(data == 1)
		return _("Short");
	else
		return _("Open");
}

function renderStatistics(data) {
	var view;
	if(data){
		const obj = JSON.parse(JSON.stringify(data));
		if (typeof  obj !== 'undefined') {
			//tamper
			view = document.getElementById("status_tamper");
			if(view != null && obj.tamper!=null)
				view.innerHTML = renderStatus(obj.tamper);

			//sensor1
			view = document.getElementById("status_sensor1");
			if(view != null && obj.sensor1!=null)
				view.innerHTML = renderStatus(obj.sensor1);

			//sensor2
			view = document.getElementById("status_sensor2");
			if(view != null && obj.sensor2!=null)
				view.innerHTML = renderStatus(obj.sensor2);

			//relay
			view = document.getElementById("status_relay");
			if(view != null && obj.relay!=null)
				view.innerHTML = renderStatus(obj.relay);


		}else{
			console.log("renderStatistics: fail to parse json");
		}
	}
	else
		console.log("renderStatistics: empty string");
}


return L.view.extend({
	/** @private */
	renderPortList: function(data) {
		portlist = data;
	},

	startPolling: function() {
		poll.add(L.bind(function() {
			return L.resolveDefault(callGetSensorStatus("sensors")).then(function(data) {
				console.log(data);
				renderStatistics(data);
			});
		}, this),10);
	},
	load: function() {
		return L.resolveDefault(callGetSensorStatus("sensors")).then(function(data) {
			renderStatistics(data);
		});
	},


	render: function(data) {
		var  m, s, o;

		m = new form.Map('tfortis_io', _('Inputs/Outputs'),	_('Configuration of build-in inputs/outputs'));
		s = m.section(form.GridSection, 'input', _('Inputs'));
		s.anonymous = false;
		s.readonly = true;


		o = s.tab('settings', _('Settings'));

		o = s.taboption('settings',form.Flag, 'state', _('Activated'));
		o.editable = true;


		o = s.taboption('settings',form.ListValue, 'alarm_state', _('Alarm state'));
		o.value("open",_("Open"));
		o.value("short",_("Short"));
		o.editable = true;


		o = s.taboption('settings', form.DummyValue, 'status', _('Current state'));
		o.editable = true;

		o.modalonly = false;
		o.render = L.bind(function(self, section, scope) {
			return 	E('td', { class: 'td cbi-value-field', id: 'status_'+section}, _('Collecting data ...'));
		}, this);



		s = m.section(form.GridSection, 'output', _('Outputs'));
		s.anonymous = false;
		s.readonly = true;

		o = s.tab('settings', _('Settings'));


		o = s.taboption('settings',form.ListValue, 'state', _('State'));
		o.value("open",_("Open"));
		o.value("short",_("Short"));
		o.editable = true;

		o = s.taboption('settings', form.DummyValue, 'status', _('Current state'));
		o.editable = true;
		o.width = '20%';
		o.modalonly = false;
		o.render = L.bind(function(self, section, scope) {
			return 	E('td', { class: 'td cbi-value-field', id: 'status_'+section}, _('Collecting data ...'));
		}, this);


		return m.render().then(L.bind(function(rendered) {
			this.startPolling();
			return rendered;
		}, this));
	}
});
