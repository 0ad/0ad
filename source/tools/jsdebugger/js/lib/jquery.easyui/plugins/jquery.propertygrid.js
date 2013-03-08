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
var _1;
function _2(_3){
var _4=$.data(_3,"propertygrid");
var _5=$.data(_3,"propertygrid").options;
$(_3).datagrid($.extend({},_5,{cls:"propertygrid",view:(_5.showGroup?_6:undefined),onClickRow:function(_7,_8){
if(_1!=this){
_c(_1);
_1=this;
}
if(_5.editIndex!=_7&&_8.editor){
var _9=$(this).datagrid("getColumnOption","value");
_9.editor=_8.editor;
_c(_1);
$(this).datagrid("beginEdit",_7);
$(this).datagrid("getEditors",_7)[0].target.focus();
_5.editIndex=_7;
}
_5.onClickRow.call(_3,_7,_8);
},loadFilter:function(_a){
_c(this);
return _5.loadFilter.call(this,_a);
},onLoadSuccess:function(_b){
$(_3).datagrid("getPanel").find("div.datagrid-group").attr("style","");
_5.onLoadSuccess.call(_3,_b);
}}));
$(document).unbind(".propertygrid").bind("mousedown.propertygrid",function(e){
var p=$(e.target).closest("div.datagrid-view,div.combo-panel");
if(p.length){
return;
}
_c(_1);
_1=undefined;
});
};
function _c(_d){
var t=$(_d);
if(!t.length){
return;
}
var _e=$.data(_d,"propertygrid").options;
var _f=_e.editIndex;
if(_f==undefined){
return;
}
var ed=t.datagrid("getEditors",_f)[0];
if(ed){
ed.target.blur();
if(t.datagrid("validateRow",_f)){
t.datagrid("endEdit",_f);
}else{
t.datagrid("cancelEdit",_f);
}
}
_e.editIndex=undefined;
};
$.fn.propertygrid=function(_10,_11){
if(typeof _10=="string"){
var _12=$.fn.propertygrid.methods[_10];
if(_12){
return _12(this,_11);
}else{
return this.datagrid(_10,_11);
}
}
_10=_10||{};
return this.each(function(){
var _13=$.data(this,"propertygrid");
if(_13){
$.extend(_13.options,_10);
}else{
var _14=$.extend({},$.fn.propertygrid.defaults,$.fn.propertygrid.parseOptions(this),_10);
_14.frozenColumns=$.extend(true,[],_14.frozenColumns);
_14.columns=$.extend(true,[],_14.columns);
$.data(this,"propertygrid",{options:_14});
}
_2(this);
});
};
$.fn.propertygrid.methods={options:function(jq){
return $.data(jq[0],"propertygrid").options;
}};
$.fn.propertygrid.parseOptions=function(_15){
var t=$(_15);
return $.extend({},$.fn.datagrid.parseOptions(_15),$.parser.parseOptions(_15,[{showGroup:"boolean"}]));
};
var _6=$.extend({},$.fn.datagrid.defaults.view,{render:function(_16,_17,_18){
var _19=$.data(_16,"datagrid");
var _1a=_19.options;
var _1b=_19.data.rows;
var _1c=$(_16).datagrid("getColumnFields",_18);
var _1d=[];
var _1e=0;
var _1f=this.groups;
for(var i=0;i<_1f.length;i++){
var _20=_1f[i];
_1d.push("<div class=\"datagrid-group\" group-index="+i+" style=\"height:25px;overflow:hidden;border-bottom:1px solid #ccc;\">");
_1d.push("<table cellspacing=\"0\" cellpadding=\"0\" border=\"0\" style=\"height:100%\"><tbody>");
_1d.push("<tr>");
_1d.push("<td style=\"border:0;\">");
if(!_18){
_1d.push("<span style=\"color:#666;font-weight:bold;\">");
_1d.push(_1a.groupFormatter.call(_16,_20.fvalue,_20.rows));
_1d.push("</span>");
}
_1d.push("</td>");
_1d.push("</tr>");
_1d.push("</tbody></table>");
_1d.push("</div>");
_1d.push("<table class=\"datagrid-btable\" cellspacing=\"0\" cellpadding=\"0\" border=\"0\"><tbody>");
for(var j=0;j<_20.rows.length;j++){
var cls=(_1e%2&&_1a.striped)?"class=\"datagrid-row datagrid-row-alt\"":"class=\"datagrid-row\"";
var _21=_1a.rowStyler?_1a.rowStyler.call(_16,_1e,_20.rows[j]):"";
var _22=_21?"style=\""+_21+"\"":"";
var _23=_19.rowIdPrefix+"-"+(_18?1:2)+"-"+_1e;
_1d.push("<tr id=\""+_23+"\" datagrid-row-index=\""+_1e+"\" "+cls+" "+_22+">");
_1d.push(this.renderRow.call(this,_16,_1c,_18,_1e,_20.rows[j]));
_1d.push("</tr>");
_1e++;
}
_1d.push("</tbody></table>");
}
$(_17).html(_1d.join(""));
},onAfterRender:function(_24){
var _25=$.data(_24,"datagrid").options;
var dc=$.data(_24,"datagrid").dc;
var _26=dc.view;
var _27=dc.view1;
var _28=dc.view2;
$.fn.datagrid.defaults.view.onAfterRender.call(this,_24);
if(_25.rownumbers||_25.frozenColumns.length){
var _29=_27.find("div.datagrid-group");
}else{
var _29=_28.find("div.datagrid-group");
}
$("<td style=\"border:0;text-align:center;width:25px\"><span class=\"datagrid-row-expander datagrid-row-collapse\" style=\"display:inline-block;width:16px;height:16px;cursor:pointer\">&nbsp;</span></td>").insertBefore(_29.find("td"));
_26.find("div.datagrid-group").each(function(){
var _2a=$(this).attr("group-index");
$(this).find("span.datagrid-row-expander").bind("click",{groupIndex:_2a},function(e){
if($(this).hasClass("datagrid-row-collapse")){
$(_24).datagrid("collapseGroup",e.data.groupIndex);
}else{
$(_24).datagrid("expandGroup",e.data.groupIndex);
}
});
});
},onBeforeRender:function(_2b,_2c){
var _2d=$.data(_2b,"datagrid").options;
var _2e=[];
for(var i=0;i<_2c.length;i++){
var row=_2c[i];
var _2f=_30(row[_2d.groupField]);
if(!_2f){
_2f={fvalue:row[_2d.groupField],rows:[row],startRow:i};
_2e.push(_2f);
}else{
_2f.rows.push(row);
}
}
function _30(_31){
for(var i=0;i<_2e.length;i++){
var _32=_2e[i];
if(_32.fvalue==_31){
return _32;
}
}
return null;
};
this.groups=_2e;
var _33=[];
for(var i=0;i<_2e.length;i++){
var _2f=_2e[i];
for(var j=0;j<_2f.rows.length;j++){
_33.push(_2f.rows[j]);
}
}
$.data(_2b,"datagrid").data.rows=_33;
}});
$.extend($.fn.datagrid.methods,{expandGroup:function(jq,_34){
return jq.each(function(){
var _35=$.data(this,"datagrid").dc.view;
if(_34!=undefined){
var _36=_35.find("div.datagrid-group[group-index=\""+_34+"\"]");
}else{
var _36=_35.find("div.datagrid-group");
}
var _37=_36.find("span.datagrid-row-expander");
if(_37.hasClass("datagrid-row-expand")){
_37.removeClass("datagrid-row-expand").addClass("datagrid-row-collapse");
_36.next("table").show();
}
$(this).datagrid("fixRowHeight");
});
},collapseGroup:function(jq,_38){
return jq.each(function(){
var _39=$.data(this,"datagrid").dc.view;
if(_38!=undefined){
var _3a=_39.find("div.datagrid-group[group-index=\""+_38+"\"]");
}else{
var _3a=_39.find("div.datagrid-group");
}
var _3b=_3a.find("span.datagrid-row-expander");
if(_3b.hasClass("datagrid-row-collapse")){
_3b.removeClass("datagrid-row-collapse").addClass("datagrid-row-expand");
_3a.next("table").hide();
}
$(this).datagrid("fixRowHeight");
});
}});
$.fn.propertygrid.defaults=$.extend({},$.fn.datagrid.defaults,{singleSelect:true,remoteSort:false,fitColumns:true,loadMsg:"",frozenColumns:[[{field:"f",width:16,resizable:false}]],columns:[[{field:"name",title:"Name",width:100,sortable:true},{field:"value",title:"Value",width:100,resizable:false}]],showGroup:false,groupField:"group",groupFormatter:function(_3c,_3d){
return _3c;
}});
})(jQuery);

