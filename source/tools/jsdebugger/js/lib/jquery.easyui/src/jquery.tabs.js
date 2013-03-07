/**
 * tabs - jQuery EasyUI
 * 
 * Copyright (c) 2009-2013 www.jeasyui.com. All rights reserved.
 *
 * Licensed under the GPL or commercial licenses
 * To use it on other terms please contact us: jeasyui@gmail.com
 * http://www.gnu.org/licenses/gpl.txt
 * http://www.jeasyui.com/license_commercial.php
 *
 * Dependencies:
 * 	 panel
 *   linkbutton
 * 
 */
(function($){
	
	/**
	 * set the tabs scrollers to show or not,
	 * dependent on the tabs count and width
	 */
	function setScrollers(container) {
		var opts = $.data(container, 'tabs').options;
		if (opts.tabPosition == 'left' || opts.tabPosition == 'right'){return}
		
		var header = $(container).children('div.tabs-header');
		var tool = header.children('div.tabs-tool');
		var sLeft = header.children('div.tabs-scroller-left');
		var sRight = header.children('div.tabs-scroller-right');
		var wrap = header.children('div.tabs-wrap');
		
		// set the tool height
		tool._outerHeight(header.outerHeight() - (opts.plain ? 2 : 0));
		
		var tabsWidth = 0;
		$('ul.tabs li', header).each(function(){
			tabsWidth += $(this).outerWidth(true);
		});
		var cWidth = header.width() - tool._outerWidth();
		
		if (tabsWidth > cWidth) {
			sLeft.show();
			sRight.show();
			if (opts.toolPosition == 'left'){
				tool.css({
					left: sLeft.outerWidth(),
					right: ''
				});
				wrap.css({
					marginLeft: sLeft.outerWidth() + tool._outerWidth(),
					marginRight: sRight._outerWidth(),
					width: cWidth - sLeft.outerWidth() - sRight.outerWidth()
				});
			} else {
				tool.css({
					left: '',
					right: sRight.outerWidth()
				});
				wrap.css({
					marginLeft: sLeft.outerWidth(),
					marginRight: sRight.outerWidth() + tool._outerWidth(),
					width: cWidth - sLeft.outerWidth() - sRight.outerWidth()
				});
			}
		} else {
			sLeft.hide();
			sRight.hide();
			if (opts.toolPosition == 'left'){
				tool.css({
					left: 0,
					right: ''
				});
				wrap.css({
					marginLeft: tool._outerWidth(),
					marginRight: 0,
					width: cWidth
				});
			} else {
				tool.css({
					left: '',
					right: 0
				});
				wrap.css({
					marginLeft: 0,
					marginRight: tool._outerWidth(),
					width: cWidth
				});
			}
//			wrap.scrollLeft(0);
		}
	}
	
	function addTools(container){
		var opts = $.data(container, 'tabs').options;
		var header = $(container).children('div.tabs-header');
		if (opts.tools) {
			if (typeof opts.tools == 'string'){
				$(opts.tools).addClass('tabs-tool').appendTo(header);
				$(opts.tools).show();
			} else {
				header.children('div.tabs-tool').remove();
				var tools = $('<div class="tabs-tool"></div>').appendTo(header);
				for(var i=0; i<opts.tools.length; i++){
					var tool = $('<a href="javascript:void(0);"></a>').appendTo(tools);
					tool[0].onclick = eval(opts.tools[i].handler || function(){});
					tool.linkbutton($.extend({}, opts.tools[i], {
						plain: true
					}));
				}
			}
		} else {
			header.children('div.tabs-tool').remove();
		}
	}
	
	function setSize(container) {
		var opts = $.data(container, 'tabs').options;
		var cc = $(container);
		
		opts.fit ? $.extend(opts, cc._fit()) : cc._fit(false);
//		if (opts.fit == true){
//			var p = cc.parent();
//			p.addClass('panel-noscroll');
//			if (p[0].tagName == 'BODY') $('html').addClass('panel-fit');
//			opts.width = p.width();
//			opts.height = p.height();
//		}
		cc.width(opts.width).height(opts.height);
		
		var header = $(container).children('div.tabs-header');
		var panels = $(container).children('div.tabs-panels');
		
		if (opts.tabPosition == 'left' || opts.tabPosition == 'right'){
			header._outerWidth(opts.headerWidth);
			panels._outerWidth(cc.width() - opts.headerWidth);
			header.add(panels)._outerHeight(opts.height);
			var wrap = header.find('div.tabs-wrap');
			wrap._outerWidth(header.width());
			header.find('.tabs')._outerWidth(wrap.width());
		} else {
			header.css('height','');
			header.find('div.tabs-wrap').css('width','');
			header.find('.tabs').css('width','');
			
			header._outerWidth(opts.width);
			
			setScrollers(container);
			
			var height = opts.height;
			if (!isNaN(height)) {
				panels._outerHeight(height - header.outerHeight());
			} else {
				panels.height('auto');
			}
			var width = opts.width;
			if (!isNaN(width)){
				panels._outerWidth(width);
			} else {
				panels.width('auto');
			}
		}
	}
	
	/**
	 * set selected tab panel size
	 */
	function setSelectedSize(container){
		var opts = $.data(container, 'tabs').options;
		var tab = getSelectedTab(container);
		if (tab){
			var panels = $(container).children('div.tabs-panels');
			var width = opts.width=='auto' ? 'auto' : panels.width();
			var height = opts.height=='auto' ? 'auto' : panels.height();
			tab.panel('resize', {
				width: width,
				height: height
			});
		}
	}
	
	/**
	 * wrap the tabs header and body
	 */
	function wrapTabs(container) {
		var tabs = $.data(container, 'tabs').tabs;
		var cc = $(container);
		cc.addClass('tabs-container');
		cc.wrapInner('<div class="tabs-panels"/>');
		$('<div class="tabs-header">'
				+ '<div class="tabs-scroller-left"></div>'
				+ '<div class="tabs-scroller-right"></div>'
				+ '<div class="tabs-wrap">'
				+ '<ul class="tabs"></ul>'
				+ '</div>'
				+ '</div>').prependTo(container);
		
		cc.children('div.tabs-panels').children('div').each(function(i){
			var opts = $.extend({}, $.parser.parseOptions(this), {
				selected: ($(this).attr('selected') ? true : undefined)
			});
			var pp = $(this);
			tabs.push(pp);
			createTab(container, pp, opts);
		});
		
		cc.children('div.tabs-header').find('.tabs-scroller-left, .tabs-scroller-right').hover(
			function(){$(this).addClass('tabs-scroller-over');},
			function(){$(this).removeClass('tabs-scroller-over');}
		);
		cc.bind('_resize', function(e,force){
			var opts = $.data(container, 'tabs').options;
			if (opts.fit == true || force){
				setSize(container);
				setSelectedSize(container);
			}
			return false;
		});
	}
	
	function setProperties(container){
		var opts = $.data(container, 'tabs').options;
		var header = $(container).children('div.tabs-header');
		var panels = $(container).children('div.tabs-panels');
		
		header.removeClass('tabs-header-top tabs-header-bottom tabs-header-left tabs-header-right');
		panels.removeClass('tabs-panels-top tabs-panels-bottom tabs-panels-left tabs-panels-right');
		if (opts.tabPosition == 'top'){
			header.insertBefore(panels);
		}else if (opts.tabPosition == 'bottom'){
			header.insertAfter(panels);
			header.addClass('tabs-header-bottom');
			panels.addClass('tabs-panels-top');
		} else if (opts.tabPosition == 'left'){
			header.addClass('tabs-header-left');
			panels.addClass('tabs-panels-right');
		} else if (opts.tabPosition == 'right'){
			header.addClass('tabs-header-right');
//			header.addClass('tabs-header-left tabs-header-right');
			panels.addClass('tabs-panels-left');
		}
		
		if (opts.plain == true) {
			header.addClass('tabs-header-plain');
		} else {
			header.removeClass('tabs-header-plain');
		}
		if (opts.border == true){
			header.removeClass('tabs-header-noborder');
			panels.removeClass('tabs-panels-noborder');
		} else {
			header.addClass('tabs-header-noborder');
			panels.addClass('tabs-panels-noborder');
		}
		
		$('.tabs-scroller-left', header).unbind('.tabs').bind('click.tabs', function(){
			$(container).tabs('scrollBy', -opts.scrollIncrement);
		});
		
		$('.tabs-scroller-right', header).unbind('.tabs').bind('click.tabs', function(){
			$(container).tabs('scrollBy', opts.scrollIncrement);
		});
	}
	
	function createTab(container, pp, options) {
		var state = $.data(container, 'tabs');
		options = options || {};
		
		// create panel
		pp.panel($.extend({}, options, {
			border: false,
			noheader: true,
			closed: true,
			doSize: false,
			iconCls: (options.icon ? options.icon : undefined),
			onLoad: function(){
				if (options.onLoad){
					options.onLoad.call(this, arguments);
				}
				state.options.onLoad.call(container, $(this));
			}
		}));
		
		var opts = pp.panel('options');
		
		var tabs = $(container).children('div.tabs-header').find('ul.tabs');
		
		opts.tab = $('<li></li>').appendTo(tabs);	// set the tab object in panel options
		opts.tab.append(
				'<a href="javascript:void(0)" class="tabs-inner">' +
				'<span class="tabs-title"></span>' +
				'<span class="tabs-icon"></span>' +
				'</a>'
		);
		opts.tab.unbind('.tabs').bind('click.tabs', {p:pp}, function(e){
			if ($(this).hasClass('tabs-disabled')){return;}
			selectTab(container, getTabIndex(container, e.data.p));
		}).bind('contextmenu.tabs', {p:pp}, function(e){
			if ($(this).hasClass('tabs-disabled')){return;}
			state.options.onContextMenu.call(container, e, $(this).find('span.tabs-title').html(), getTabIndex(container, e.data.p));
		});
		
//		updateTab(container, {
//			tab: pp,
//			options: opts
//		});
		$(container).tabs('update', {
			tab: pp,
			options: opts
		});
	}
	
	function addTab(container, options) {
		var opts = $.data(container, 'tabs').options;
		var tabs = $.data(container, 'tabs').tabs;
		if (options.selected == undefined) options.selected = true;
		
		var pp = $('<div></div>').appendTo($(container).children('div.tabs-panels'));
		tabs.push(pp);
		createTab(container, pp, options);
		
		opts.onAdd.call(container, options.title, tabs.length-1);
		
		setScrollers(container);
		if (options.selected){
			selectTab(container, tabs.length-1);	// select the added tab panel
		}
	}
	
	/**
	 * update tab panel, param has following properties:
	 * tab: the tab panel to be updated
	 * options: the tab panel options
	 */
	function updateTab(container, param){
		var selectHis = $.data(container, 'tabs').selectHis;
		var pp = param.tab;	// the tab panel
		var oldTitle = pp.panel('options').title; 
		pp.panel($.extend({}, param.options, {
			iconCls: (param.options.icon ? param.options.icon : undefined)
		}));
		
		var opts = pp.panel('options');	// get the tab panel options
		var tab = opts.tab;
		
		var s_title = tab.find('span.tabs-title');
		var s_icon = tab.find('span.tabs-icon');
		s_title.html(opts.title);
		s_icon.attr('class', 'tabs-icon');
		
		tab.find('a.tabs-close').remove();
		if (opts.closable){
			s_title.addClass('tabs-closable');
			var a_close = $('<a href="javascript:void(0)" class="tabs-close"></a>').appendTo(tab);
			a_close.bind('click.tabs', {p:pp}, function(e){
				if ($(this).parent().hasClass('tabs-disabled')){return;}
				closeTab(container, getTabIndex(container, e.data.p));
				return false;
			});
		} else{
			s_title.removeClass('tabs-closable');
		}
		if (opts.iconCls){
			s_title.addClass('tabs-with-icon');
			s_icon.addClass(opts.iconCls);
		} else {
			s_title.removeClass('tabs-with-icon');
		}
		
		if (oldTitle != opts.title){
			for(var i=0; i<selectHis.length; i++){
				if (selectHis[i] == oldTitle){
					selectHis[i] = opts.title;
				}
			}
		}
		
		tab.find('span.tabs-p-tool').remove();
		if (opts.tools){
			var p_tool = $('<span class="tabs-p-tool"></span>').insertAfter(tab.find('a.tabs-inner'));
			if (typeof opts.tools == 'string'){
				$(opts.tools).children().appendTo(p_tool);
			} else {
				for(var i=0; i<opts.tools.length; i++){
					var t = $('<a href="javascript:void(0)"></a>').appendTo(p_tool);
					t.addClass(opts.tools[i].iconCls);
					if (opts.tools[i].handler){
//						t.bind('click', eval(opts.tools[i].handler));
						t.bind('click', {handler:opts.tools[i].handler}, function(e){
							if ($(this).parents('li').hasClass('tabs-disabled')){return;}
							e.data.handler.call(this);
						});
					}
				}
			}
			var pr = p_tool.children().length * 12;
			if (opts.closable) {
				pr += 8;
			} else {
				pr -= 3;
				p_tool.css('right','5px');
			}
			s_title.css('padding-right', pr+'px');
		}
		
//		setProperties(container);
		setScrollers(container);
		
		$.data(container, 'tabs').options.onUpdate.call(container, opts.title, getTabIndex(container, pp));
	}
	
	/**
	 * close a tab with specified index or title
	 */
	function closeTab(container, which) {
		var opts = $.data(container, 'tabs').options;
		var tabs = $.data(container, 'tabs').tabs;
		var selectHis = $.data(container, 'tabs').selectHis;
		
		if (!exists(container, which)) return;
		
		var tab = getTab(container, which);
		var title = tab.panel('options').title;
		var index = getTabIndex(container, tab);
		
		if (opts.onBeforeClose.call(container, title, index) == false) return;
		
		var tab = getTab(container, which, true);
		tab.panel('options').tab.remove();
		tab.panel('destroy');
		
		opts.onClose.call(container, title, index);
		
		setScrollers(container);
		
		// remove the select history item
		for(var i=0; i<selectHis.length; i++){
			if (selectHis[i] == title){
				selectHis.splice(i, 1);
				i --;
			}
		}
		
		// select the nearest tab panel
		var hisTitle = selectHis.pop();
		if (hisTitle){
			selectTab(container, hisTitle);
		} else if (tabs.length){
			selectTab(container, 0);
		}
	}
	
	/**
	 * get the specified tab panel
	 */
	function getTab(container, which, removeit){
		var tabs = $.data(container, 'tabs').tabs;
		if (typeof which == 'number'){
			if (which < 0 || which >= tabs.length){
				return null;
			} else {
				var tab = tabs[which];
				if (removeit) {
					tabs.splice(which, 1);
				}
				return tab;
			}
		}
		for(var i=0; i<tabs.length; i++){
			var tab = tabs[i];
			if (tab.panel('options').title == which){
				if (removeit){
					tabs.splice(i, 1);
				}
				return tab;
			}
		}
		return null;
	}
	
	function getTabIndex(container, tab){
		var tabs = $.data(container, 'tabs').tabs;
		for(var i=0; i<tabs.length; i++){
			if (tabs[i][0] == $(tab)[0]){
				return i;
			}
		}
		return -1;
	}
	
	function getSelectedTab(container){
		var tabs = $.data(container, 'tabs').tabs;
		for(var i=0; i<tabs.length; i++){
			var tab = tabs[i];
			if (tab.panel('options').closed == false){
				return tab;
			}
		}
		return null;
	}
	
	/**
	 * do first select action, if no tab is setted the first tab will be selected.
	 */
	function doFirstSelect(container){
		var tabs = $.data(container, 'tabs').tabs;
		for(var i=0; i<tabs.length; i++){
			if (tabs[i].panel('options').selected){
				selectTab(container, i);
				return;
			}
		}
		if (tabs.length){
			selectTab(container, 0);
		}
	}
	
	function selectTab(container, which){
		var opts = $.data(container, 'tabs').options;
		var tabs = $.data(container, 'tabs').tabs;
		var selectHis = $.data(container, 'tabs').selectHis;
		
		if (tabs.length == 0) return;
		
		var panel = getTab(container, which); // get the panel to be activated
		if (!panel) return;
		
		var selected = getSelectedTab(container);
		if (selected){
			selected.panel('close');
			selected.panel('options').tab.removeClass('tabs-selected');
		}
		
		panel.panel('open');
		var title = panel.panel('options').title;	// the panel title
		selectHis.push(title);	// push select history
		
		var tab = panel.panel('options').tab;	// get the tab object
		tab.addClass('tabs-selected');
		
		// scroll the tab to center position if required.
		var wrap = $(container).find('>div.tabs-header>div.tabs-wrap');
		var left = tab.position().left;
		var right = left + tab.outerWidth();
		if (left < 0 || right > wrap.width()){
			var deltaX = left - (wrap.width()-tab.width()) / 2;
			$(container).tabs('scrollBy', deltaX);
		} else {
			$(container).tabs('scrollBy', 0);
		}
		
		setSelectedSize(container);
		
		opts.onSelect.call(container, title, getTabIndex(container, panel));
	}
	
	function exists(container, which){
		return getTab(container, which) != null;
	}
	
	
	$.fn.tabs = function(options, param){
		if (typeof options == 'string') {
			return $.fn.tabs.methods[options](this, param);
		}
		
		options = options || {};
		return this.each(function(){
			var state = $.data(this, 'tabs');
			var opts;
			if (state) {
				opts = $.extend(state.options, options);
				state.options = opts;
			} else {
				$.data(this, 'tabs', {
					options: $.extend({},$.fn.tabs.defaults, $.fn.tabs.parseOptions(this), options),
					tabs: [],
					selectHis: []
				});
				wrapTabs(this);
			}
			
			addTools(this);
			setProperties(this);
			setSize(this);
			
			doFirstSelect(this);
		});
	};
	
	$.fn.tabs.methods = {
		options: function(jq){
			return $.data(jq[0], 'tabs').options;
		},
		tabs: function(jq){
			return $.data(jq[0], 'tabs').tabs;
		},
		resize: function(jq){
			return jq.each(function(){
				setSize(this);
				setSelectedSize(this);
			});
		},
		add: function(jq, options){
			return jq.each(function(){
				addTab(this, options);
			});
		},
		close: function(jq, which){
			return jq.each(function(){
				closeTab(this, which);
			});
		},
		getTab: function(jq, which){
			return getTab(jq[0], which);
		},
		getTabIndex: function(jq, tab){
			return getTabIndex(jq[0], tab);
		},
		getSelected: function(jq){
			return getSelectedTab(jq[0]);
		},
		select: function(jq, which){
			return jq.each(function(){
				selectTab(this, which);
			});
		},
		exists: function(jq, which){
			return exists(jq[0], which);
		},
		update: function(jq, options){
			return jq.each(function(){
				updateTab(this, options);
			});
		},
		enableTab: function(jq, which){
			return jq.each(function(){
				$(this).tabs('getTab', which).panel('options').tab.removeClass('tabs-disabled');
			});
		},
		disableTab: function(jq, which){
			return jq.each(function(){
				$(this).tabs('getTab', which).panel('options').tab.addClass('tabs-disabled');
			});
		},
		scrollBy: function(jq, deltaX){	// scroll the tab header by the specified amount of pixels
			return jq.each(function(){
				var opts = $(this).tabs('options');
				var wrap = $(this).find('>div.tabs-header>div.tabs-wrap');
				var pos = Math.min(wrap._scrollLeft() + deltaX, getMaxScrollWidth());
				wrap.animate({scrollLeft: pos}, opts.scrollDuration);
				
				function getMaxScrollWidth(){
					var w = 0;
					var ul = wrap.children('ul');
					ul.children('li').each(function(){
						w += $(this).outerWidth(true);
					});
					return w - wrap.width() + (ul.outerWidth() - ul.width());
				}
			});
		}
	};
	
	$.fn.tabs.parseOptions = function(target){
		return $.extend({}, $.parser.parseOptions(target, [
			'width','height','tools','toolPosition','tabPosition',
			{fit:'boolean',border:'boolean',plain:'boolean',headerWidth:'number'}
		]));
	};
	
	$.fn.tabs.defaults = {
		width: 'auto',
		height: 'auto',
		headerWidth: 150,	// the tab header width, it is valid only when tabPosition set to 'left' or 'right' 
		plain: false,
		fit: false,
		border: true,
		tools: null,
		toolPosition: 'right',	// left,right
		tabPosition: 'top',		// possible values: top,bottom
		scrollIncrement: 100,
		scrollDuration: 400,
		onLoad: function(panel){},
		onSelect: function(title, index){},
		onBeforeClose: function(title, index){},
		onClose: function(title, index){},
		onAdd: function(title, index){},
		onUpdate: function(title, index){},
		onContextMenu: function(e, title, index){}
	};
})(jQuery);
