'use strict';
'require fs';
'require ui';
'require fdb.fdb as logview';

var keyTimeout = null;

const svgExpand =
	'<svg width="18" height="18" viewBox="0 0 438.54 438.54" ' +
	'xmlns="http://www.w3.org/2000/svg">' +
	'<path d="m407.42 159.03c3.62 3.616 7.898 5.428 12.847 5.428 2.282 0 ' +
	'4.668-0.476 7.139-1.429 7.426-3.235 11.136-8.853 11.136-16.846v-127.91c0-' +
	'4.949-1.807-9.231-5.428-12.847-3.61-3.617-7.898-5.424-12.847-5.424h-127.91c-' +
	'7.991 0-13.607 3.805-16.848 11.419-3.23 7.423-1.902 13.99 4 19.698l41.111 ' +
	'41.112-101.35 101.36-101.35-101.36 41.112-41.112c5.901-5.708 7.232-12.275 ' +
	'3.999-19.698-3.239-7.614-8.853-11.421-16.846-11.421h-127.91c-4.952 0-9.235 ' +
	'1.809-12.851 5.426-3.617 3.616-5.424 7.898-5.424 12.847v127.91c0 7.996 ' +
	'3.809 13.61 11.419 16.846 2.285 0.948 4.57 1.429 6.855 1.429 4.948 0 ' +
	'9.229-1.812 12.847-5.427l41.112-41.109 101.35 101.35-101.35 101.35-41.112-' +
	'41.113c-5.711-5.903-12.275-7.231-19.702-4.001-7.614 3.241-11.419 8.856-' +
	'11.419 16.854v127.91c0 4.948 1.807 9.229 5.424 12.847 3.619 3.614 7.902 ' +
	'5.421 12.851 5.421h127.91c7.996 0 13.61-3.806 16.846-11.416 3.234-7.427 ' +
	'1.903-13.99-3.999-19.705l-41.112-41.106 101.35-101.36 101.35 101.36-41.114 ' +
	'41.11c-5.899 5.708-7.228 12.279-3.997 19.698 3.237 7.617 8.856 11.423 ' +
	'16.851 11.423h127.91c4.948 0 9.232-1.813 12.847-5.428 3.613-3.613 ' +
	'5.42-7.898 5.42-12.847v-127.9c0-7.994-3.709-13.613-11.136-16.851-' +
	'7.802-3.23-14.462-1.903-19.985 4.004l-41.106 41.106-101.36-101.35 ' +
	'101.36-101.35 41.11 41.112z"/>' +
	'</svg>';

const svgCompress =
	'<svg width="18" height="18" viewBox="0 0 512 512" ' +
	'xmlns="http://www.w3.org/2000/svg">' +
	'<path d="M200 288H88c-21.4 0-32.1 25.8-17 41l32.9 31-99.2 99.3c-6.2 ' +
	'6.2-6.2 16.4 0 22.6l25.4 25.4c6.2 6.2 16.4 6.2 22.6 0L152 408l31.1 ' +
	'33c15.1 15.1 40.9 4.4 40.9-17V312c0-13.3-10.7-24-24-24zm112-64h112c21.4 0 ' +
	'32.1-25.9 17-41l-33-31 99.3-99.3c6.2-6.2 6.2-16.4 0-22.6L481.9 4.7c-6.2-' +
	'6.2-16.4-6.2-22.6 0L360 104l-31.1-33C313.8 55.9 288 66.6 288 88v112c0 ' +
	'13.3 10.7 24 24 24zm96 136l33-31.1c15.1-15.1 4.4-40.9-17-40.9H312c-13.3 ' +
	'0-24 10.7-24 24v112c0 21.4 25.9 32.1 41 17l31-32.9 99.3 99.3c6.2 6.2 16.4 ' +
	'6.2 22.6 0l25.4-25.4c6.2-6.2 6.2-16.4 0-22.6L408 360zM183 71.1L152 104 52.7 ' +
	'4.7c-6.2-6.2-16.4-6.2-22.6 0L4.7 30.1c-6.2 6.2-6.2 16.4 0 22.6L104 152l-33 ' +
	'31.1C55.9 198.2 66.6 224 88 224h112c13.3 0 24-10.7 24-24V88c0-21.3-25.9-' +
	'32-41-16.9z"/></svg>';

return L.view.extend({
	load: function() {
		return logview.load();
	},

	expandedView: false,
	logs: {},
	controls: {},

	handleDisplayErrors: function(e) {
		if (e.name == 'PermissionError') {
			if (e.message.trim() == 'Exec permission denied') {
				L.notifySessionExpiry();
				return;
			}
		}

		L.ui.addNotification(null, E('p', e.message), 'error');
	},

	updateControls: function(log_name, loaded) {
		if (!this.controls.hasOwnProperty(log_name))
			return;

		var controls = this.controls[log_name];

		/*controls.querySelectorAll('.cbi-button, input[type="checkbox"]').forEach(function(e) {
			e.setAttribute('disabled', 'disabled');
		});*/

		if (loaded) {
			controls.querySelectorAll('.cbi-button, input[type="checkbox"]').forEach(function(e) {
				e.removeAttribute('disabled');
			});
		} else {
			controls.querySelector('.logview-btn-refresh')
				.removeAttribute('disabled');
		};
	},

	handleTabActive: function(ev) {
		var fdb = this.logs[ev.detail.tab];
		var target = ev.target;

		return logview.display(fdb.name, fdb.filter, false).then(L.bind(function(loaded) {
			this.updateControls(fdb.name, loaded);
			this.update(fdb);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	

	handleRefresh: function(fdb, ev) {
		if (fdb.vid_filter) {
			/* Add spinner to priorities filter */
			var node = document.querySelector('div[data-tab="' + fdb.name + '"] div.logview-priorities > div');
			L.dom.content(node, E('em', { 'class': 'spinning' }, _('Loading data…')));
		}

		/* Add spinner to columns filter */
		var node = document.querySelector('div[data-tab="' + fdb.name + '"] div.logview-columns > div');
		L.dom.content(node, E('em', { 'class': 'spinning' }, _('Loading data…')));

		return logview.display(fdb.name, fdb.filter, true).then(L.bind(function(loaded) {
			this.updateControls(fdb.name, loaded);
			this.update(fdb);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	handleFilterKeyUp: function(fdb, ev) {
		if (keyTimeout !== null)
			window.clearTimeout(keyTimeout);

		keyTimeout = window.setTimeout(L.bind(function() {
			fdb.filter = ev.target.value;
			return logview.display(fdb.name, fdb.filter, false).then(L.bind(function() {
				this.update(fdb);
			}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
		}, this), 250);
	},

	handleFilterReset: function(fdb, ev) {
		fdb.filter = '';
		var filterEl = document.querySelector('div[data-tab="' + fdb.name + '"] input[name="filter"]');
		filterEl.value = fdb.filter;
		logview.display(fdb.name, fdb.filter, false).then(L.bind(function() {
			this.update(fdb);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	handleVlanToggle: function(fdb, priority, ev) {
		if (!fdb.vid_filter)
			return;

		if (ev.target.getAttribute('disabled') == 'disabled')
			return;

		var checked = (ev.target.getAttribute('data-hide') === 'true');

		if (checked) {
			fdb.vlan_visible++;
			ev.target.classList.add('cb-active');
			ev.target.setAttribute('data-hide', false);
			logview.setVlanVisible(fdb.name, priority, checked);
		} else {
			/* Uncheck prioritiy */
			if (fdb.vlan_visible == 1) {
				/*
				 * Unchecked last one priority
				 * Enable all priorities
				 */
				fdb.vlan_visible = fdb.vlan_total;

				Object.keys(logview.logPriorities(fdb.name)).forEach(function(p) {
					logview.setVlanVisible(fdb.name, p, true);
				});

				ev.target.parentNode.querySelectorAll('div:not(.cb-active)').forEach(function(e) {
					e.classList.add('cb-active');
					e.setAttribute('data-hide', false);
				});
			} else if (fdb.vlan_visible == fdb.vlan_total) {
				/*
				 * Unchecked priority when all priorities is checked
				 * Enable only clicked priority
				 */
				fdb.vlan_visible = 1;

				ev.target.parentNode.querySelectorAll('.cb-active').forEach(function(e) {
					if (e !== ev.target) {
						e.classList.remove('cb-active');
						e.setAttribute('data-hide', true);
					}
				});

				Object.keys(logview.logPriorities(fdb.name)).forEach(function(p) {
					logview.setVlanVisible(fdb.name, p, (p == priority) ? true : false);
				});

			} else {
				fdb.vlan_visible--;
				ev.target.classList.remove('cb-active');
				ev.target.setAttribute('data-hide', true);
				logview.setVlanVisible(fdb.name, priority, checked);
			}
		}

		logview.display(fdb.name, fdb.filter, false).then(L.bind(function() {
			this.update(fdb);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	handleColumnToggle: function(fdb, column, ev) {
		if (ev.target.getAttribute('disabled') == 'disabled')
			return;

		var checked = (ev.target.getAttribute('data-hide') === 'true')

		if (checked)
			ev.target.classList.add('cb-active');
		else
			ev.target.classList.remove('cb-active');

		ev.target.setAttribute('data-hide', !checked);
		logview.setColumnVisible(fdb.name, column.index, checked);

		if (checked) {
			fdb.columns_visible++;
			if (fdb.columns_visible == 2) {
				var node = ev.target.parentNode.querySelector('div[disabled="disabled"]');
				node.removeAttribute('disabled');
			}
		} else {
			fdb.columns_visible--;
			if (fdb.columns_visible == 1) {
				var node = ev.target.parentNode.querySelector('div.cb-active');
				node.setAttribute('disabled', 'disabled');
			}
		}

		logview.display(fdb.name, fdb.filter, false).then(L.bind(function() {
			this.update(fdb);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},



	update: function(fdb) {
		this.updateColumns(fdb);
		if (fdb.vid_filter)
			this.updatePriorities(fdb);
		this.handleTableHeight();
	},

	updateColumns: function(fdb) {
		var columns = logview.logColumns(fdb.name);
		fdb.columns_visible = 0;

		var logviewColumns = [];
		columns.forEach(L.bind(function(column) {
			if (column.empty)
				return;

			if (column.show)
				fdb.columns_visible++;

			logviewColumns.push(
				E('div', {
					'class': column.show ? 'cb-active' : '',
					'data-hide': column.show ? false : true,
					'click': ui.createHandlerFn(this, 'handleColumnToggle', fdb, column)
				}, [ column.display ])
			);
		}, this));

		/* div[data-tab="..." .logview-priorities > div > ... */
		var node = document.querySelector('div[data-tab="' + fdb.name + '"] div.logview-columns > div');
		L.dom.content(node, logviewColumns);
	},

	updatePriorities: function(fdb) {
		if (!fdb.vid_filter)
			return;

		var priorities = logview.logPriorities(fdb.name);
		var logviewPriorities = [];

		fdb.vlan_visible = 0;
		fdb.vlan_total = 0;

		Object.keys(priorities).forEach(L.bind(function(p) {
			fdb.vlan_total++;
			if (!priorities[p].hide)
				fdb.vlan_visible++;

			logviewPriorities.push(
				E('div', {
					'class': 'lv-p-' + p + (priorities[p].hide ? '' : ' cb-active'),
					'data-hide': priorities[p].hide ? true : false,
					'data-count': priorities[p].count,
					'click': ui.createHandlerFn(this, 'handleVlanToggle', fdb, p)
				}, [ priorities[p].display + ' (%d)'.format(priorities[p].count) ])
			);
		}, this));

		var node = document.querySelector('div[data-tab="' + fdb.name + '"] div.logview-priorities > div');
		L.dom.content(node, logviewPriorities);
	},

	renderTabs: function(container) {
		var log_names = logview.logNames();

		L.dom.content(container, null);

		if (container.hasAttribute('data-initialized')) {
			container.removeAttribute('data-initialized');
			container.parentNode.removeChild(container.previousElementSibling);
		}

		if (log_names.length == 0) {
			container.appendChild(
				E('div', { 'class': 'alert-message warning' }, [
					_('No logs available for display. Probably you do not have enough permissions to view the logs.')
				])
			);

			return;
		}

		for (var i = 0; i < log_names.length; i++) {
			if (!logview.hasPlugin(log_names[i]))
				continue;

			this.logs[log_names[i]] = { name: log_names[i] };
			var fdb = this.logs[log_names[i]];

			var logviewTable = E('div', { 'class': 'logview-table' }, [
				E('p', {}, [
					E('em', { 'class': 'spinning' }, [ _('Loading data…') ])
				])
			]);

			fdb.target = logviewTable;
			fdb.vid_filter = false;
			fdb.filter = '';

			logview.setTarget(fdb.name, fdb.target);

			logview.logColumns(fdb.name).forEach(function(column) {
				if (column.name == 'vid')
					fdb.vid_filter = true;
			});

			if (fdb.vid_filter) {
				fdb.vlan_visible = 0;
				fdb.vlan_total = 0;
			}

			container.appendChild(E('div', {
				'data-tab': log_names[i],
				'data-tab-title': logview.logTitle(fdb.name),
				'cbi-tab-active': L.bind(this.handleTabActive, this)
			}, [
				E('div', { 'class': 'logview-cbfilters-container' }, [
					fdb.vid_filter ? E('div', { 'class': 'cbi-section logview-cbfilter logview-priorities' }, [
						E('label', {}, _('Display VLANs') + ':'),
						E('div', {}, E('em', { 'class': 'spinning' }, _('Loading data…')))
					]) : E('div', {})
				]),
				E('div', { 'class': 'cbi-section logview-filter' }, [
					E('label', {}, _('Entries filter') + ':'),
					E('input', {
						'type': 'text',
						'name': 'filter',
						'placeholder': _('Type to filter…'),
						'value': '',
						'keyup': L.bind(this.handleFilterKeyUp, this, fdb)
					}),
					E('button', {
						'class': 'cbi-button cbi-button-action',
						'click': L.bind(this.handleFilterReset, this, fdb)
					}, [ _('Clear') ])
				]),
				E('div', { 'class': 'cbi-section' }, [
					E('legend', {}, [
						_('fdb Entries'),
						' (',
						E('span', { 'id': 'logview-count-info-' + fdb.name }, '?'),
						')'
					]),
					logviewTable
				])
			]));
		}

		ui.tabs.initTabGroup(container.childNodes);
	},

	handleTableHeight: function() {
		var view   = document.querySelector('div#maincontent');
		var tables = view.querySelectorAll('.logview-table .table-wrapper');

		var windowHeight = window.innerHeight;
		var footerHeight = 0;

		
		var footer = document.querySelector('div.footer');
		if (footer) {
			var footerStyle = footer.currentStyle || window.getComputedStyle(footer);
			footerHeight = parseInt(footerStyle.height, 10) +
						   parseInt(footerStyle.marginTop, 10) +
						   parseInt(footerStyle.marginBottom, 10);
		}
		
		tables.forEach(function(table) {
			var tableRect = table.getBoundingClientRect();
			var padding;

			if (!footerHeight && !padding)
				padding = 16;

			table.style.height = (windowHeight - tableRect.top - padding - footerHeight) + 'px';
		});
	},

	render: function() {
		var logTabs = E('div', { 'data-name': 'fdb' });

		var view = E([], [
			E('div', { 'class': 'logview' }, [
				E('link', { 'rel': 'stylesheet', 'href': L.resource('fdb/fdb.css')}),
				E('h2', {}, [
					_('ARP/MAC Tables')
				]),
				E('div', { 'class': 'cbi-map-descr' }, _('Detailed information about MAC addresses and IP addresses on interfaces.')),
				logTabs
			])
		]);

		window.addEventListener('resize', this.handleTableHeight);
		
		this.renderTabs(logTabs);
		return view;
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
