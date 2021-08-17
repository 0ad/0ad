import Dom from '../../dom';
import Types from '../../types';
import Event from '../../event';

export default class FiltersVisibility{

    /**
     * Filters Row Visibility extension
     * @param {Object} tf TableFilter instance
     * @param {Object} f Config
     */
    constructor(tf, f){

        this.initialized = false;
        this.name = f.name;
        this.desc = f.description || 'Filters row visibility manager';

        // Path and image filenames
        this.stylesheet = f.stylesheet || 'filtersVisibility.css';
        this.icnExpand = f.expand_icon_name || 'icn_exp.png';
        this.icnCollapse = f.collapse_icon_name || 'icn_clp.png';

        //expand/collapse filters span element
        this.contEl = null;
        //expand/collapse filters btn element
        this.btnEl = null;

        this.icnExpandHtml = '<img src="'+ tf.themesPath + this.icnExpand +
            '" alt="Expand filters" >';
        this.icnCollapseHtml = '<img src="'+ tf.themesPath + this.icnCollapse +
            '" alt="Collapse filters" >';
        this.defaultText = 'Toggle filters';

        //id of container element
        this.targetId =  f.target_id || null;
        //enables/disables expand/collapse icon
        this.enableIcon = f.enable_icon===false ? false : true;
        this.btnText = f.btn_text || '';

        //defines expand/collapse filters text
        this.collapseBtnHtml = this.enableIcon ?
            this.icnCollapseHtml + this.btnText :
            this.btnText || this.defaultText;
        this.expandBtnHtml =  this.enableIcon ?
            this.icnExpandHtml + this.btnText :
            this.btnText || this.defaultText;

        //defines expand/collapse filters button innerHtml
        this.btnHtml = f.btn_html || null;
        //defines css class for expand/collapse filters button
        this.btnCssClass = f.btn_css_class || 'btnExpClpFlt';
        //defines css class span containing expand/collapse filters
        this.contCssClass = f.cont_css_class || 'expClpFlt';
        this.filtersRowIndex = !Types.isUndef(f.filters_row_index) ?
                f.filters_row_index : tf.getFiltersRowIndex();

        this.visibleAtStart = !Types.isUndef(f.visible_at_start) ?
            Boolean(f.visible_at_start) : true;

        // Prefix
        this.prfx = 'fltsVis_';

        //callback before filters row is shown
        this.onBeforeShow = Types.isFn(f.on_before_show) ?
            f.on_before_show : null;
        //callback after filters row is shown
        this.onAfterShow = Types.isFn(f.on_after_show) ?
            f.on_after_show : null;
        //callback before filters row is hidden
        this.onBeforeHide = Types.isFn(f.on_before_hide) ?
            f.on_before_hide : null;
        //callback after filters row is hidden
        this.onAfterHide = Types.isFn(f.on_after_hide) ? f.on_after_hide : null;

        //Loads extension stylesheet
        tf.import(f.name+'Style', tf.stylePath + this.stylesheet, null, 'link');

        this.tf = tf;
    }

    /**
     * Initialise extension
     */
    init(){
        if(this.initialized){
            return;
        }

        this.buildUI();
        this.initialized = true;
    }

    /**
     * Build UI elements
     */
    buildUI(){
        let tf = this.tf;
        let span = Dom.create('span',['id', this.prfx+tf.id]);
        span.className = this.contCssClass;

        //Container element (rdiv or custom element)
        if(!this.targetId){
            tf.setToolbar();
        }
        let targetEl = !this.targetId ? tf.rDiv : Dom.id(this.targetId);

        if(!this.targetId){
            let firstChild = targetEl.firstChild;
            firstChild.parentNode.insertBefore(span, firstChild);
        } else {
            targetEl.appendChild(span);
        }

        let btn;
        if(!this.btnHtml){
            btn = Dom.create('a', ['href', 'javascript:void(0);']);
            btn.className = this.btnCssClass;
            btn.title = this.btnText || this.defaultText;
            btn.innerHTML = this.collapseBtnHtml;
            span.appendChild(btn);
        } else { //Custom html
            span.innerHTML = this.btnHtml;
            btn = span.firstChild;
        }

        Event.add(btn, 'click', ()=> this.toggle());

        this.contEl = span;
        this.btnEl = btn;

        if(!this.visibleAtStart){
            this.toggle();
        }
    }

    /**
     * Toggle filters visibility
     */
    toggle(){
        let tf = this.tf;
        let tbl = tf.gridLayout? tf.feature('gridLayout').headTbl : tf.tbl;
        let fltRow = tbl.rows[this.filtersRowIndex];
        let fltRowDisplay = fltRow.style.display;

        if(this.onBeforeShow && fltRowDisplay !== ''){
            this.onBeforeShow.call(this, this);
        }
        if(this.onBeforeHide && fltRowDisplay === ''){
            this.onBeforeHide.call(null, this);
        }

        fltRow.style.display = fltRowDisplay==='' ? 'none' : '';
        if(this.enableIcon && !this.btnHtml){
            this.btnEl.innerHTML = fltRowDisplay === '' ?
                this.expandBtnHtml : this.collapseBtnHtml;
        }

        if(this.onAfterShow && fltRowDisplay !== ''){
            this.onAfterShow.call(null, this);
        }
        if(this.onAfterHide && fltRowDisplay === ''){
            this.onAfterHide.call(null, this);
        }
    }

    /**
     * Destroy the UI
     */
    destroy(){
        if(!this.btnEl && !this.contEl){
            return;
        }

        this.btnEl.innerHTML = '';
        this.btnEl.parentNode.removeChild(this.btnEl);
        this.btnEl = null;

        this.contEl.innerHTML = '';
        this.contEl.parentNode.removeChild(this.contEl);
        this.contEl = null;
        this.initialized = false;
    }

}
