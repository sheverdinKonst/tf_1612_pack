'use strict';
'require view';
'require dom';
'require fs';
'require ui';
'require uci';
'require rpc';
'require form';
'require tools.widgets as widgets';

let hash = [];

var callRename = rpc.declare({
	object: 'uci',
	method: 'rename',
	params: [ 'config', 'section','name'],
	expect: { result: [] }
});

var callCreateUser = rpc.declare({
	object: 'tf_system',
	method: 'createUser',
	params: [ 'username', 'password','level']
});

var callDeleteUser = rpc.declare({
	object: 'tf_system',
	method: 'deleteUser',
	params: [ 'username']
});

var callGenerateHash = rpc.declare({
	object: 'tf_system',
	method: 'generateHash',
	params: [ 'username', 'password','level']
});


function generateHash(user, pass, level){
	console.log("generateHash");
	callGenerateHash(user,pass,level).then(L.bind(function(data) {
		hash[user] = data.result;
		console.log(data.result);
		//uci.set('rpcd', section_id, 'hash', data.result.toString());

		//uci.set('rpcd', section_id, 'hash', "sdfdsfdsf");

	}, this));
}


/*function callDmHash(opcode,user, pass, level){

	console.log("callDmHash " + opcode + user + pass + level);

	return fs.exec_direct('/usr/bin/dmhash',[opcode,user,pass,level]).then(function(res){
		return res;
	});
}*/

function rename_login_sections(){
	var login_section = uci.sections('rpcd', 'login');
	for(var i = 0; i<login_section.length; i++){
		if(login_section[i]['.anonymous'] && login_section.at(i).username != null){
			L.resolveDefault(callRename("rpcd",login_section[i]['.name'],login_section.at(i).username));
		}
	}
}

var aclList = {};

function globListToRegExp(section_id, option) {
	var list = L.toArray(uci.get('rpcd', section_id, option)),
		positivePatterns = [],
		negativePatterns = [];

	if (option == 'read')
		list.push.apply(list, L.toArray(uci.get('rpcd', section_id, 'write')));

	for (var i = 0; i < list.length; i++) {
		var array, glob;

		if (list[i].match(/^\s*!/)) {
			glob = list[i].replace(/^\s*!/, '').trim();
			array = negativePatterns;
		}
		else {
			glob = list[i].trim(),
				array = positivePatterns;
		}

		array.push(glob.replace(/[.*+?^${}()|[\]\\]/g, function(m) {
			switch (m[0]) {
				case '?':
					return '.';

				case '*':
					return '.*';

				default:
					return '\\' + m[0];
			}
		}));
	}

	return [
		new RegExp('^' + (positivePatterns.length ? '(' + positivePatterns.join('|') + ')' : '') + '$'),
		new RegExp('^' + (negativePatterns.length ? '(' + negativePatterns.join('|') + ')' : '') + '$')
	];
}

var cbiACLLevel = form.DummyValue.extend({
	textvalue: function(section_id) {
		var allowedAclMatches = globListToRegExp(section_id, this.option.match(/read/) ? 'read' : 'write'),
			aclGroupNames = Object.keys(aclList),
			matchingGroupNames = [];

		for (var j = 0; j < aclGroupNames.length; j++)
			if (allowedAclMatches[0].test(aclGroupNames[j]) && !allowedAclMatches[1].test(aclGroupNames[j]))
				matchingGroupNames.push(aclGroupNames[j]);

		if (matchingGroupNames.length == aclGroupNames.length)
			return E('span', { 'class': 'label' }, [ _('full', 'All permissions granted') ]);
		else if (matchingGroupNames.length > 0)
			return E('span', { 'class': 'label' }, [ _('partial (%d/%d)', 'Some permissions granted').format(matchingGroupNames.length, aclGroupNames.length) ]);
		else
			return E('span', { 'class': 'label warning' }, [ _('denied', 'No permissions granted') ]);
	}
});

var cbiACLSelect = form.Value.extend({
	renderWidget: function(section_id) {
		var readMatches = globListToRegExp(section_id, 'read'),
			writeMatches = globListToRegExp(section_id, 'write');

		var table = E('table', { 'class': 'table' }, [
			E('tr', { 'class': 'tr' }, [
				E('th', { 'class': 'th' }, [ _('ACL group') ]),
				E('th', { 'class': 'th' }, [ _('Description') ]),
				E('th', { 'class': 'th' }, [ _('Access level') ])
			]),
			E('tr', { 'class': 'tr' }, [
				E('td', { 'class': 'td' }, [ '' ]),
				E('td', { 'class': 'td' }, [ '' ]),
				E('td', { 'class': 'td' }, [
					_('Set all: ', 'Set all permissions in the table below to one of the given values'),
					E('a', { 'href': '#', 'click': function() {
							table.querySelectorAll('select').forEach(function(select) { select.value = select.options[0].value });
						} }, [ _('denied', 'No permissions granted') ]), ' | ',
					E('a', { 'href': '#', 'click': function() {
							table.querySelectorAll('select').forEach(function(select) { select.value = 'read' });
						} }, [ _('readonly', 'Only read permissions granted') ]), ' | ',
					E('a', { 'href': '#', 'click': function() {
							table.querySelectorAll('select').forEach(function(select) { select.value = 'write' });
						} }, [ _('full', 'All permissions granted') ]),
				])
			])
		]);

		Object.keys(aclList).sort().forEach(function(aclGroupName) {
			var isRequired = (aclGroupName == 'unauthenticated' || aclGroupName == 'luci-base'),
				isReadable = (readMatches[0].test(aclGroupName) && !readMatches[1].test(aclGroupName)) || null,
				isWritable = (writeMatches[0].test(aclGroupName) && !writeMatches[1].test(aclGroupName)) || null;

			table.appendChild(E('tr', { 'class': 'tr' }, [
				E('td', { 'class': 'td' }, [ aclGroupName ]),
				E('td', { 'class': 'td' }, [ aclList[aclGroupName].description || '-' ]),
				E('td', { 'class': 'td' }, [
					E('select', { 'data-acl-group': aclGroupName }, [
						isRequired ? E([]) : E('option', { 'value': '' }, [ _('denied', 'No permissions granted') ]),
						E('option', { 'value': 'read', 'selected': isReadable }, [ _('readonly', 'Only read permissions granted') ]),
						E('option', { 'value': 'write', 'selected': isWritable }, [ _('full', 'All permissions granted') ])
					])
				])
			]));
		});

		return table;
	},

	formvalue: function(section_id) {
		var node = this.map.findElement('data-field', this.cbid(section_id)),
			data = {};

		node.querySelectorAll('[data-acl-group]').forEach(function(select) {
			var aclGroupName = select.getAttribute('data-acl-group'),
				value = select.value;

			if (!value)
				return;

			switch (value) {
				case 'write':
					data.write = data.write || [];
					data.write.push(aclGroupName);
				/* fall through */

				case 'read':
					data.read = data.read || [];
					data.read.push(aclGroupName);
					break;
			}
		});

		return data;
	},

	write: function(section_id, value) {
		uci.unset('rpcd', section_id, 'read');
		uci.unset('rpcd', section_id, 'write');

		if (L.isObject(value) && Array.isArray(value.read))
			uci.set('rpcd', section_id, 'read', value.read);

		if (L.isObject(value) && Array.isArray(value.write))
			uci.set('rpcd', section_id, 'write', value.write);
	}
});

return view.extend({
	load: function() {
		uci.load('rpcd');

		return L.resolveDefault(fs.list('/usr/share/rpcd/acl.d'), []).then(function(entries) {
			var tasks = [
				L.resolveDefault(fs.stat('/usr/sbin/uhttpd'), null),
				fs.lines('/etc/passwd')
			];

			for (var i = 0; i < entries.length; i++)
				if (entries[i].type == 'file' && entries[i].name.match(/\.json$/))
					tasks.push(L.resolveDefault(fs.read('/usr/share/rpcd/acl.d/' + entries[i].name).then(JSON.parse)));

			return Promise.all(tasks);
		});
	},


	handleSave: function(ev) {
		var tasks = [];
		document.getElementById('maincontent')
			.querySelectorAll('.cbi-map').forEach(function(map) {
			tasks.push(DOM.callClassMethod(map, 'save'));
		});
		var login_section = uci.sections('rpcd', 'login');
		for(var i = 0; i<login_section.length; i++){
			if(login_section[i]['.anonymous'] && login_section.at(i).username != null){
				L.resolveDefault(callRename("rpcd",login_section[i]['.name'],login_section.at(i).username));

				if(typeof login_section[i].write !== 'undefined'){
					if(login_section[i].write[0] == '*'){
						if(typeof hash[login_section.at(i).username] !== 'undefined')
							uci.set('rpcd', login_section[i]['.name'], 'hash', hash[login_section.at(i).username]);
					}
				}
			}
		}

		return Promise.all(tasks);

	},

	render: function(data) {
		var has_uhttpd = data[0],
			known_unix_users = {};

		for (var i = 0; i < data[1].length; i++) {
			var parts = data[1][i].split(/:/);

			if (parts.length >= 7)
				known_unix_users[parts[0]] = true;
		}

		for (var i = 2; i < data.length; i++) {
			if (!L.isObject(data[i]))
				continue;

			for (var aclName in data[i]) {
				if (!data[i].hasOwnProperty(aclName))
					continue;

				aclList[aclName] = data[i][aclName];
			}
		}

		var m, s, o;

		m = new form.Map('rpcd');

		s = m.section(form.GridSection, 'login',_('User Accounts'),_('User Accounts description'));
		s.anonymous = true;
		s.addremove = true;


		s.handleRemove = function(section_id, ev) {
			callDeleteUser(uci.get('rpcd', section_id, 'username'));
			return form.GridSection.prototype.handleRemove.apply(this, [section_id, ev]);
		};

		s.modaltitle = function(section_id) {
			return _('User Accounts') + ' » ' + (uci.get('rpcd', section_id, 'username') || _('New account'));
		};

		o = s.option(form.Value, 'username', _('Username'));
		o.rmempty = false;
		o.cfgvalue = function(section_id) {
			return uci.get('rpcd', section_id, 'username');
		};
		o.validate = function(section_id, value) {
			if(value != "root")
				return true;
			else
				return "Root username is reserverd";
		}

		o = s.option(form.Value, 'password', _('New Password'));
		o.modalonly = true;
		o.password = true;
		o.rmempty = false;
		o.cfgvalue = function(section_id) {
			return "";
		};
		o.validate = function(section_id, value) {
			return true;
		};
		o.write = function(section_id, value) {
			uci.set('rpcd', section_id, 'password', '$p$'+this.section.formvalue(section_id, 'username'));

			if(typeof this.section.formvalue(section_id, '_level') !== "undefined"){
				if(this.section.formvalue(section_id, '_level') == "write"){
					callCreateUser(this.section.formvalue(section_id, 'username'),value,'full').then(L.bind(function(data) {
						if(typeof data.result !== 'undefined'){
							if(data.result != "ok")
								ui.addNotification(null, E('p', _('Error: '+data.result)), 'danger');
						}
					}, this));


					generateHash(this.section.formvalue(section_id, 'username'),value,'full');
				}
				else
					//callDmHash("-a",this.section.formvalue(section_id, 'username'),value);
					callCreateUser(this.section.formvalue(section_id, 'username'),value);
			}
		};

		o = s.option(form.Value, 'password_confirm', _('Password confirmation'));
		o.modalonly = true;
		o.password = true;
		o.rmempty = false;
		o.cfgvalue = function(section_id) {
			return "";
		};
		o.validate = function(section_id, value) {
			if(this.section.formvalue(section_id, 'password') != value){
				return _('Password mismatch');
			}
			return true;
		};
		o.write = function() {};


		o = s.option(form.Value, 'timeout', _('Session timeout'));
		o.default = '300';
		o.datatype = 'uinteger';
		o.textvalue = function(section_id) {
			var value = uci.get('rpcd', section_id, 'timeout') || this.default;
			return +value ? '%ds'.format(value) : E('em', [ _('does not expire') ]);
		};

		o = s.option(cbiACLLevel, '_read', _('Read access'));
		o.modalonly = false;

		o = s.option(cbiACLLevel, '_write', _('Write access'));
		o.modalonly = false;

		o = s.option(form.ListValue, '_level', _('Access level'));
		o.modalonly = true;
		o.value('write', _('full', 'All permissions granted'));
		o.value('read', _('readonly', 'Only read permissions granted'));
		o.value('individual', _('individual', 'Select individual permissions manually'));

		o.cfgvalue = function(section_id) {
			var readList = L.toArray(uci.get('rpcd', section_id, 'read')),
				writeList = L.toArray(uci.get('rpcd', section_id, 'write'));

			//if(writeList.length && readList.length){
			if (writeList.length == 1 && writeList[0] == '*')
				return 'write';
			else if (readList.length == 1 && readList[0] == '*')
				return 'read';
			else
				return 'individual';
			//}
			//else{
			//	return 'write';
			//}
		};
		o.write = function(section_id) {
			switch (this.formvalue(section_id)) {
				case 'write':
					uci.set('rpcd', section_id, 'read', ['*']);
					uci.set('rpcd', section_id, 'write', ['*']);
					break;

				case 'read':
					uci.set('rpcd', section_id, 'read', ['*']);
					uci.unset('rpcd', section_id, 'write');
					break;
			}
		};
		o.remove = function() {};

		o = s.option(cbiACLSelect, '_acl');
		o.modalonly = true;
		o.depends('_level', 'individual');

		return m.render();
	}
});
