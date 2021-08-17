import {Feature} from './feature';
import Types from '../types';
import Dom from '../dom';
import Event from '../event';

export class PopupFilter extends Feature{

    /**
     * Pop-up filter component
     * @param {Object} tf TableFilter instance
     */
    constructor(tf){
        super(tf, 'popupFilters');

        // Configuration object
        var f = this.config;

        // Enable external filters behaviour
        tf.isExternalFlt = true;
        tf.externalFltTgtIds = [];

        //filter icon path
        this.popUpImgFlt = f.popup_filters_image ||
            tf.themesPath+'icn_filter.gif';
        //active filter icon path
        this.popUpImgFltActive = f.popup_filters_image_active ||
            tf.themesPath+'icn_filterActive.gif';
        this.popUpImgFltHtml = f.popup_filters_image_html ||
            '<img src="'+ this.popUpImgFlt +'" alt="Column filter" />';
        //defines css class for popup div containing filter
        this.popUpDivCssClass = f.popup_div_css_class || 'popUpFilter';
        //callback function before popup filtes is opened
        this.onBeforePopUpOpen = Types.isFn(f.on_before_popup_filter_open) ?
            f.on_before_popup_filter_open : null;
        //callback function after popup filtes is opened
        this.onAfterPopUpOpen = Types.isFn(f.on_after_popup_filter_open) ?
            f.on_after_popup_filter_open : null;
        //callback function before popup filtes is closed
        this.onBeforePopUpClose =
            Types.isFn(f.on_before_popup_filter_close) ?
            f.on_before_popup_filter_close : null;
        //callback function after popup filtes is closed
        this.onAfterPopUpClose = Types.isFn(f.on_after_popup_filter_close) ?
            f.on_after_popup_filter_close : null;

        //stores filters spans
        this.popUpFltSpans = [];
        //stores filters icons
        this.popUpFltImgs = [];
        //stores filters containers
        this.popUpFltElms = this.popUpFltElmCache || [];
        this.popUpFltAdjustToContainer = true;

        //id prefix for pop-up filter span
        this.prfxPopUpSpan = 'popUpSpan_';
        //id prefix for pop-up div containing filter
        this.prfxPopUpDiv = 'popUpDiv_';
    }

    onClick(e){
        var evt = e || global.event,
            elm = evt.target.parentNode,
            colIndex = parseInt(elm.getAttribute('ci'), 10);

        this.closeAll(colIndex);
        this.toggle(colIndex);

        if(this.popUpFltAdjustToContainer){
            var popUpDiv = this.popUpFltElms[colIndex],
                header = this.tf.getHeaderElement(colIndex),
                headerWidth = header.clientWidth * 0.95;
            popUpDiv.style.width = parseInt(headerWidth, 10)  + 'px';
        }
        Event.cancel(evt);
        Event.stop(evt);
    }

    /**
     * Initialize DOM elements
     */
    init(){
        if(this.initialized){
            return;
        }

        var tf = this.tf;
        for(var i=0; i<tf.nbCells; i++){
            if(tf.getFilterType(i) === tf.fltTypeNone){
                continue;
            }
            var popUpSpan = Dom.create(
                'span',
                ['id', this.prfxPopUpSpan+tf.id+'_'+i],
                ['ci', i]
            );
            popUpSpan.innerHTML = this.popUpImgFltHtml;
            var header = tf.getHeaderElement(i);
            header.appendChild(popUpSpan);
            Event.add(popUpSpan, 'click', (evt) => { this.onClick(evt); });
            this.popUpFltSpans[i] = popUpSpan;
            this.popUpFltImgs[i] = popUpSpan.firstChild;
        }

        this.initialized = true;
    }

    /**
     * Reset previously destroyed feature
     */
    reset(){
        this.enable();
        this.init();
        this.buildAll();
    }

    /**
     * Build all pop-up filters elements
     */
    buildAll(){
        for(var i=0; i<this.popUpFltElmCache.length; i++){
            this.build(i, this.popUpFltElmCache[i]);
        }
    }

    /**
     * Build a specified pop-up filter elements
     * @param  {Number} colIndex Column index
     * @param  {Object} div      Optional container DOM element
     */
    build(colIndex, div){
        var tf = this.tf;
        var popUpDiv = !div ?
            Dom.create('div', ['id', this.prfxPopUpDiv+tf.id+'_'+colIndex]) :
            div;
        popUpDiv.className = this.popUpDivCssClass;
        tf.externalFltTgtIds.push(popUpDiv.id);
        var header = tf.getHeaderElement(colIndex);
        header.insertBefore(popUpDiv, header.firstChild);
        Event.add(popUpDiv, 'click', (evt) => { Event.stop(evt); });
        this.popUpFltElms[colIndex] = popUpDiv;
    }

    /**
     * Toogle visibility of specified filter
     * @param  {Number} colIndex Column index
     */
    toggle(colIndex){
        var tf = this.tf,
            popUpFltElm = this.popUpFltElms[colIndex];

        if(popUpFltElm.style.display === 'none' ||
            popUpFltElm.style.display === ''){
            if(this.onBeforePopUpOpen){
                this.onBeforePopUpOpen.call(
                    null, this, this.popUpFltElms[colIndex], colIndex);
            }
            popUpFltElm.style.display = 'block';
            if(tf.getFilterType(colIndex) === tf.fltTypeInp){
                var flt = tf.getFilterElement(colIndex);
                if(flt){
                    flt.focus();
                }
            }
            if(this.onAfterPopUpOpen){
                this.onAfterPopUpOpen.call(
                    null, this, this.popUpFltElms[colIndex], colIndex);
            }
        } else {
            if(this.onBeforePopUpClose){
                this.onBeforePopUpClose.call(
                    null, this, this.popUpFltElms[colIndex], colIndex);
            }
            popUpFltElm.style.display = 'none';
            if(this.onAfterPopUpClose){
                this.onAfterPopUpClose.call(
                    null, this, this.popUpFltElms[colIndex], colIndex);
            }
        }
    }

    /**
     * Close all filters excepted for the specified one if any
     * @param  {Number} exceptIdx Column index of the filter to not close
     */
    closeAll(exceptIdx){
        for(var i=0; i<this.popUpFltElms.length; i++){
            if(i === exceptIdx){
                continue;
            }
            var popUpFltElm = this.popUpFltElms[i];
            if(popUpFltElm){
                popUpFltElm.style.display = 'none';
            }
        }
    }

    /**
     * Build all the icons representing the pop-up filters
     */
    buildIcons(){
        for(var i=0; i<this.popUpFltImgs.length; i++){
            this.buildIcon(i, false);
        }
    }

    /**
     * Build specified icon
     * @param  {Number} colIndex Column index
     * @param  {Boolean} active   Apply active state
     */
    buildIcon(colIndex, active){
        if(this.popUpFltImgs[colIndex]){
            this.popUpFltImgs[colIndex].src = active ?
                this.popUpImgFltActive : this.popUpImgFlt;
        }
    }

    /**
     * Remove pop-up filters
     */
    destroy(){
        if(!this.initialized){
            return;
        }

        this.popUpFltElmCache = [];
        for(var i=0; i<this.popUpFltElms.length; i++){
            var popUpFltElm = this.popUpFltElms[i],
                popUpFltSpan = this.popUpFltSpans[i],
                popUpFltImg = this.popUpFltImgs[i];
            if(popUpFltElm){
                popUpFltElm.parentNode.removeChild(popUpFltElm);
                this.popUpFltElmCache[i] = popUpFltElm;
            }
            popUpFltElm = null;
            if(popUpFltSpan){
                popUpFltSpan.parentNode.removeChild(popUpFltSpan);
            }
            popUpFltSpan = null;
            if(popUpFltImg){
                popUpFltImg.parentNode.removeChild(popUpFltImg);
            }
            popUpFltImg = null;
        }
        this.popUpFltElms = [];
        this.popUpFltSpans = [];
        this.popUpFltImgs = [];

        this.disable();
        this.initialized = false;
    }

}
