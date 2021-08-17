import {Feature} from './feature';
import Dom from '../dom';
import Types from '../types';

var global = window;

export class StatusBar extends Feature{

    /**
     * Status bar UI component
     * @param {Object} tf TableFilter instance
     */
    constructor(tf){
        super(tf, 'statusBar');

        // Configuration object
        var f = this.config;

        //id of custom container element
        this.statusBarTgtId = f.status_bar_target_id || null;
        //element containing status bar label
        this.statusBarDiv = null;
        //status bar
        this.statusBarSpan = null;
        //status bar label
        this.statusBarSpanText = null;
        //defines status bar text
        this.statusBarText = f.status_bar_text || '';
        //defines css class status bar
        this.statusBarCssClass = f.status_bar_css_class || 'status';
        //delay for status bar clearing
        this.statusBarCloseDelay =  250;

        //calls function before message is displayed
        this.onBeforeShowMsg = Types.isFn(f.on_before_show_msg) ?
            f.on_before_show_msg : null;
        //calls function after message is displayed
        this.onAfterShowMsg = Types.isFn(f.on_after_show_msg) ?
            f.on_after_show_msg : null;

        // status bar div
        this.prfxStatus = 'status_';
        // status bar label
        this.prfxStatusSpan = 'statusSpan_';
        // text preceding status bar label
        this.prfxStatusTxt = 'statusText_';
    }

    init(){
        if(this.initialized){
            return;
        }

        var tf = this.tf;

        //status bar container
        var statusDiv = Dom.create('div', ['id', this.prfxStatus+tf.id]);
        statusDiv.className = this.statusBarCssClass;

        //status bar label
        var statusSpan = Dom.create('span', ['id', this.prfxStatusSpan+tf.id]);
        //preceding text
        var statusSpanText = Dom.create('span',
            ['id', this.prfxStatusTxt+tf.id]);
        statusSpanText.appendChild(Dom.text(this.statusBarText));

        // target element container
        if(!this.statusBarTgtId){
            tf.setToolbar();
        }
        var targetEl = (!this.statusBarTgtId) ?
                tf.lDiv : Dom.id(this.statusBarTgtId);

        //default container: 'lDiv'
        if(!this.statusBarTgtId){
            statusDiv.appendChild(statusSpanText);
            statusDiv.appendChild(statusSpan);
            targetEl.appendChild(statusDiv);
        } else {
            // custom container, no need to append statusDiv
            targetEl.appendChild(statusSpanText);
            targetEl.appendChild(statusSpan);
        }

        this.statusBarDiv = statusDiv;
        this.statusBarSpan = statusSpan;
        this.statusBarSpanText = statusSpanText;

        this.initialized = true;
    }

    message(t=''){
        if(!this.isEnabled()){
            return;
        }

        if(this.onBeforeShowMsg){
            this.onBeforeShowMsg.call(null, this.tf, t);
        }

        var d = t==='' ? this.statusBarCloseDelay : 1;
        global.setTimeout(() => {
            this.statusBarSpan.innerHTML = t;
            if(this.onAfterShowMsg){
                this.onAfterShowMsg.call(null, this.tf, t);
            }
        }, d);
    }

    destroy(){
        if(!this.initialized){
            return;
        }

        this.statusBarDiv.innerHTML = '';
        this.statusBarDiv.parentNode.removeChild(this.statusBarDiv);
        this.statusBarSpan = null;
        this.statusBarSpanText = null;
        this.statusBarDiv = null;

        this.disable();
        this.initialized = false;
    }

}
