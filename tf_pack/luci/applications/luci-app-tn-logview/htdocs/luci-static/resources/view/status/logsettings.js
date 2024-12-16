'use strict';
'require view';
'require uci';
'require form';


return view.extend({
	load: function() {
		return Promise.all([
			uci.load('log')
		]);
	},
	render: function() {
		var  m, s, o;

		m = new form.Map("log",_('Log Settings'),_('Log Settings description'));
		m.tabbed = true;


		s = m.section(form.GridSection, 'rule', _("Rules"), _("This page specifies the rules for sorting logs"));
		s.addremove = true;
		s.anonymous = true;
		s.sortable  = true;
		s.nodescriptions = true;
		s.handleAdd = function(ev) {
			var section_id = null;
			section_id = uci.add('log', 'rule');
			return this.map.save(null, true);
		};

		o = s.option( form.ListValue, 'filter', _('Filter name'), _('Select Filter name'))
		o.editable  = true;
		var filterlist = uci.sections('log', 'filter');
		for (var i = 0; i < filterlist.length; i++){
			o.value(filterlist[i].name);
		}

		o = s.option(form.ListValue, 'action', _('Action'), _('Select the required action'));
		o.editable  = true;
		var actionlist = uci.sections('log', 'action');
		for (var i = 0; i < actionlist.length; i++){
			o.value(actionlist[i].name);
		}



		s = m.section(form.TypedSection, 'filter', _('Filters'), _("This page specifies the filters for sorting logs"));
		s.anonymous = true;
		s.addremove = true;
		o = s.option(form.Value, 'name', _('Name'));

		o = s.option( form.ListValue, 'facility', _('Facility'), _('Select the required Facility, * - any facilities'));
		o.value("*");
		o.value("kern");
		o.value("user");
		o.value("mail");
		o.value("daemon");
		o.value("auth");
		o.value("syslog");
		o.value("lpr");
		o.value("news");
		o.value("uucp");
		o.value("cron");
		o.value("authpriv");
		o.value("ftp");
		o.value("ntp");
		o.value("security");
		o.value("console");
		o.value("local0");
		o.value("local1");
		o.value("local2");
		o.value("local3");
		o.value("local4");
		o.value("local5");
		o.value("local6");
		o.value("local7");

		o = s.option( form.ListValue, 'severity', _('Severity'), _('Select the required Severity level, * - any levels'));
		o.value("*");
		o.value("emerg");
		o.value("alert");
		o.value("crit");
		o.value("err");
		o.value("warning");
		o.value("notice");
		o.value("info");
		o.value("debug");

		o = s.option(form.Value, 'progname', _('Program name'), _('Filter by program name. Empty - no filter'));

		o = s.option(form.Value, 'content', _('Contains'), _('Filter by log message. Empty - no filter'));






		s = m.section(form.TypedSection, "action", _("Actions"), _("This page specifies the actions for sorting logs"));
		s.anonymous = true;
		s.addremove = true;
		o = s.option(form.Value, 'name', _('Name'));
		o = s.option( form.ListValue, 'action', _('Action'));
		o.value("memory","Write to memoty");
		o.value("flash","Write to flash");
		o.value("syslog","Send to syslog");
		o.value("email","Send to email");

		o = s.option(form.DynamicList, 'log_syslog', _('Servers'));
		o.datatype = 'host(0)';
		o.depends("action","syslog");
		o.retain = true;

		o = s.option(form.DynamicList, 'log_email', _('Recipients'));
		o.datatype = 'email';
		o.depends("action","email");
		o.retain = true;


		return m.render();
	}
});
