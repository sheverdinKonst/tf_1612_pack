'use strict';
'require fs';
'require ui';
'require logview.logview as logview';

var keyTimeout = null;




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

		controls.querySelectorAll('.cbi-button, input[type="checkbox"]').forEach(function(e) {
			e.setAttribute('disabled', 'disabled');
		});

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
		var log = this.logs[ev.detail.tab];
		var target = ev.target;

		return logview.display(log.name, log.filter, false).then(L.bind(function(loaded) {
			this.updateControls(log.name, loaded);
			this.update(log);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	handleDownload: function(log, ev, dlname) {
		return logview.download(log.name, dlname);
	},

	handleSortingToggle: function(log, ev) {
		var checked = ev.target.checked;

		ev.target.setAttribute('disabled', true);
		logview.setSorting(log.name, checked);
		ev.target.removeAttribute('disabled');
	},

	handleRefresh: function(log, ev) {
		if (log.priorities_filter) {
			/* Add spinner to priorities filter */
			var node = document.querySelector('div[data-tab="' + log.name + '"] div.logview-priorities > div');
			L.dom.content(node, E('em', { 'class': 'spinning' }, _('Loading data…')));
		}

		/* Add spinner to columns filter */
		var node = document.querySelector('div[data-tab="' + log.name + '"] div.logview-columns > div');
		L.dom.content(node, E('em', { 'class': 'spinning' }, _('Loading data…')));

		return logview.display(log.name, log.filter, true).then(L.bind(function(loaded) {
			this.updateControls(log.name, loaded);
			this.update(log);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	handleClear: function(log, ev) {

		return logview.clear(log.name);
	},

	handleFilterKeyUp: function(log, ev) {
		if (keyTimeout !== null)
			window.clearTimeout(keyTimeout);

		keyTimeout = window.setTimeout(L.bind(function() {
			log.filter = ev.target.value;
			return logview.display(log.name, log.filter, false).then(L.bind(function() {
				this.update(log);
			}, this)).catch(L.bind(function(e) {
				this.handleDisplayErrors(e);
			}, this));
		}, this), 250);
	},

	handleFilterReset: function(log, ev) {
		log.filter = '';
		var filterEl = document.querySelector('div[data-tab="' + log.name + '"] input[name="filter"]');
		filterEl.value = log.filter;
		logview.display(log.name, log.filter, false).then(L.bind(function() {
			this.update(log);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	handlePriorityToggle: function(log, priority, ev) {
		if (!log.priorities_filter)
			return;

		if (ev.target.getAttribute('disabled') == 'disabled')
			return;

		var checked = (ev.target.getAttribute('data-hide') === 'true');

		if (checked) {
			log.priorities_visible++;
			ev.target.classList.add('cb-active');
			ev.target.setAttribute('data-hide', false);
			logview.setPriorityVisible(log.name, priority, checked);
		} else {
			/* Uncheck prioritiy */
			if (log.priorities_visible == 1) {
				/*
				 * Unchecked last one priority
				 * Enable all priorities
				 */
				log.priorities_visible = log.priorities_total;

				Object.keys(logview.logPriorities(log.name)).forEach(function(p) {
					logview.setPriorityVisible(log.name, p, true);
				});

				ev.target.parentNode.querySelectorAll('div:not(.cb-active)').forEach(function(e) {
					e.classList.add('cb-active');
					e.setAttribute('data-hide', false);
				});
			} else if (log.priorities_visible == log.priorities_total) {
				/*
				 * Unchecked priority when all priorities is checked
				 * Enable only clicked priority
				 */
				log.priorities_visible = 1;

				ev.target.parentNode.querySelectorAll('.cb-active').forEach(function(e) {
					if (e !== ev.target) {
						e.classList.remove('cb-active');
						e.setAttribute('data-hide', true);
					}
				});

				Object.keys(logview.logPriorities(log.name)).forEach(function(p) {
					logview.setPriorityVisible(log.name, p, (p == priority) ? true : false);
				});

			} else {
				log.priorities_visible--;
				ev.target.classList.remove('cb-active');
				ev.target.setAttribute('data-hide', true);
				logview.setPriorityVisible(log.name, priority, checked);
			}
		}

		logview.display(log.name, log.filter, false).then(L.bind(function() {
			this.update(log);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	handleColumnToggle: function(log, column, ev) {
		if (ev.target.getAttribute('disabled') == 'disabled')
			return;

		var checked = (ev.target.getAttribute('data-hide') === 'true')

		if (checked)
			ev.target.classList.add('cb-active');
		else
			ev.target.classList.remove('cb-active');

		ev.target.setAttribute('data-hide', !checked);
		logview.setColumnVisible(log.name, column.index, checked);

		if (checked) {
			log.columns_visible++;
			if (log.columns_visible == 2) {
				var node = ev.target.parentNode.querySelector('div[disabled="disabled"]');
				node.removeAttribute('disabled');
			}
		} else {
			log.columns_visible--;
			if (log.columns_visible == 1) {
				var node = ev.target.parentNode.querySelector('div.cb-active');
				node.setAttribute('disabled', 'disabled');
			}
		}

		logview.display(log.name, log.filter, false).then(L.bind(function() {
			this.update(log);
		}, this)).catch(L.bind(function(e) {
			this.handleDisplayErrors(e);
		}, this));
	},

	update: function(log) {
		this.updateColumns(log);
		if (log.priorities_filter)
			this.updatePriorities(log);
		this.handleTableHeight();
	},

	updateColumns: function(log) {
		var columns = logview.logColumns(log.name);
		log.columns_visible = 0;

		var logviewColumns = [];
		columns.forEach(L.bind(function(column) {
			if (column.empty)
				return;

			if (column.show)
				log.columns_visible++;

			logviewColumns.push(
				E('div', {
					'class': column.show ? 'cb-active' : '',
					'data-hide': column.show ? false : true,
					'click': ui.createHandlerFn(this, 'handleColumnToggle', log, column)
				}, [ column.display ])
			);
		}, this));

		/* div[data-tab="..." .logview-priorities > div > ... */
		var node = document.querySelector('div[data-tab="' + log.name + '"] div.logview-columns > div');
		L.dom.content(node, logviewColumns);
	},

	updatePriorities: function(log) {
		if (!log.priorities_filter)
			return;

		var priorities = logview.logPriorities(log.name);
		var logviewPriorities = [];

		log.priorities_visible = 0;
		log.priorities_total = 0;

		Object.keys(priorities).forEach(L.bind(function(p) {
			log.priorities_total++;
			if (!priorities[p].hide)
				log.priorities_visible++;

			logviewPriorities.push(
				E('div', {
					'class': 'lv-p-' + p + (priorities[p].hide ? '' : ' cb-active'),
					'data-hide': priorities[p].hide ? true : false,
					'data-count': priorities[p].count,
					'click': ui.createHandlerFn(this, 'handlePriorityToggle', log, p)
				}, [ priorities[p].display + ' (%d)'.format(priorities[p].count) ])
			);
		}, this));

		/* div[data-tab="..." .logview-priorities > div > ... */
		var node = document.querySelector('div[data-tab="' + log.name + '"] div.logview-priorities > div');
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
			var log = this.logs[log_names[i]];

			var downloads = {
				value: null,
				choices: {},
				options: {
					click: ui.createHandlerFn(this, 'handleDownload', log),
					classes: {}
				}
			};

			logview.logDownloads(log.name).forEach(function(dl) {
				if (!downloads.value)
					downloads.value = dl.name;

				downloads.choices[dl.name] = '%s (%s)'.format(
					_('Download', 'Download data (action)'), dl.display);
				downloads.options.classes[dl.name] =
					'cbi-button cbi-button-action logview-btn-download';
			});

			var downloadsButton = new ui.ComboButton(
				downloads.value,
				downloads.choices,
				downloads.options
			).render();

			downloadsButton.setAttribute('disabled', 'disabled');

			var buttons = [
				E('button', {
					'class': 'cbi-button cbi-button-action logview-btn-refresh',
					'click': ui.createHandlerFn(this, 'handleRefresh', log),
					'disabled': 'disabled',
					'title': _('Reload log contents')
				}, _('Refresh')),
				downloadsButton,
				E('button', {
					'class': 'cbi-button cbi-button-action logview-btn-clear',
					'click': ui.createHandlerFn(this, 'handleClear', log),
					'disabled': 'disabled',
					'title': _('Clear log')
				}, _('Clear')),
			];

			var logviewTable = E('div', { 'class': 'logview-table' }, [
				E('p', {}, [
					E('em', { 'class': 'spinning' }, [ _('Loading data…') ])
				])
			]);

			log.target = logviewTable;
			log.priorities_filter = false;
			log.filter = '';

			logview.setTarget(log.name, log.target);

			logview.logColumns(log.name).forEach(function(column) {
				if (column.name == 'priority')
					log.priorities_filter = true;
			});

			if (log.priorities_filter) {
				log.priorities_visible = 0;
				log.priorities_total = 0;
			}

			this.controls[log.name] = E('div', { 'class': 'cbi-section logview-controls' }, [
				E('div', {}, [
					E('input', {
						'type': 'checkbox',
						'id': 'sorting-toggle-' + log.name,
						'name': 'sorting-toggle-' + log.name,
						'checked': 'checked',
						'disabled': 'disabled',
						'click': L.bind(this.handleSortingToggle, this, log)
					}),
					E('label', { 'for': 'sorting-toggle-' + log.name },
						_('Display descending time (latest on top)'))
				]),
				E('div', {}, buttons)
			]);

			container.appendChild(E('div', {
				'data-tab': log_names[i],
				'data-tab-title': logview.logTitle(log.name),
				'cbi-tab-active': L.bind(this.handleTabActive, this)
			}, [
				this.controls[log.name],
				E('div', { 'class': 'logview-cbfilters-container' }, [
					log.priorities_filter ? E('div', { 'class': 'cbi-section logview-cbfilter logview-priorities' }, [
						E('label', {}, _('Display priorities') + ':'),
						E('div', {}, E('em', { 'class': 'spinning' }, _('Loading data…')))
					]) : E('div', {}),
					E('div', { 'class': 'cbi-section logview-cbfilter logview-columns' }, [
						E('label', {}, _('Display columns') + ':'),
						E('div', {}, E('em', { 'class': 'spinning' }, _('Loading data…')))
					])
				]),
				E('div', { 'class': 'cbi-section logview-filter' }, [
					E('label', {}, _('Entries filter') + ':'),
					E('input', {
						'type': 'text',
						'name': 'filter',
						'placeholder': _('Type to filter…'),
						'value': '',
						'keyup': L.bind(this.handleFilterKeyUp, this, log)
					}),
					E('button', {
						'class': 'cbi-button cbi-button-action',
						'click': L.bind(this.handleFilterReset, this, log)
					}, [ _('Clear') ])
				]),
				E('div', { 'class': 'cbi-section' }, [
					E('legend', {}, [
						_('Log Entries'),
						' (',
						E('span', { 'id': 'logview-count-info-' + log.name }, '?'),
						')'
					]),
					logviewTable
				]),
			]));
		}

		ui.tabs.initTabGroup(container.childNodes);
	},

	handleTableHeight: function() {
		//var mc     = document.querySelector('.maincontent');
		var view   = document.querySelector('div#maincontent');
		var tables = view.querySelectorAll('.logview-table .table-wrapper');

		var windowHeight = window.innerHeight;
		var footerHeight = 0;

		if (!this.expandedView) {
			var footer = document.querySelector('div.footer');
			if (footer) {
				var footerStyle = footer.currentStyle || window.getComputedStyle(footer);
				footerHeight = parseInt(footerStyle.height, 10) +
					parseInt(footerStyle.marginTop, 10) +
					parseInt(footerStyle.marginBottom, 10);
			}
		}

		//var mcStyle = mc.currentStyle || window.getComputedStyle(mc);

		tables.forEach(function(table) {
			var tableRect = table.getBoundingClientRect();
			var padding;// = parseInt(mcStyle.paddingBottom, 10);

			if (!footerHeight && !padding)
				padding = 16;

			table.style.height = (windowHeight - tableRect.top - padding - footerHeight) + 'px';
		});
	},

	render: function() {
		var logTabs = E('div', { 'data-name': 'logs' });

		var view = E([], [
			E('div', { 'class': 'logview' }, [
				E('link', { 'rel': 'stylesheet', 'href': L.resource('logview/logview.css')}),
				E('h2', {}, [
					_('Logs View')

				]),
				logTabs
			])
		]);

		//window.addEventListener('resize', this.handleTableHeight);

		this.renderTabs(logTabs);
		return view;
	},

	handleSaveApply: null,
	handleSave: null,
	handleReset: null
});
