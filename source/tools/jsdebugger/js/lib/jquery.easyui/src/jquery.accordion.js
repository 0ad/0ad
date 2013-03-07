/**
 * accordion - jQuery EasyUI
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
 * 
 */
(function($){
	
	function setSize(container){
		var opts = $.data(container, 'accordion').options;
		var panels = $.data(container, 'accordion').panels;
		
		var cc = $(container);
		opts.fit ? $.extend(opts, cc._fit()) : cc._fit(false);
//		if (opts.fit == true){
//			var p = cc.parent();
//			p.addClass('panel-noscroll');
//			if (p[0].tagName == 'BODY') $('html').addClass('panel-fit');
//			opts.width = p.width();
//			opts.height = p.height();
//		}
		
		if (opts.width > 0){
			cc._outerWidth(opts.width);
		}
		var panelHeight = 'auto';
		if (opts.height > 0){
			cc._outerHeight(opts.height);
			// get the first panel's header height as all the header height
			var headerHeight = panels.length ? panels[0].panel('header').css('height', '')._outerHeight() : 'auto';
			var panelHeight = cc.height() - (panels.length-1)*headerHeight;
		}
		for(var i=0; i<panels.length; i++){
			var panel = panels[i];
			var header = panel.panel('header');
			header._outerHeight(headerHeight);
			panel.panel('resize', {
				width: cc.width(),
				height: panelHeight
			});
		}
	}
	
	/**
	 * get the current panel
	 */
	function getCurrent(container){
		var panels = $.data(container, 'accordion').panels;
		for(var i=0; i<panels.length; i++){
			var panel = panels[i];
			if (panel.panel('options').collapsed == false){
				return panel;
			}
		}
		return null;
	}
	
	/**
	 * get panel index, start with 0
	 */
	function getPanelIndex(container, panel){
		var panels = $.data(container, 'accordion').panels;
		for(var i=0; i<panels.length; i++){
			if (panels[i][0] == $(panel)[0]){
				return i;
			}
		}
		return -1;
	}
	
	/**
	 * get the specified panel, remove it from panel array if removeit setted to true.
	 */
	function getPanel(container, which, removeit){
		var panels = $.data(container, 'accordion').panels;
		if (typeof which == 'number'){
			if (which < 0 || which >= panels.length){
				return null;
			} else {
				var panel = panels[which];
				if (removeit){
					panels.splice(which,1);
				}
				return panel;
			}
		}
		for(var i=0; i<panels.length; i++){
			var panel = panels[i];
			if (panel.panel('options').title == which){
				if (removeit){
					panels.splice(i, 1);
				}
				return panel;
			}
		}
		return null;
	}
	
	function setProperties(container){
		var opts = $.data(container, 'accordion').options;
		var cc = $(container);
		if (opts.border){
			cc.removeClass('accordion-noborder');
		} else {
			cc.addClass('accordion-noborder');
		}
	}
	
	function wrapAccordion(container){
		var cc = $(container);
		cc.addClass('accordion');
		
		var panels = [];
		cc.children('div').each(function(){
			var opts = $.extend({}, $.parser.parseOptions(this), {
				selected: ($(this).attr('selected') ? true : undefined)
			});
			var pp = $(this);
			panels.push(pp);
			createPanel(container, pp, opts);
		});
		
		cc.bind('_resize', function(e,force){
			var opts = $.data(container, 'accordion').options;
			if (opts.fit == true || force){
				setSize(container);
			}
			return false;
		});
		
		return {
			accordion: cc,
			panels: panels
		}
	}
	
	function createPanel(container, pp, options){
		pp.panel($.extend({}, options, {
			collapsible: false,
			minimizable: false,
			maximizable: false,
			closable: false,
			doSize: false,
			collapsed: true,
			headerCls: 'accordion-header',
			bodyCls: 'accordion-body',
			onBeforeExpand: function(){
				var curr = getCurrent(container);
				if (curr){
					var header = $(curr).panel('header');
					header.removeClass('accordion-header-selected');
					header.find('.accordion-collapse').triggerHandler('click');
				}
				var header = pp.panel('header');
				header.addClass('accordion-header-selected');
				header.find('.accordion-collapse').removeClass('accordion-expand');
			},
			onExpand: function(){
				var opts = $.data(container, 'accordion').options;
				opts.onSelect.call(container, pp.panel('options').title, getPanelIndex(container, this));
			},
			onBeforeCollapse: function(){
				var header = pp.panel('header');
				header.removeClass('accordion-header-selected');
				header.find('.accordion-collapse').addClass('accordion-expand');
			}
		}));
		
		var header = pp.panel('header');
		var t = $('<a class="accordion-collapse accordion-expand" href="javascript:void(0)"></a>').appendTo(header.children('div.panel-tool'));
		t.bind('click', function(e){
			var animate = $.data(container, 'accordion').options.animate;
			stopAnimate(container);
			if (pp.panel('options').collapsed){
				pp.panel('expand', animate);
			} else {
				pp.panel('collapse', animate);
			}
			return false;
		});
		
		header.click(function(){
			$(this).find('.accordion-collapse').triggerHandler('click');
			return false;
		});
	}
	
	/**
	 * select and set the specified panel active
	 */
	function select(container, which){
		var panel = getPanel(container, which);
		if (!panel) return;
		
		var curr = getCurrent(container);
		if (curr && curr[0] == panel[0]){
			return;
		}
		
		panel.panel('header').triggerHandler('click');
	}
	
	function doFirstSelect(container){
		var panels = $.data(container, 'accordion').panels;
		for(var i=0; i<panels.length; i++){
			if (panels[i].panel('options').selected){
				_select(i);
				return;
			}
		}
		if (panels.length){
			_select(0);
		}
		
		function _select(index){
			var opts = $.data(container, 'accordion').options;
			var animate = opts.animate;
			opts.animate = false;
			select(container, index);
			opts.animate = animate;
		}
	}
	
	/**
	 * stop the animation of all panels
	 */
	function stopAnimate(container){
		var panels = $.data(container, 'accordion').panels;
		for(var i=0; i<panels.length; i++){
			panels[i].stop(true,true);
		}
	}
	
	function add(container, options){
		var opts = $.data(container, 'accordion').options;
		var panels = $.data(container, 'accordion').panels;
		if (options.selected == undefined) options.selected = true;

		stopAnimate(container);
		
		var pp = $('<div></div>').appendTo(container);
		panels.push(pp);
		createPanel(container, pp, options);
		setSize(container);
		
		opts.onAdd.call(container, options.title, panels.length-1);
		
		if (options.selected){
			select(container, panels.length-1);
		}
	}
	
	function remove(container, which){
		var opts = $.data(container, 'accordion').options;
		var panels = $.data(container, 'accordion').panels;
		
		stopAnimate(container);
		
		var panel = getPanel(container, which);
		var title = panel.panel('options').title;
		var index = getPanelIndex(container, panel);
		
		if (opts.onBeforeRemove.call(container, title, index) == false) return;
		
		var panel = getPanel(container, which, true);
		if (panel){
			panel.panel('destroy');
			if (panels.length){
				setSize(container);
				var curr = getCurrent(container);
				if (!curr){
					select(container, 0);
				}
			}
		}
		
		opts.onRemove.call(container, title, index);
	}
	
	$.fn.accordion = function(options, param){
		if (typeof options == 'string'){
			return $.fn.accordion.methods[options](this, param);
		}
		
		options = options || {};
		
		return this.each(function(){
			var state = $.data(this, 'accordion');
			var opts;
			if (state){
				opts = $.extend(state.options, options);
				state.opts = opts;
			} else {
				opts = $.extend({}, $.fn.accordion.defaults, $.fn.accordion.parseOptions(this), options);
				var r = wrapAccordion(this);
				$.data(this, 'accordion', {
					options: opts,
					accordion: r.accordion,
					panels: r.panels
				});
			}
			
			setProperties(this);
			setSize(this);
			doFirstSelect(this);
		});
	};
	
	$.fn.accordion.methods = {
		options: function(jq){
			return $.data(jq[0], 'accordion').options;
		},
		panels: function(jq){
			return $.data(jq[0], 'accordion').panels;
		},
		resize: function(jq){
			return jq.each(function(){
				setSize(this);
			});
		},
		getSelected: function(jq){
			return getCurrent(jq[0]);
		},
		getPanel: function(jq, which){
			return getPanel(jq[0], which);
		},
		getPanelIndex: function(jq, panel){
			return getPanelIndex(jq[0], panel);
		},
		select: function(jq, which){
			return jq.each(function(){
				select(this, which);
			});
		},
		add: function(jq, options){
			return jq.each(function(){
				add(this, options);
			});
		},
		remove: function(jq, which){
			return jq.each(function(){
				remove(this, which);
			});
		}
	};
	
	$.fn.accordion.parseOptions = function(target){
		var t = $(target);
		return $.extend({}, $.parser.parseOptions(target, [
			'width','height',{fit:'boolean',border:'boolean',animate:'boolean'}
		]));
	};
	
	$.fn.accordion.defaults = {
		width: 'auto',
		height: 'auto',
		fit: false,
		border: true,
		animate: true,
		
		onSelect: function(title, index){},
		onAdd: function(title, index){},
		onBeforeRemove: function(title, index){},
		onRemove: function(title, index){}
	};
})(jQuery);
