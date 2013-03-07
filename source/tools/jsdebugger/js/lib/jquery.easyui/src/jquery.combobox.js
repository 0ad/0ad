/**
 * combobox - jQuery EasyUI
 * 
 * Copyright (c) 2009-2013 www.jeasyui.com. All rights reserved.
 *
 * Licensed under the GPL or commercial licenses
 * To use it on other terms please contact us: jeasyui@gmail.com
 * http://www.gnu.org/licenses/gpl.txt
 * http://www.jeasyui.com/license_commercial.php
 * 
 * Dependencies:
 *   combo
 * 
 */
(function($){
	/**
	 * scroll panel to display the specified item
	 */
	function scrollTo(target, value){
		var panel = $(target).combo('panel');
		var item = panel.find('div.combobox-item[value="' + value + '"]');
		if (item.length){
			if (item.position().top <= 0){
				var h = panel.scrollTop() + item.position().top;
				panel.scrollTop(h);
			} else if (item.position().top + item.outerHeight() > panel.height()){
				var h = panel.scrollTop() + item.position().top + item.outerHeight() - panel.height();
				panel.scrollTop(h);
			}
		}
	}
	
	/**
	 * select previous item
	 */
	function selectPrev(target){
		var panel = $(target).combo('panel');
		var values = $(target).combo('getValues');
		var item = panel.find('div.combobox-item[value="' + values.pop() + '"]');
		if (item.length){
			var prev = item.prev(':visible');
			if (prev.length){
				item = prev;
			}
		} else {
			item = panel.find('div.combobox-item:visible:last');
		}
		var value = item.attr('value');
		select(target, value);
//		setValues(target, [value]);
		scrollTo(target, value);
	}
	
	/**
	 * select next item
	 */
	function selectNext(target){
		var panel = $(target).combo('panel');
		var values = $(target).combo('getValues');
		var item = panel.find('div.combobox-item[value="' + values.pop() + '"]');
		if (item.length){
			var next = item.next(':visible');
			if (next.length){
				item = next;
			}
		} else {
			item = panel.find('div.combobox-item:visible:first');
		}
		var value = item.attr('value');
		select(target, value);
//		setValues(target, [value]);
		scrollTo(target, value);
	}
	
	/**
	 * select the specified value
	 */
	function select(target, value){
		var opts = $.data(target, 'combobox').options;
		var data = $.data(target, 'combobox').data;
		
		if (opts.multiple){
			var values = $(target).combo('getValues');
			for(var i=0; i<values.length; i++){
				if (values[i] == value) return;
			}
			values.push(value);
			setValues(target, values);
		} else {
			setValues(target, [value]);
		}
		
		for(var i=0; i<data.length; i++){
			if (data[i][opts.valueField] == value){
				opts.onSelect.call(target, data[i]);
				return;
			}
		}
	}
	
	/**
	 * unselect the specified value
	 */
	function unselect(target, value){
		var opts = $.data(target, 'combobox').options;
		var data = $.data(target, 'combobox').data;
		var values = $(target).combo('getValues');
		for(var i=0; i<values.length; i++){
			if (values[i] == value){
				values.splice(i, 1);
				setValues(target, values);
				break;
			}
		}
		for(var i=0; i<data.length; i++){
			if (data[i][opts.valueField] == value){
				opts.onUnselect.call(target, data[i]);
				return;
			}
		}
	}
	
	/**
	 * set values
	 */
	function setValues(target, values, remainText){
		var opts = $.data(target, 'combobox').options;
		var data = $.data(target, 'combobox').data;
		var panel = $(target).combo('panel');
		
		panel.find('div.combobox-item-selected').removeClass('combobox-item-selected');
		var vv = [], ss = [];
		for(var i=0; i<values.length; i++){
			var v = values[i];
			var s = v;
			for(var j=0; j<data.length; j++){
				if (data[j][opts.valueField] == v){
					s = data[j][opts.textField];
					break;
				}
			}
			vv.push(v);
			ss.push(s);
			panel.find('div.combobox-item[value="' + v + '"]').addClass('combobox-item-selected');
		}
		
		$(target).combo('setValues', vv);
		if (!remainText){
			$(target).combo('setText', ss.join(opts.separator));
		}
	}
	
	function transformData(target){
		var opts = $.data(target, 'combobox').options;
		var data = [];
		$('>option', target).each(function(){
			var item = {};
			item[opts.valueField] = $(this).attr('value')!=undefined ? $(this).attr('value') : $(this).html();
			item[opts.textField] = $(this).html();
			item['selected'] = $(this).attr('selected');
			data.push(item);
		});
		return data;
	}
	
	/**
	 * load data, the old list items will be removed.
	 */
	function loadData(target, data, remainText){
		var opts = $.data(target, 'combobox').options;
		var panel = $(target).combo('panel');
		
		$.data(target, 'combobox').data = data;
		
		var selected = $(target).combobox('getValues');
		panel.empty();	// clear old data
		for(var i=0; i<data.length; i++){
			var v = data[i][opts.valueField];
			var s = data[i][opts.textField];
			var item = $('<div class="combobox-item"></div>').appendTo(panel);
			item.attr('value', v);
			if (opts.formatter){
				item.html(opts.formatter.call(target, data[i]));
			} else {
				item.html(s);
			}
			if (data[i]['selected']){
				(function(){
					for(var i=0; i<selected.length; i++){
						if (v == selected[i]) return;
					}
					selected.push(v);
				})();
			}
		}
		if (opts.multiple){
			setValues(target, selected, remainText);
		} else {
			if (selected.length){
				setValues(target, [selected[selected.length-1]], remainText);
			} else {
				setValues(target, [], remainText);
			}
		}
		
		opts.onLoadSuccess.call(target, data);
		
		$('.combobox-item', panel).hover(
			function(){$(this).addClass('combobox-item-hover');},
			function(){$(this).removeClass('combobox-item-hover');}
		).click(function(){
			var item = $(this);
			if (opts.multiple){
				if (item.hasClass('combobox-item-selected')){
					unselect(target, item.attr('value'));
				} else {
					select(target, item.attr('value'));
				}
			} else {
				select(target, item.attr('value'));
				$(target).combo('hidePanel');
			}
		});
	}
	
	/**
	 * request remote data if the url property is setted.
	 */
	function request(target, url, param, remainText){
		var opts = $.data(target, 'combobox').options;
		if (url){
			opts.url = url;
		}
//		if (!opts.url) return;
		param = param || {};
		
		if (opts.onBeforeLoad.call(target, param) == false) return;

		opts.loader.call(target, param, function(data){
			loadData(target, data, remainText);
		}, function(){
			opts.onLoadError.apply(this, arguments);
		});
	}
	
	/**
	 * do the query action
	 */
	function doQuery(target, q){
		var opts = $.data(target, 'combobox').options;
		
		if (opts.multiple && !q){
			setValues(target, [], true);
		} else {
			setValues(target, [q], true);
		}
		
		if (opts.mode == 'remote'){
			request(target, null, {q:q}, true);
		} else {
			var panel = $(target).combo('panel');
			panel.find('div.combobox-item').hide();
			var data = $.data(target, 'combobox').data;
			for(var i=0; i<data.length; i++){
				if (opts.filter.call(target, q, data[i])){
					var v = data[i][opts.valueField];
					var s = data[i][opts.textField];
					var item = panel.find('div.combobox-item[value="' + v + '"]');
					item.show();
					if (s == q){
						setValues(target, [v], true);
						item.addClass('combobox-item-selected');
					}
				}
			}
		}
	}
	
	/**
	 * create the component
	 */
	function create(target){
		var opts = $.data(target, 'combobox').options;
		$(target).addClass('combobox-f');
		$(target).combo($.extend({}, opts, {
			onShowPanel: function(){
				$(target).combo('panel').find('div.combobox-item').show();
				scrollTo(target, $(target).combobox('getValue'));
				opts.onShowPanel.call(target);
			}
		}));
	}
	
	$.fn.combobox = function(options, param){
		if (typeof options == 'string'){
			var method = $.fn.combobox.methods[options];
			if (method){
				return method(this, param);
			} else {
				return this.combo(options, param);
			}
		}
		
		options = options || {};
		return this.each(function(){
			var state = $.data(this, 'combobox');
			if (state){
				$.extend(state.options, options);
				create(this);
			} else {
				state = $.data(this, 'combobox', {
					options: $.extend({}, $.fn.combobox.defaults, $.fn.combobox.parseOptions(this), options)
				});
				create(this);
				loadData(this, transformData(this));
			}
			if (state.options.data){
				loadData(this, state.options.data);
			}
			request(this);
		});
	};
	
	
	$.fn.combobox.methods = {
		options: function(jq){
			var opts = $.data(jq[0], 'combobox').options;
			opts.originalValue = jq.combo('options').originalValue;
			return opts;
		},
		getData: function(jq){
			return $.data(jq[0], 'combobox').data;
		},
		setValues: function(jq, values){
			return jq.each(function(){
				setValues(this, values);
			});
		},
		setValue: function(jq, value){
			return jq.each(function(){
				setValues(this, [value]);
			});
		},
		clear: function(jq){
			return jq.each(function(){
				$(this).combo('clear');
				var panel = $(this).combo('panel');
				panel.find('div.combobox-item-selected').removeClass('combobox-item-selected');
			});
		},
		reset: function(jq){
			return jq.each(function(){
				var opts = $(this).combobox('options');
				if (opts.multiple){
					$(this).combobox('setValues', opts.originalValue);
				} else {
					$(this).combobox('setValue', opts.originalValue);
				}
			});
		},
		loadData: function(jq, data){
			return jq.each(function(){
				loadData(this, data);
			});
		},
		reload: function(jq, url){
			return jq.each(function(){
				request(this, url);
			});
		},
		select: function(jq, value){
			return jq.each(function(){
				select(this, value);
			});
		},
		unselect: function(jq, value){
			return jq.each(function(){
				unselect(this, value);
			});
		}
	};
	
	$.fn.combobox.parseOptions = function(target){
		var t = $(target);
		return $.extend({}, $.fn.combo.parseOptions(target), $.parser.parseOptions(target,[
			'valueField','textField','mode','method','url'
		]));
	};
	
	$.fn.combobox.defaults = $.extend({}, $.fn.combo.defaults, {
		valueField: 'value',
		textField: 'text',
		mode: 'local',	// or 'remote'
		method: 'post',
		url: null,
		data: null,
		
		keyHandler: {
			up: function(){selectPrev(this);},
			down: function(){selectNext(this);},
			enter: function(){
				var values = $(this).combobox('getValues');
				$(this).combobox('setValues', values);
				$(this).combobox('hidePanel');
			},
			query: function(q){doQuery(this, q);}
		},
		filter: function(q, row){
			var opts = $(this).combobox('options');
			return row[opts.textField].indexOf(q) == 0;
		},
		formatter: function(row){
			var opts = $(this).combobox('options');
			return row[opts.textField];
		},
		loader: function(param, success, error){
			var opts = $(this).combobox('options');
			if (!opts.url) return false;
			$.ajax({
				type: opts.method,
				url: opts.url,
				data: param,
				dataType: 'json',
				success: function(data){
					success(data);
				},
				error: function(){
					error.apply(this, arguments);
				}
			});
		},
		
		onBeforeLoad: function(param){},
		onLoadSuccess: function(){},
		onLoadError: function(){},
		onSelect: function(record){},
		onUnselect: function(record){}
	});
})(jQuery);
