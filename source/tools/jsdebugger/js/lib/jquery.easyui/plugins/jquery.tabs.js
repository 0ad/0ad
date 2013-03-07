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
var _3=$.data(_2,"tabs").options;
if(_3.tabPosition=="left"||_3.tabPosition=="right"){
return;
}
var _4=$(_2).children("div.tabs-header");
var _5=_4.children("div.tabs-tool");
var _6=_4.children("div.tabs-scroller-left");
var _7=_4.children("div.tabs-scroller-right");
var _8=_4.children("div.tabs-wrap");
_5._outerHeight(_4.outerHeight()-(_3.plain?2:0));
var _9=0;
$("ul.tabs li",_4).each(function(){
_9+=$(this).outerWidth(true);
});
var _a=_4.width()-_5._outerWidth();
if(_9>_a){
_6.show();
_7.show();
if(_3.toolPosition=="left"){
_5.css({left:_6.outerWidth(),right:""});
_8.css({marginLeft:_6.outerWidth()+_5._outerWidth(),marginRight:_7._outerWidth(),width:_a-_6.outerWidth()-_7.outerWidth()});
}else{
_5.css({left:"",right:_7.outerWidth()});
_8.css({marginLeft:_6.outerWidth(),marginRight:_7.outerWidth()+_5._outerWidth(),width:_a-_6.outerWidth()-_7.outerWidth()});
}
}else{
_6.hide();
_7.hide();
if(_3.toolPosition=="left"){
_5.css({left:0,right:""});
_8.css({marginLeft:_5._outerWidth(),marginRight:0,width:_a});
}else{
_5.css({left:"",right:0});
_8.css({marginLeft:0,marginRight:_5._outerWidth(),width:_a});
}
}
};
function _b(_c){
var _d=$.data(_c,"tabs").options;
var _e=$(_c).children("div.tabs-header");
if(_d.tools){
if(typeof _d.tools=="string"){
$(_d.tools).addClass("tabs-tool").appendTo(_e);
$(_d.tools).show();
}else{
_e.children("div.tabs-tool").remove();
var _f=$("<div class=\"tabs-tool\"></div>").appendTo(_e);
for(var i=0;i<_d.tools.length;i++){
var _10=$("<a href=\"javascript:void(0);\"></a>").appendTo(_f);
_10[0].onclick=eval(_d.tools[i].handler||function(){
});
_10.linkbutton($.extend({},_d.tools[i],{plain:true}));
}
}
}else{
_e.children("div.tabs-tool").remove();
}
};
function _11(_12){
var _13=$.data(_12,"tabs").options;
var cc=$(_12);
_13.fit?$.extend(_13,cc._fit()):cc._fit(false);
cc.width(_13.width).height(_13.height);
var _14=$(_12).children("div.tabs-header");
var _15=$(_12).children("div.tabs-panels");
if(_13.tabPosition=="left"||_13.tabPosition=="right"){
_14._outerWidth(_13.headerWidth);
_15._outerWidth(cc.width()-_13.headerWidth);
_14.add(_15)._outerHeight(_13.height);
var _16=_14.find("div.tabs-wrap");
_16._outerWidth(_14.width());
_14.find(".tabs")._outerWidth(_16.width());
}else{
_14.css("height","");
_14.find("div.tabs-wrap").css("width","");
_14.find(".tabs").css("width","");
_14._outerWidth(_13.width);
_1(_12);
var _17=_13.height;
if(!isNaN(_17)){
_15._outerHeight(_17-_14.outerHeight());
}else{
_15.height("auto");
}
var _18=_13.width;
if(!isNaN(_18)){
_15._outerWidth(_18);
}else{
_15.width("auto");
}
}
};
function _19(_1a){
var _1b=$.data(_1a,"tabs").options;
var tab=_1c(_1a);
if(tab){
var _1d=$(_1a).children("div.tabs-panels");
var _1e=_1b.width=="auto"?"auto":_1d.width();
var _1f=_1b.height=="auto"?"auto":_1d.height();
tab.panel("resize",{width:_1e,height:_1f});
}
};
function _20(_21){
var _22=$.data(_21,"tabs").tabs;
var cc=$(_21);
cc.addClass("tabs-container");
cc.wrapInner("<div class=\"tabs-panels\"/>");
$("<div class=\"tabs-header\">"+"<div class=\"tabs-scroller-left\"></div>"+"<div class=\"tabs-scroller-right\"></div>"+"<div class=\"tabs-wrap\">"+"<ul class=\"tabs\"></ul>"+"</div>"+"</div>").prependTo(_21);
cc.children("div.tabs-panels").children("div").each(function(i){
var _23=$.extend({},$.parser.parseOptions(this),{selected:($(this).attr("selected")?true:undefined)});
var pp=$(this);
_22.push(pp);
_2b(_21,pp,_23);
});
cc.children("div.tabs-header").find(".tabs-scroller-left, .tabs-scroller-right").hover(function(){
$(this).addClass("tabs-scroller-over");
},function(){
$(this).removeClass("tabs-scroller-over");
});
cc.bind("_resize",function(e,_24){
var _25=$.data(_21,"tabs").options;
if(_25.fit==true||_24){
_11(_21);
_19(_21);
}
return false;
});
};
function _26(_27){
var _28=$.data(_27,"tabs").options;
var _29=$(_27).children("div.tabs-header");
var _2a=$(_27).children("div.tabs-panels");
_29.removeClass("tabs-header-top tabs-header-bottom tabs-header-left tabs-header-right");
_2a.removeClass("tabs-panels-top tabs-panels-bottom tabs-panels-left tabs-panels-right");
if(_28.tabPosition=="top"){
_29.insertBefore(_2a);
}else{
if(_28.tabPosition=="bottom"){
_29.insertAfter(_2a);
_29.addClass("tabs-header-bottom");
_2a.addClass("tabs-panels-top");
}else{
if(_28.tabPosition=="left"){
_29.addClass("tabs-header-left");
_2a.addClass("tabs-panels-right");
}else{
if(_28.tabPosition=="right"){
_29.addClass("tabs-header-right");
_2a.addClass("tabs-panels-left");
}
}
}
}
if(_28.plain==true){
_29.addClass("tabs-header-plain");
}else{
_29.removeClass("tabs-header-plain");
}
if(_28.border==true){
_29.removeClass("tabs-header-noborder");
_2a.removeClass("tabs-panels-noborder");
}else{
_29.addClass("tabs-header-noborder");
_2a.addClass("tabs-panels-noborder");
}
$(".tabs-scroller-left",_29).unbind(".tabs").bind("click.tabs",function(){
$(_27).tabs("scrollBy",-_28.scrollIncrement);
});
$(".tabs-scroller-right",_29).unbind(".tabs").bind("click.tabs",function(){
$(_27).tabs("scrollBy",_28.scrollIncrement);
});
};
function _2b(_2c,pp,_2d){
var _2e=$.data(_2c,"tabs");
_2d=_2d||{};
pp.panel($.extend({},_2d,{border:false,noheader:true,closed:true,doSize:false,iconCls:(_2d.icon?_2d.icon:undefined),onLoad:function(){
if(_2d.onLoad){
_2d.onLoad.call(this,arguments);
}
_2e.options.onLoad.call(_2c,$(this));
}}));
var _2f=pp.panel("options");
var _30=$(_2c).children("div.tabs-header").find("ul.tabs");
_2f.tab=$("<li></li>").appendTo(_30);
_2f.tab.append("<a href=\"javascript:void(0)\" class=\"tabs-inner\">"+"<span class=\"tabs-title\"></span>"+"<span class=\"tabs-icon\"></span>"+"</a>");
_2f.tab.unbind(".tabs").bind("click.tabs",{p:pp},function(e){
if($(this).hasClass("tabs-disabled")){
return;
}
_37(_2c,_31(_2c,e.data.p));
}).bind("contextmenu.tabs",{p:pp},function(e){
if($(this).hasClass("tabs-disabled")){
return;
}
_2e.options.onContextMenu.call(_2c,e,$(this).find("span.tabs-title").html(),_31(_2c,e.data.p));
});
$(_2c).tabs("update",{tab:pp,options:_2f});
};
function _32(_33,_34){
var _35=$.data(_33,"tabs").options;
var _36=$.data(_33,"tabs").tabs;
if(_34.selected==undefined){
_34.selected=true;
}
var pp=$("<div></div>").appendTo($(_33).children("div.tabs-panels"));
_36.push(pp);
_2b(_33,pp,_34);
_35.onAdd.call(_33,_34.title,_36.length-1);
_1(_33);
if(_34.selected){
_37(_33,_36.length-1);
}
};
function _38(_39,_3a){
var _3b=$.data(_39,"tabs").selectHis;
var pp=_3a.tab;
var _3c=pp.panel("options").title;
pp.panel($.extend({},_3a.options,{iconCls:(_3a.options.icon?_3a.options.icon:undefined)}));
var _3d=pp.panel("options");
var tab=_3d.tab;
var _3e=tab.find("span.tabs-title");
var _3f=tab.find("span.tabs-icon");
_3e.html(_3d.title);
_3f.attr("class","tabs-icon");
tab.find("a.tabs-close").remove();
if(_3d.closable){
_3e.addClass("tabs-closable");
var _40=$("<a href=\"javascript:void(0)\" class=\"tabs-close\"></a>").appendTo(tab);
_40.bind("click.tabs",{p:pp},function(e){
if($(this).parent().hasClass("tabs-disabled")){
return;
}
_42(_39,_31(_39,e.data.p));
return false;
});
}else{
_3e.removeClass("tabs-closable");
}
if(_3d.iconCls){
_3e.addClass("tabs-with-icon");
_3f.addClass(_3d.iconCls);
}else{
_3e.removeClass("tabs-with-icon");
}
if(_3c!=_3d.title){
for(var i=0;i<_3b.length;i++){
if(_3b[i]==_3c){
_3b[i]=_3d.title;
}
}
}
tab.find("span.tabs-p-tool").remove();
if(_3d.tools){
var _41=$("<span class=\"tabs-p-tool\"></span>").insertAfter(tab.find("a.tabs-inner"));
if(typeof _3d.tools=="string"){
$(_3d.tools).children().appendTo(_41);
}else{
for(var i=0;i<_3d.tools.length;i++){
var t=$("<a href=\"javascript:void(0)\"></a>").appendTo(_41);
t.addClass(_3d.tools[i].iconCls);
if(_3d.tools[i].handler){
t.bind("click",{handler:_3d.tools[i].handler},function(e){
if($(this).parents("li").hasClass("tabs-disabled")){
return;
}
e.data.handler.call(this);
});
}
}
}
var pr=_41.children().length*12;
if(_3d.closable){
pr+=8;
}else{
pr-=3;
_41.css("right","5px");
}
_3e.css("padding-right",pr+"px");
}
_1(_39);
$.data(_39,"tabs").options.onUpdate.call(_39,_3d.title,_31(_39,pp));
};
function _42(_43,_44){
var _45=$.data(_43,"tabs").options;
var _46=$.data(_43,"tabs").tabs;
var _47=$.data(_43,"tabs").selectHis;
if(!_48(_43,_44)){
return;
}
var tab=_49(_43,_44);
var _4a=tab.panel("options").title;
var _4b=_31(_43,tab);
if(_45.onBeforeClose.call(_43,_4a,_4b)==false){
return;
}
var tab=_49(_43,_44,true);
tab.panel("options").tab.remove();
tab.panel("destroy");
_45.onClose.call(_43,_4a,_4b);
_1(_43);
for(var i=0;i<_47.length;i++){
if(_47[i]==_4a){
_47.splice(i,1);
i--;
}
}
var _4c=_47.pop();
if(_4c){
_37(_43,_4c);
}else{
if(_46.length){
_37(_43,0);
}
}
};
function _49(_4d,_4e,_4f){
var _50=$.data(_4d,"tabs").tabs;
if(typeof _4e=="number"){
if(_4e<0||_4e>=_50.length){
return null;
}else{
var tab=_50[_4e];
if(_4f){
_50.splice(_4e,1);
}
return tab;
}
}
for(var i=0;i<_50.length;i++){
var tab=_50[i];
if(tab.panel("options").title==_4e){
if(_4f){
_50.splice(i,1);
}
return tab;
}
}
return null;
};
function _31(_51,tab){
var _52=$.data(_51,"tabs").tabs;
for(var i=0;i<_52.length;i++){
if(_52[i][0]==$(tab)[0]){
return i;
}
}
return -1;
};
function _1c(_53){
var _54=$.data(_53,"tabs").tabs;
for(var i=0;i<_54.length;i++){
var tab=_54[i];
if(tab.panel("options").closed==false){
return tab;
}
}
return null;
};
function _55(_56){
var _57=$.data(_56,"tabs").tabs;
for(var i=0;i<_57.length;i++){
if(_57[i].panel("options").selected){
_37(_56,i);
return;
}
}
if(_57.length){
_37(_56,0);
}
};
function _37(_58,_59){
var _5a=$.data(_58,"tabs").options;
var _5b=$.data(_58,"tabs").tabs;
var _5c=$.data(_58,"tabs").selectHis;
if(_5b.length==0){
return;
}
var _5d=_49(_58,_59);
if(!_5d){
return;
}
var _5e=_1c(_58);
if(_5e){
_5e.panel("close");
_5e.panel("options").tab.removeClass("tabs-selected");
}
_5d.panel("open");
var _5f=_5d.panel("options").title;
_5c.push(_5f);
var tab=_5d.panel("options").tab;
tab.addClass("tabs-selected");
var _60=$(_58).find(">div.tabs-header>div.tabs-wrap");
var _61=tab.position().left;
var _62=_61+tab.outerWidth();
if(_61<0||_62>_60.width()){
var _63=_61-(_60.width()-tab.width())/2;
$(_58).tabs("scrollBy",_63);
}else{
$(_58).tabs("scrollBy",0);
}
_19(_58);
_5a.onSelect.call(_58,_5f,_31(_58,_5d));
};
function _48(_64,_65){
return _49(_64,_65)!=null;
};
$.fn.tabs=function(_66,_67){
if(typeof _66=="string"){
return $.fn.tabs.methods[_66](this,_67);
}
_66=_66||{};
return this.each(function(){
var _68=$.data(this,"tabs");
var _69;
if(_68){
_69=$.extend(_68.options,_66);
_68.options=_69;
}else{
$.data(this,"tabs",{options:$.extend({},$.fn.tabs.defaults,$.fn.tabs.parseOptions(this),_66),tabs:[],selectHis:[]});
_20(this);
}
_b(this);
_26(this);
_11(this);
_55(this);
});
};
$.fn.tabs.methods={options:function(jq){
return $.data(jq[0],"tabs").options;
},tabs:function(jq){
return $.data(jq[0],"tabs").tabs;
},resize:function(jq){
return jq.each(function(){
_11(this);
_19(this);
});
},add:function(jq,_6a){
return jq.each(function(){
_32(this,_6a);
});
},close:function(jq,_6b){
return jq.each(function(){
_42(this,_6b);
});
},getTab:function(jq,_6c){
return _49(jq[0],_6c);
},getTabIndex:function(jq,tab){
return _31(jq[0],tab);
},getSelected:function(jq){
return _1c(jq[0]);
},select:function(jq,_6d){
return jq.each(function(){
_37(this,_6d);
});
},exists:function(jq,_6e){
return _48(jq[0],_6e);
},update:function(jq,_6f){
return jq.each(function(){
_38(this,_6f);
});
},enableTab:function(jq,_70){
return jq.each(function(){
$(this).tabs("getTab",_70).panel("options").tab.removeClass("tabs-disabled");
});
},disableTab:function(jq,_71){
return jq.each(function(){
$(this).tabs("getTab",_71).panel("options").tab.addClass("tabs-disabled");
});
},scrollBy:function(jq,_72){
return jq.each(function(){
var _73=$(this).tabs("options");
var _74=$(this).find(">div.tabs-header>div.tabs-wrap");
var pos=Math.min(_74._scrollLeft()+_72,_75());
_74.animate({scrollLeft:pos},_73.scrollDuration);
function _75(){
var w=0;
var ul=_74.children("ul");
ul.children("li").each(function(){
w+=$(this).outerWidth(true);
});
return w-_74.width()+(ul.outerWidth()-ul.width());
};
});
}};
$.fn.tabs.parseOptions=function(_76){
return $.extend({},$.parser.parseOptions(_76,["width","height","tools","toolPosition","tabPosition",{fit:"boolean",border:"boolean",plain:"boolean",headerWidth:"number"}]));
};
$.fn.tabs.defaults={width:"auto",height:"auto",headerWidth:150,plain:false,fit:false,border:true,tools:null,toolPosition:"right",tabPosition:"top",scrollIncrement:100,scrollDuration:400,onLoad:function(_77){
},onSelect:function(_78,_79){
},onBeforeClose:function(_7a,_7b){
},onClose:function(_7c,_7d){
},onAdd:function(_7e,_7f){
},onUpdate:function(_80,_81){
},onContextMenu:function(e,_82,_83){
}};
})(jQuery);

