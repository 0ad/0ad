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
var _3=$.data(_2,"accordion").options;
var _4=$.data(_2,"accordion").panels;
var cc=$(_2);
_3.fit?$.extend(_3,cc._fit()):cc._fit(false);
if(_3.width>0){
cc._outerWidth(_3.width);
}
var _5="auto";
if(_3.height>0){
cc._outerHeight(_3.height);
var _6=_4.length?_4[0].panel("header").css("height","")._outerHeight():"auto";
var _5=cc.height()-(_4.length-1)*_6;
}
for(var i=0;i<_4.length;i++){
var _7=_4[i];
var _8=_7.panel("header");
_8._outerHeight(_6);
_7.panel("resize",{width:cc.width(),height:_5});
}
};
function _9(_a){
var _b=$.data(_a,"accordion").panels;
for(var i=0;i<_b.length;i++){
var _c=_b[i];
if(_c.panel("options").collapsed==false){
return _c;
}
}
return null;
};
function _d(_e,_f){
var _10=$.data(_e,"accordion").panels;
for(var i=0;i<_10.length;i++){
if(_10[i][0]==$(_f)[0]){
return i;
}
}
return -1;
};
function _11(_12,_13,_14){
var _15=$.data(_12,"accordion").panels;
if(typeof _13=="number"){
if(_13<0||_13>=_15.length){
return null;
}else{
var _16=_15[_13];
if(_14){
_15.splice(_13,1);
}
return _16;
}
}
for(var i=0;i<_15.length;i++){
var _16=_15[i];
if(_16.panel("options").title==_13){
if(_14){
_15.splice(i,1);
}
return _16;
}
}
return null;
};
function _17(_18){
var _19=$.data(_18,"accordion").options;
var cc=$(_18);
if(_19.border){
cc.removeClass("accordion-noborder");
}else{
cc.addClass("accordion-noborder");
}
};
function _1a(_1b){
var cc=$(_1b);
cc.addClass("accordion");
var _1c=[];
cc.children("div").each(function(){
var _1d=$.extend({},$.parser.parseOptions(this),{selected:($(this).attr("selected")?true:undefined)});
var pp=$(this);
_1c.push(pp);
_20(_1b,pp,_1d);
});
cc.bind("_resize",function(e,_1e){
var _1f=$.data(_1b,"accordion").options;
if(_1f.fit==true||_1e){
_1(_1b);
}
return false;
});
return {accordion:cc,panels:_1c};
};
function _20(_21,pp,_22){
pp.panel($.extend({},_22,{collapsible:false,minimizable:false,maximizable:false,closable:false,doSize:false,collapsed:true,headerCls:"accordion-header",bodyCls:"accordion-body",onBeforeExpand:function(){
var _23=_9(_21);
if(_23){
var _24=$(_23).panel("header");
_24.removeClass("accordion-header-selected");
_24.find(".accordion-collapse").triggerHandler("click");
}
var _24=pp.panel("header");
_24.addClass("accordion-header-selected");
_24.find(".accordion-collapse").removeClass("accordion-expand");
},onExpand:function(){
var _25=$.data(_21,"accordion").options;
_25.onSelect.call(_21,pp.panel("options").title,_d(_21,this));
},onBeforeCollapse:function(){
var _26=pp.panel("header");
_26.removeClass("accordion-header-selected");
_26.find(".accordion-collapse").addClass("accordion-expand");
}}));
var _27=pp.panel("header");
var t=$("<a class=\"accordion-collapse accordion-expand\" href=\"javascript:void(0)\"></a>").appendTo(_27.children("div.panel-tool"));
t.bind("click",function(e){
var _28=$.data(_21,"accordion").options.animate;
_35(_21);
if(pp.panel("options").collapsed){
pp.panel("expand",_28);
}else{
pp.panel("collapse",_28);
}
return false;
});
_27.click(function(){
$(this).find(".accordion-collapse").triggerHandler("click");
return false;
});
};
function _29(_2a,_2b){
var _2c=_11(_2a,_2b);
if(!_2c){
return;
}
var _2d=_9(_2a);
if(_2d&&_2d[0]==_2c[0]){
return;
}
_2c.panel("header").triggerHandler("click");
};
function _2e(_2f){
var _30=$.data(_2f,"accordion").panels;
for(var i=0;i<_30.length;i++){
if(_30[i].panel("options").selected){
_31(i);
return;
}
}
if(_30.length){
_31(0);
}
function _31(_32){
var _33=$.data(_2f,"accordion").options;
var _34=_33.animate;
_33.animate=false;
_29(_2f,_32);
_33.animate=_34;
};
};
function _35(_36){
var _37=$.data(_36,"accordion").panels;
for(var i=0;i<_37.length;i++){
_37[i].stop(true,true);
}
};
function add(_38,_39){
var _3a=$.data(_38,"accordion").options;
var _3b=$.data(_38,"accordion").panels;
if(_39.selected==undefined){
_39.selected=true;
}
_35(_38);
var pp=$("<div></div>").appendTo(_38);
_3b.push(pp);
_20(_38,pp,_39);
_1(_38);
_3a.onAdd.call(_38,_39.title,_3b.length-1);
if(_39.selected){
_29(_38,_3b.length-1);
}
};
function _3c(_3d,_3e){
var _3f=$.data(_3d,"accordion").options;
var _40=$.data(_3d,"accordion").panels;
_35(_3d);
var _41=_11(_3d,_3e);
var _42=_41.panel("options").title;
var _43=_d(_3d,_41);
if(_3f.onBeforeRemove.call(_3d,_42,_43)==false){
return;
}
var _41=_11(_3d,_3e,true);
if(_41){
_41.panel("destroy");
if(_40.length){
_1(_3d);
var _44=_9(_3d);
if(!_44){
_29(_3d,0);
}
}
}
_3f.onRemove.call(_3d,_42,_43);
};
$.fn.accordion=function(_45,_46){
if(typeof _45=="string"){
return $.fn.accordion.methods[_45](this,_46);
}
_45=_45||{};
return this.each(function(){
var _47=$.data(this,"accordion");
var _48;
if(_47){
_48=$.extend(_47.options,_45);
_47.opts=_48;
}else{
_48=$.extend({},$.fn.accordion.defaults,$.fn.accordion.parseOptions(this),_45);
var r=_1a(this);
$.data(this,"accordion",{options:_48,accordion:r.accordion,panels:r.panels});
}
_17(this);
_1(this);
_2e(this);
});
};
$.fn.accordion.methods={options:function(jq){
return $.data(jq[0],"accordion").options;
},panels:function(jq){
return $.data(jq[0],"accordion").panels;
},resize:function(jq){
return jq.each(function(){
_1(this);
});
},getSelected:function(jq){
return _9(jq[0]);
},getPanel:function(jq,_49){
return _11(jq[0],_49);
},getPanelIndex:function(jq,_4a){
return _d(jq[0],_4a);
},select:function(jq,_4b){
return jq.each(function(){
_29(this,_4b);
});
},add:function(jq,_4c){
return jq.each(function(){
add(this,_4c);
});
},remove:function(jq,_4d){
return jq.each(function(){
_3c(this,_4d);
});
}};
$.fn.accordion.parseOptions=function(_4e){
var t=$(_4e);
return $.extend({},$.parser.parseOptions(_4e,["width","height",{fit:"boolean",border:"boolean",animate:"boolean"}]));
};
$.fn.accordion.defaults={width:"auto",height:"auto",fit:false,border:true,animate:true,onSelect:function(_4f,_50){
},onAdd:function(_51,_52){
},onBeforeRemove:function(_53,_54){
},onRemove:function(_55,_56){
}};
})(jQuery);

