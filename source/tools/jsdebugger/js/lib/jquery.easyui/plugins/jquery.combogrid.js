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
var _3=$.data(_2,"combogrid").options;
var _4=$.data(_2,"combogrid").grid;
$(_2).addClass("combogrid-f");
$(_2).combo(_3);
var _5=$(_2).combo("panel");
if(!_4){
_4=$("<table></table>").appendTo(_5);
$.data(_2,"combogrid").grid=_4;
}
_4.datagrid($.extend({},_3,{border:false,fit:true,singleSelect:(!_3.multiple),onLoadSuccess:function(_6){
var _7=$.data(_2,"combogrid").remainText;
var _8=$(_2).combo("getValues");
_1c(_2,_8,_7);
_3.onLoadSuccess.apply(_2,arguments);
},onClickRow:_9,onSelect:function(_a,_b){
_c();
_3.onSelect.call(this,_a,_b);
},onUnselect:function(_d,_e){
_c();
_3.onUnselect.call(this,_d,_e);
},onSelectAll:function(_f){
_c();
_3.onSelectAll.call(this,_f);
},onUnselectAll:function(_10){
if(_3.multiple){
_c();
}
_3.onUnselectAll.call(this,_10);
}}));
function _9(_11,row){
$.data(_2,"combogrid").remainText=false;
_c();
if(!_3.multiple){
$(_2).combo("hidePanel");
}
_3.onClickRow.call(this,_11,row);
};
function _c(){
var _12=$.data(_2,"combogrid").remainText;
var _13=_4.datagrid("getSelections");
var vv=[],ss=[];
for(var i=0;i<_13.length;i++){
vv.push(_13[i][_3.idField]);
ss.push(_13[i][_3.textField]);
}
if(!_3.multiple){
$(_2).combo("setValues",(vv.length?vv:[""]));
}else{
$(_2).combo("setValues",vv);
}
if(!_12){
$(_2).combo("setText",ss.join(_3.separator));
}
};
};
function _14(_15,_16){
var _17=$.data(_15,"combogrid").options;
var _18=$.data(_15,"combogrid").grid;
var _19=_18.datagrid("getRows").length;
if(!_19){
return;
}
$.data(_15,"combogrid").remainText=false;
var _1a;
var _1b=_18.datagrid("getSelections");
if(_1b.length){
_1a=_18.datagrid("getRowIndex",_1b[_1b.length-1][_17.idField]);
_1a+=_16;
if(_1a<0){
_1a=0;
}
if(_1a>=_19){
_1a=_19-1;
}
}else{
if(_16>0){
_1a=0;
}else{
if(_16<0){
_1a=_19-1;
}else{
_1a=-1;
}
}
}
if(_1a>=0){
_18.datagrid("clearSelections");
_18.datagrid("selectRow",_1a);
}
};
function _1c(_1d,_1e,_1f){
var _20=$.data(_1d,"combogrid").options;
var _21=$.data(_1d,"combogrid").grid;
var _22=_21.datagrid("getRows");
var ss=[];
for(var i=0;i<_1e.length;i++){
var _23=_21.datagrid("getRowIndex",_1e[i]);
if(_23>=0){
_21.datagrid("selectRow",_23);
ss.push(_22[_23][_20.textField]);
}else{
ss.push(_1e[i]);
}
}
if($(_1d).combo("getValues").join(",")==_1e.join(",")){
return;
}
$(_1d).combo("setValues",_1e);
if(!_1f){
$(_1d).combo("setText",ss.join(_20.separator));
}
};
function _24(_25,q){
var _26=$.data(_25,"combogrid").options;
var _27=$.data(_25,"combogrid").grid;
$.data(_25,"combogrid").remainText=true;
if(_26.multiple&&!q){
_1c(_25,[],true);
}else{
_1c(_25,[q],true);
}
if(_26.mode=="remote"){
_27.datagrid("clearSelections");
_27.datagrid("load",$.extend({},_26.queryParams,{q:q}));
}else{
if(!q){
return;
}
var _28=_27.datagrid("getRows");
for(var i=0;i<_28.length;i++){
if(_26.filter.call(_25,q,_28[i])){
_27.datagrid("clearSelections");
_27.datagrid("selectRow",i);
return;
}
}
}
};
$.fn.combogrid=function(_29,_2a){
if(typeof _29=="string"){
var _2b=$.fn.combogrid.methods[_29];
if(_2b){
return _2b(this,_2a);
}else{
return $.fn.combo.methods[_29](this,_2a);
}
}
_29=_29||{};
return this.each(function(){
var _2c=$.data(this,"combogrid");
if(_2c){
$.extend(_2c.options,_29);
}else{
_2c=$.data(this,"combogrid",{options:$.extend({},$.fn.combogrid.defaults,$.fn.combogrid.parseOptions(this),_29)});
}
_1(this);
});
};
$.fn.combogrid.methods={options:function(jq){
var _2d=$.data(jq[0],"combogrid").options;
_2d.originalValue=jq.combo("options").originalValue;
return _2d;
},grid:function(jq){
return $.data(jq[0],"combogrid").grid;
},setValues:function(jq,_2e){
return jq.each(function(){
_1c(this,_2e);
});
},setValue:function(jq,_2f){
return jq.each(function(){
_1c(this,[_2f]);
});
},clear:function(jq){
return jq.each(function(){
$(this).combogrid("grid").datagrid("clearSelections");
$(this).combo("clear");
});
},reset:function(jq){
return jq.each(function(){
var _30=$(this).combogrid("options");
if(_30.multiple){
$(this).combogrid("setValues",_30.originalValue);
}else{
$(this).combogrid("setValue",_30.originalValue);
}
});
}};
$.fn.combogrid.parseOptions=function(_31){
var t=$(_31);
return $.extend({},$.fn.combo.parseOptions(_31),$.fn.datagrid.parseOptions(_31),$.parser.parseOptions(_31,["idField","textField","mode"]));
};
$.fn.combogrid.defaults=$.extend({},$.fn.combo.defaults,$.fn.datagrid.defaults,{loadMsg:null,idField:null,textField:null,mode:"local",keyHandler:{up:function(){
_14(this,-1);
},down:function(){
_14(this,1);
},enter:function(){
_14(this,0);
$(this).combo("hidePanel");
},query:function(q){
_24(this,q);
}},filter:function(q,row){
var _32=$(this).combogrid("options");
return row[_32.textField].indexOf(q)==0;
}});
})(jQuery);

