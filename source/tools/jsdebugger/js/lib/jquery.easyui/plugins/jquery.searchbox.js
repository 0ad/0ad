/**
 * jQuery EasyUI 1.3.2
 * 
 * Copyright (c) 2009-2013 www.jeasyui.com. All rights reserved.
 *
 * Licensed under the GPL or commercial licenses
 * To use it on other terms please contact us: jeasyui@gmail.com
 * http://www.gnu.org/licenses/gpl.txt
 * http://www.jeasyui.com/license_commercial.php
 *
 */
(function($){
function _1(_2){
$(_2).hide();
var _3=$("<span class=\"searchbox\"></span>").insertAfter(_2);
var _4=$("<input type=\"text\" class=\"searchbox-text\">").appendTo(_3);
$("<span><span class=\"searchbox-button\"></span></span>").appendTo(_3);
var _5=$(_2).attr("name");
if(_5){
_4.attr("name",_5);
$(_2).removeAttr("name").attr("searchboxName",_5);
}
return _3;
};
function _6(_7,_8){
var _9=$.data(_7,"searchbox").options;
var sb=$.data(_7,"searchbox").searchbox;
if(_8){
_9.width=_8;
}
sb.appendTo("body");
if(isNaN(_9.width)){
_9.width=sb._outerWidth();
}
var _a=sb.find("span.searchbox-button");
var _b=sb.find("a.searchbox-menu");
var _c=sb.find("input.searchbox-text");
sb._outerWidth(_9.width)._outerHeight(_9.height);
_c._outerWidth(sb.width()-_b._outerWidth()-_a._outerWidth());
_c.css({height:sb.height()+"px",lineHeight:sb.height()+"px"});
_b._outerHeight(sb.height());
_a._outerHeight(sb.height());
var _d=_b.find("span.l-btn-left");
_d._outerHeight(sb.height());
_d.find("span.l-btn-text,span.m-btn-downarrow").css({height:_d.height()+"px",lineHeight:_d.height()+"px"});
sb.insertAfter(_7);
};
function _e(_f){
var _10=$.data(_f,"searchbox");
var _11=_10.options;
if(_11.menu){
_10.menu=$(_11.menu).menu({onClick:function(_12){
_13(_12);
}});
var _14=_10.menu.children("div.menu-item:first");
_10.menu.children("div.menu-item").each(function(){
var _15=$.extend({},$.parser.parseOptions(this),{selected:($(this).attr("selected")?true:undefined)});
if(_15.selected){
_14=$(this);
return false;
}
});
_14.triggerHandler("click");
}else{
_10.searchbox.find("a.searchbox-menu").remove();
_10.menu=null;
}
function _13(_16){
_10.searchbox.find("a.searchbox-menu").remove();
var mb=$("<a class=\"searchbox-menu\" href=\"javascript:void(0)\"></a>").html(_16.text);
mb.prependTo(_10.searchbox).menubutton({menu:_10.menu,iconCls:_16.iconCls});
_10.searchbox.find("input.searchbox-text").attr("name",$(_16.target).attr("name")||_16.text);
_6(_f);
};
};
function _17(_18){
var _19=$.data(_18,"searchbox");
var _1a=_19.options;
var _1b=_19.searchbox.find("input.searchbox-text");
var _1c=_19.searchbox.find(".searchbox-button");
_1b.unbind(".searchbox").bind("blur.searchbox",function(e){
_1a.value=$(this).val();
if(_1a.value==""){
$(this).val(_1a.prompt);
$(this).addClass("searchbox-prompt");
}else{
$(this).removeClass("searchbox-prompt");
}
}).bind("focus.searchbox",function(e){
if($(this).val()!=_1a.value){
$(this).val(_1a.value);
}
$(this).removeClass("searchbox-prompt");
}).bind("keydown.searchbox",function(e){
if(e.keyCode==13){
e.preventDefault();
var _1d=$.fn.prop?_1b.prop("name"):_1b.attr("name");
_1a.value=$(this).val();
_1a.searcher.call(_18,_1a.value,_1d);
return false;
}
});
_1c.unbind(".searchbox").bind("click.searchbox",function(){
var _1e=$.fn.prop?_1b.prop("name"):_1b.attr("name");
_1a.searcher.call(_18,_1a.value,_1e);
}).bind("mouseenter.searchbox",function(){
$(this).addClass("searchbox-button-hover");
}).bind("mouseleave.searchbox",function(){
$(this).removeClass("searchbox-button-hover");
});
};
function _1f(_20){
var _21=$.data(_20,"searchbox");
var _22=_21.options;
var _23=_21.searchbox.find("input.searchbox-text");
if(_22.value==""){
_23.val(_22.prompt);
_23.addClass("searchbox-prompt");
}else{
_23.val(_22.value);
_23.removeClass("searchbox-prompt");
}
};
$.fn.searchbox=function(_24,_25){
if(typeof _24=="string"){
return $.fn.searchbox.methods[_24](this,_25);
}
_24=_24||{};
return this.each(function(){
var _26=$.data(this,"searchbox");
if(_26){
$.extend(_26.options,_24);
}else{
_26=$.data(this,"searchbox",{options:$.extend({},$.fn.searchbox.defaults,$.fn.searchbox.parseOptions(this),_24),searchbox:_1(this)});
}
_e(this);
_1f(this);
_17(this);
_6(this);
});
};
$.fn.searchbox.methods={options:function(jq){
return $.data(jq[0],"searchbox").options;
},menu:function(jq){
return $.data(jq[0],"searchbox").menu;
},textbox:function(jq){
return $.data(jq[0],"searchbox").searchbox.find("input.searchbox-text");
},getValue:function(jq){
return $.data(jq[0],"searchbox").options.value;
},setValue:function(jq,_27){
return jq.each(function(){
$(this).searchbox("options").value=_27;
$(this).searchbox("textbox").val(_27);
$(this).searchbox("textbox").blur();
});
},getName:function(jq){
return $.data(jq[0],"searchbox").searchbox.find("input.searchbox-text").attr("name");
},selectName:function(jq,_28){
return jq.each(function(){
var _29=$.data(this,"searchbox").menu;
if(_29){
_29.children("div.menu-item[name=\""+_28+"\"]").triggerHandler("click");
}
});
},destroy:function(jq){
return jq.each(function(){
var _2a=$(this).searchbox("menu");
if(_2a){
_2a.menu("destroy");
}
$.data(this,"searchbox").searchbox.remove();
$(this).remove();
});
},resize:function(jq,_2b){
return jq.each(function(){
_6(this,_2b);
});
}};
$.fn.searchbox.parseOptions=function(_2c){
var t=$(_2c);
return $.extend({},$.parser.parseOptions(_2c,["width","height","prompt","menu"]),{value:t.val(),searcher:(t.attr("searcher")?eval(t.attr("searcher")):undefined)});
};
$.fn.searchbox.defaults={width:"auto",height:22,prompt:"",value:"",menu:null,searcher:function(_2d,_2e){
}};
})(jQuery);

