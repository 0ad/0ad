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
var _3=$.data(_2,"menubutton").options;
var _4=$(_2);
_4.removeClass("m-btn-active m-btn-plain-active").addClass("m-btn");
_4.linkbutton($.extend({},_3,{text:_3.text+"<span class=\"m-btn-downarrow\">&nbsp;</span>"}));
if(_3.menu){
$(_3.menu).menu({onShow:function(){
_4.addClass((_3.plain==true)?"m-btn-plain-active":"m-btn-active");
},onHide:function(){
_4.removeClass((_3.plain==true)?"m-btn-plain-active":"m-btn-active");
}});
}
_5(_2,_3.disabled);
};
function _5(_6,_7){
var _8=$.data(_6,"menubutton").options;
_8.disabled=_7;
var _9=$(_6);
if(_7){
_9.linkbutton("disable");
_9.unbind(".menubutton");
}else{
_9.linkbutton("enable");
_9.unbind(".menubutton");
_9.bind("click.menubutton",function(){
_a();
return false;
});
var _b=null;
_9.bind("mouseenter.menubutton",function(){
_b=setTimeout(function(){
_a();
},_8.duration);
return false;
}).bind("mouseleave.menubutton",function(){
if(_b){
clearTimeout(_b);
}
});
}
function _a(){
if(!_8.menu){
return;
}
$("body>div.menu-top").menu("hide");
$(_8.menu).menu("show",{alignTo:_9});
_9.blur();
};
};
$.fn.menubutton=function(_c,_d){
if(typeof _c=="string"){
return $.fn.menubutton.methods[_c](this,_d);
}
_c=_c||{};
return this.each(function(){
var _e=$.data(this,"menubutton");
if(_e){
$.extend(_e.options,_c);
}else{
$.data(this,"menubutton",{options:$.extend({},$.fn.menubutton.defaults,$.fn.menubutton.parseOptions(this),_c)});
$(this).removeAttr("disabled");
}
_1(this);
});
};
$.fn.menubutton.methods={options:function(jq){
return $.data(jq[0],"menubutton").options;
},enable:function(jq){
return jq.each(function(){
_5(this,false);
});
},disable:function(jq){
return jq.each(function(){
_5(this,true);
});
},destroy:function(jq){
return jq.each(function(){
var _f=$(this).menubutton("options");
if(_f.menu){
$(_f.menu).menu("destroy");
}
$(this).remove();
});
}};
$.fn.menubutton.parseOptions=function(_10){
var t=$(_10);
return $.extend({},$.fn.linkbutton.parseOptions(_10),$.parser.parseOptions(_10,["menu",{plain:"boolean",duration:"number"}]));
};
$.fn.menubutton.defaults=$.extend({},$.fn.linkbutton.defaults,{plain:true,menu:null,duration:100});
})(jQuery);

