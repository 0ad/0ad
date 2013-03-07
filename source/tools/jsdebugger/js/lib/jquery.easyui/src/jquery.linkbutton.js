/**
 * linkbutton - jQuery EasyUI
 * 
 * Copyright (c) 2009-2013 www.jeasyui.com. All rights reserved.
 *
 * Licensed under the GPL or commercial licenses
 * To use it on other terms please contact us: jeasyui@gmail.com
 * http://www.gnu.org/licenses/gpl.txt
 * http://www.jeasyui.com/license_commercial.php
 */
(function($){
	
	function createButton(target) {
		var opts = $.data(target, 'linkbutton').options;
		
		$(target).empty();
		$(target).addClass('l-btn');
		if (opts.id){
			$(target).attr('id', opts.id);
		} else {
//			$.fn.removeProp ? $(target).removeProp('id') : $(target).removeAttr('id'); 
//			$(target).removeAttr('id');
			$(target).attr('id', '');
		}
		if (opts.plain){
			$(target).addClass('l-btn-plain');
		} else {
			$(target).removeClass('l-btn-plain');
		}
		
		if (opts.text){
			$(target).html(opts.text).wrapInner(
					'<span class="l-btn-left">' +
					'<span class="l-btn-text">' +
					'</span>' +
					'</span>'
			);
			if (opts.iconCls){
				$(target).find('.l-btn-text').addClass(opts.iconCls).addClass(opts.iconAlign=='left' ? 'l-btn-icon-left' : 'l-btn-icon-right');
			}
		} else {
			$(target).html('&nbsp;').wrapInner(
					'<span class="l-btn-left">' +
					'<span class="l-btn-text">' +
					'<span class="l-btn-empty"></span>' +
					'</span>' +
					'</span>'
			);
			if (opts.iconCls){
				$(target).find('.l-btn-empty').addClass(opts.iconCls);
			}
		}
		$(target).unbind('.linkbutton').bind('focus.linkbutton',function(){
			if (!opts.disabled){
				$(this).find('span.l-btn-text').addClass('l-btn-focus');
			}
		}).bind('blur.linkbutton',function(){
			$(this).find('span.l-btn-text').removeClass('l-btn-focus');
		});
		
		setDisabled(target, opts.disabled);
	}
	
	function setDisabled(target, disabled){
		var state = $.data(target, 'linkbutton');
		if (disabled){
			state.options.disabled = true;
			var href = $(target).attr('href');
			if (href){
				state.href = href;
				$(target).attr('href', 'javascript:void(0)');
			}
			if (target.onclick){
				state.onclick = target.onclick;
				target.onclick = null;
			}
//			var onclick = $(target).attr('onclick');
//			if (onclick) {
//				state.onclick = onclick;
//				$(target).attr('onclick', '');
//			}
			$(target).addClass('l-btn-disabled');
		} else {
			state.options.disabled = false;
			if (state.href) {
				$(target).attr('href', state.href);
			}
			if (state.onclick) {
				target.onclick = state.onclick;
			}
			$(target).removeClass('l-btn-disabled');
		}
	}
	
	$.fn.linkbutton = function(options, param){
		if (typeof options == 'string'){
			return $.fn.linkbutton.methods[options](this, param);
		}
		
		options = options || {};
		return this.each(function(){
			var state = $.data(this, 'linkbutton');
			if (state){
				$.extend(state.options, options);
			} else {
				$.data(this, 'linkbutton', {
					options: $.extend({}, $.fn.linkbutton.defaults, $.fn.linkbutton.parseOptions(this), options)
				});
				$(this).removeAttr('disabled');
			}
			
			createButton(this);
		});
	};
	
	$.fn.linkbutton.methods = {
		options: function(jq){
			return $.data(jq[0], 'linkbutton').options;
		},
		enable: function(jq){
			return jq.each(function(){
				setDisabled(this, false);
			});
		},
		disable: function(jq){
			return jq.each(function(){
				setDisabled(this, true);
			});
		}
	};
	
	$.fn.linkbutton.parseOptions = function(target){
		var t = $(target);
		return $.extend({}, $.parser.parseOptions(target, ['id','iconCls','iconAlign',{plain:'boolean'}]), {
			disabled: (t.attr('disabled') ? true : undefined),
			text: $.trim(t.html()),
			iconCls: (t.attr('icon') || t.attr('iconCls'))
		});
	};
	
	$.fn.linkbutton.defaults = {
		id: null,
		disabled: false,
		plain: false,
		text: '',
		iconCls: null,
		iconAlign: 'left'
	};
	
})(jQuery);
