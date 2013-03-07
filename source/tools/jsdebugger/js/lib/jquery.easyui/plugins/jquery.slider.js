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
var _3=$("<div class=\"slider\">"+"<div class=\"slider-inner\">"+"<a href=\"javascript:void(0)\" class=\"slider-handle\"></a>"+"<span class=\"slider-tip\"></span>"+"</div>"+"<div class=\"slider-rule\"></div>"+"<div class=\"slider-rulelabel\"></div>"+"<div style=\"clear:both\"></div>"+"<input type=\"hidden\" class=\"slider-value\">"+"</div>").insertAfter(_2);
var _4=$(_2).hide().attr("name");
if(_4){
_3.find("input.slider-value").attr("name",_4);
$(_2).removeAttr("name").attr("sliderName",_4);
}
return _3;
};
function _5(_6,_7){
var _8=$.data(_6,"slider").options;
var _9=$.data(_6,"slider").slider;
if(_7){
if(_7.width){
_8.width=_7.width;
}
if(_7.height){
_8.height=_7.height;
}
}
if(_8.mode=="h"){
_9.css("height","");
_9.children("div").css("height","");
if(!isNaN(_8.width)){
_9.width(_8.width);
}
}else{
_9.css("width","");
_9.children("div").css("width","");
if(!isNaN(_8.height)){
_9.height(_8.height);
_9.find("div.slider-rule").height(_8.height);
_9.find("div.slider-rulelabel").height(_8.height);
_9.find("div.slider-inner")._outerHeight(_8.height);
}
}
_a(_6);
};
function _b(_c){
var _d=$.data(_c,"slider").options;
var _e=$.data(_c,"slider").slider;
var aa=_d.mode=="h"?_d.rule:_d.rule.slice(0).reverse();
if(_d.reversed){
aa=aa.slice(0).reverse();
}
_f(aa);
function _f(aa){
var _10=_e.find("div.slider-rule");
var _11=_e.find("div.slider-rulelabel");
_10.empty();
_11.empty();
for(var i=0;i<aa.length;i++){
var _12=i*100/(aa.length-1)+"%";
var _13=$("<span></span>").appendTo(_10);
_13.css((_d.mode=="h"?"left":"top"),_12);
if(aa[i]!="|"){
_13=$("<span></span>").appendTo(_11);
_13.html(aa[i]);
if(_d.mode=="h"){
_13.css({left:_12,marginLeft:-Math.round(_13.outerWidth()/2)});
}else{
_13.css({top:_12,marginTop:-Math.round(_13.outerHeight()/2)});
}
}
}
};
};
function _14(_15){
var _16=$.data(_15,"slider").options;
var _17=$.data(_15,"slider").slider;
_17.removeClass("slider-h slider-v slider-disabled");
_17.addClass(_16.mode=="h"?"slider-h":"slider-v");
_17.addClass(_16.disabled?"slider-disabled":"");
_17.find("a.slider-handle").draggable({axis:_16.mode,cursor:"pointer",disabled:_16.disabled,onDrag:function(e){
var _18=e.data.left;
var _19=_17.width();
if(_16.mode!="h"){
_18=e.data.top;
_19=_17.height();
}
if(_18<0||_18>_19){
return false;
}else{
var _1a=_2c(_15,_18);
_1b(_1a);
return false;
}
},onStartDrag:function(){
_16.onSlideStart.call(_15,_16.value);
},onStopDrag:function(e){
var _1c=_2c(_15,(_16.mode=="h"?e.data.left:e.data.top));
_1b(_1c);
_16.onSlideEnd.call(_15,_16.value);
}});
function _1b(_1d){
var s=Math.abs(_1d%_16.step);
if(s<_16.step/2){
_1d-=s;
}else{
_1d=_1d-s+_16.step;
}
_1e(_15,_1d);
};
};
function _1e(_1f,_20){
var _21=$.data(_1f,"slider").options;
var _22=$.data(_1f,"slider").slider;
var _23=_21.value;
if(_20<_21.min){
_20=_21.min;
}
if(_20>_21.max){
_20=_21.max;
}
_21.value=_20;
$(_1f).val(_20);
_22.find("input.slider-value").val(_20);
var pos=_24(_1f,_20);
var tip=_22.find(".slider-tip");
if(_21.showTip){
tip.show();
tip.html(_21.tipFormatter.call(_1f,_21.value));
}else{
tip.hide();
}
if(_21.mode=="h"){
var _25="left:"+pos+"px;";
_22.find(".slider-handle").attr("style",_25);
tip.attr("style",_25+"margin-left:"+(-Math.round(tip.outerWidth()/2))+"px");
}else{
var _25="top:"+pos+"px;";
_22.find(".slider-handle").attr("style",_25);
tip.attr("style",_25+"margin-left:"+(-Math.round(tip.outerWidth()))+"px");
}
if(_23!=_20){
_21.onChange.call(_1f,_20,_23);
}
};
function _a(_26){
var _27=$.data(_26,"slider").options;
var fn=_27.onChange;
_27.onChange=function(){
};
_1e(_26,_27.value);
_27.onChange=fn;
};
function _24(_28,_29){
var _2a=$.data(_28,"slider").options;
var _2b=$.data(_28,"slider").slider;
if(_2a.mode=="h"){
var pos=(_29-_2a.min)/(_2a.max-_2a.min)*_2b.width();
if(_2a.reversed){
pos=_2b.width()-pos;
}
}else{
var pos=_2b.height()-(_29-_2a.min)/(_2a.max-_2a.min)*_2b.height();
if(_2a.reversed){
pos=_2b.height()-pos;
}
}
return pos.toFixed(0);
};
function _2c(_2d,pos){
var _2e=$.data(_2d,"slider").options;
var _2f=$.data(_2d,"slider").slider;
if(_2e.mode=="h"){
var _30=_2e.min+(_2e.max-_2e.min)*(pos/_2f.width());
}else{
var _30=_2e.min+(_2e.max-_2e.min)*((_2f.height()-pos)/_2f.height());
}
return _2e.reversed?_2e.max-_30.toFixed(0):_30.toFixed(0);
};
$.fn.slider=function(_31,_32){
if(typeof _31=="string"){
return $.fn.slider.methods[_31](this,_32);
}
_31=_31||{};
return this.each(function(){
var _33=$.data(this,"slider");
if(_33){
$.extend(_33.options,_31);
}else{
_33=$.data(this,"slider",{options:$.extend({},$.fn.slider.defaults,$.fn.slider.parseOptions(this),_31),slider:_1(this)});
$(this).removeAttr("disabled");
}
_14(this);
_b(this);
_5(this);
});
};
$.fn.slider.methods={options:function(jq){
return $.data(jq[0],"slider").options;
},destroy:function(jq){
return jq.each(function(){
$.data(this,"slider").slider.remove();
$(this).remove();
});
},resize:function(jq,_34){
return jq.each(function(){
_5(this,_34);
});
},getValue:function(jq){
return jq.slider("options").value;
},setValue:function(jq,_35){
return jq.each(function(){
_1e(this,_35);
});
},enable:function(jq){
return jq.each(function(){
$.data(this,"slider").options.disabled=false;
_14(this);
});
},disable:function(jq){
return jq.each(function(){
$.data(this,"slider").options.disabled=true;
_14(this);
});
}};
$.fn.slider.parseOptions=function(_36){
var t=$(_36);
return $.extend({},$.parser.parseOptions(_36,["width","height","mode",{reversed:"boolean",showTip:"boolean",min:"number",max:"number",step:"number"}]),{value:(t.val()||undefined),disabled:(t.attr("disabled")?true:undefined),rule:(t.attr("rule")?eval(t.attr("rule")):undefined)});
};
$.fn.slider.defaults={width:"auto",height:"auto",mode:"h",reversed:false,showTip:false,disabled:false,value:0,min:0,max:100,step:1,rule:[],tipFormatter:function(_37){
return _37;
},onChange:function(_38,_39){
},onSlideStart:function(_3a){
},onSlideEnd:function(_3b){
}};
})(jQuery);

