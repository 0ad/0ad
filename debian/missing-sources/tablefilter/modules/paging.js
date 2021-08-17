import {Feature} from './feature';
import Dom from '../dom';
import Types from '../types';
import Str from '../string';
import Event from '../event';

export class Paging extends Feature{

    /**
     * Pagination component
     * @param {Object} tf TableFilter instance
     */
    constructor(tf){
        super(tf, 'paging');

        // Configuration object
        var f = this.config;

        //css class for paging buttons (previous,next,etc.)
        this.btnPageCssClass = f.paging_btn_css_class || 'pgInp';
        //stores paging select element
        this.pagingSlc = null;
        //results per page select element
        this.resultsPerPageSlc = null;
        //id of container element
        this.pagingTgtId = f.paging_target_id || null;
        //defines table paging length
        this.pagingLength = !isNaN(f.paging_length) ? f.paging_length : 10;
        //id of container element
        this.resultsPerPageTgtId = f.results_per_page_target_id || null;
        //css class for paging select element
        this.pgSlcCssClass = f.paging_slc_css_class || 'pgSlc';
        //css class for paging input element
        this.pgInpCssClass = f.paging_inp_css_class || 'pgNbInp';
        //stores results per page text and values
        this.resultsPerPage = f.results_per_page || null;
        //enables/disables results per page drop-down
        this.hasResultsPerPage = Types.isArray(this.resultsPerPage);
        //defines css class for results per page select
        this.resultsSlcCssClass = f.results_slc_css_class || 'rspg';
        //css class for label preceding results per page select
        this.resultsSpanCssClass = f.results_span_css_class || 'rspgSpan';
        //1st row index of current page
        this.startPagingRow = 0;
        //total nb of pages
        this.nbPages = 0;
        //current page nb
        this.currentPageNb = 1;
        //defines next page button text
        this.btnNextPageText = f.btn_next_page_text || '>';
        //defines previous page button text
        this.btnPrevPageText = f.btn_prev_page_text || '<';
        //defines last page button text
        this.btnLastPageText = f.btn_last_page_text || '>|';
        //defines first page button text
        this.btnFirstPageText = f.btn_first_page_text || '|<';
        //defines next page button html
        this.btnNextPageHtml = f.btn_next_page_html ||
            (!tf.enableIcons ? null :
            '<input type="button" value="" class="'+this.btnPageCssClass +
            ' nextPage" title="Next page" />');
        //defines previous page button html
        this.btnPrevPageHtml = f.btn_prev_page_html ||
            (!tf.enableIcons ? null :
            '<input type="button" value="" class="'+this.btnPageCssClass +
            ' previousPage" title="Previous page" />');
        //defines last page button html
        this.btnFirstPageHtml = f.btn_first_page_html ||
            (!tf.enableIcons ? null :
            '<input type="button" value="" class="'+this.btnPageCssClass +
            ' firstPage" title="First page" />');
        //defines previous page button html
        this.btnLastPageHtml = f.btn_last_page_html ||
            (!tf.enableIcons ? null :
            '<input type="button" value="" class="'+this.btnPageCssClass +
            ' lastPage" title="Last page" />');
        //defines text preceeding page selector drop-down
        this.pageText = f.page_text || ' Page ';
        //defines text after page selector drop-down
        this.ofText = f.of_text || ' of ';
        //css class for span containing tot nb of pages
        this.nbPgSpanCssClass = f.nb_pages_css_class || 'nbpg';
        //enables/disables paging buttons
        this.hasPagingBtns = f.paging_btns===false ? false : true;
        //defines previous page button html
        this.pageSelectorType = f.page_selector_type || tf.fltTypeSlc;
        //calls function before page is changed
        this.onBeforeChangePage = Types.isFn(f.on_before_change_page) ?
            f.on_before_change_page : null;
        //calls function before page is changed
        this.onAfterChangePage = Types.isFn(f.on_after_change_page) ?
            f.on_after_change_page : null;

        //pages select
        this.prfxSlcPages = 'slcPages_';
        //results per page select
        this.prfxSlcResults = 'slcResults_';
        //label preciding results per page select
        this.prfxSlcResultsTxt = 'slcResultsTxt_';
        //span containing next page button
        this.prfxBtnNextSpan = 'btnNextSpan_';
        //span containing previous page button
        this.prfxBtnPrevSpan = 'btnPrevSpan_';
        //span containing last page button
        this.prfxBtnLastSpan = 'btnLastSpan_';
        //span containing first page button
        this.prfxBtnFirstSpan = 'btnFirstSpan_';
        //next button
        this.prfxBtnNext = 'btnNext_';
        //previous button
        this.prfxBtnPrev = 'btnPrev_';
        //last button
        this.prfxBtnLast = 'btnLast_';
        //first button
        this.prfxBtnFirst = 'btnFirst_';
        //span for tot nb pages
        this.prfxPgSpan = 'pgspan_';
        //span preceding pages select (contains 'Page')
        this.prfxPgBeforeSpan = 'pgbeforespan_';
        //span following pages select (contains ' of ')
        this.prfxPgAfterSpan = 'pgafterspan_';

        var start_row = this.refRow;
        var nrows = this.nbRows;
        //calculates page nb
        this.nbPages = Math.ceil((nrows-start_row)/this.pagingLength);

        //Paging elements events
        var o = this;
        // Paging DOM events
        this.evt = {
            slcIndex(){
                return (o.pageSelectorType===tf.fltTypeSlc) ?
                    o.pagingSlc.options.selectedIndex :
                    parseInt(o.pagingSlc.value, 10)-1;
            },
            nbOpts(){
                return (o.pageSelectorType===tf.fltTypeSlc) ?
                    parseInt(o.pagingSlc.options.length, 10)-1 :
                    (o.nbPages-1);
            },
            next(){
                var nextIndex = o.evt.slcIndex() < o.evt.nbOpts() ?
                    o.evt.slcIndex()+1 : 0;
                o.changePage(nextIndex);
            },
            prev(){
                var prevIndex = o.evt.slcIndex()>0 ?
                    o.evt.slcIndex()-1 : o.evt.nbOpts();
                o.changePage(prevIndex);
            },
            last(){
                o.changePage(o.evt.nbOpts());
            },
            first(){
                o.changePage(0);
            },
            _detectKey(e){
                var key = Event.keyCode(e);
                if(key===13){
                    if(tf.sorted){
                        tf.filter();
                        o.changePage(o.evt.slcIndex());
                    } else{
                        o.changePage();
                    }
                    this.blur();
                }
            },
            slcPagesChange: null,
            nextEvt: null,
            prevEvt: null,
            lastEvt: null,
            firstEvt: null
        };
    }

    /**
     * Initialize DOM elements
     */
    init(){
        var slcPages;
        var tf = this.tf;
        var evt = this.evt;

        if(this.initialized){
            return;
        }

        // Check resultsPerPage is in expected format and initialise the
        // results per page component
        if(this.hasResultsPerPage){
            if(this.resultsPerPage.length<2){
                this.hasResultsPerPage = false;
            } else {
                this.pagingLength = this.resultsPerPage[1][0];
                this.setResultsPerPage();
            }
        }

        evt.slcPagesChange = (event) => {
            var slc = event.target;
            this.changePage(slc.selectedIndex);
        };

        // Paging drop-down list selector
        if(this.pageSelectorType === tf.fltTypeSlc){
            slcPages = Dom.create(
                tf.fltTypeSlc, ['id', this.prfxSlcPages+tf.id]);
            slcPages.className = this.pgSlcCssClass;
            Event.add(slcPages, 'change', evt.slcPagesChange);
        }

        // Paging input selector
        if(this.pageSelectorType === tf.fltTypeInp){
            slcPages = Dom.create(
                tf.fltTypeInp,
                ['id', this.prfxSlcPages+tf.id],
                ['value', this.currentPageNb]
            );
            slcPages.className = this.pgInpCssClass;
            Event.add(slcPages, 'keypress', evt._detectKey);
        }

        // btns containers
        var btnNextSpan = Dom.create(
            'span',['id', this.prfxBtnNextSpan+tf.id]);
        var btnPrevSpan = Dom.create(
            'span',['id', this.prfxBtnPrevSpan+tf.id]);
        var btnLastSpan = Dom.create(
            'span',['id', this.prfxBtnLastSpan+tf.id]);
        var btnFirstSpan = Dom.create(
            'span',['id', this.prfxBtnFirstSpan+tf.id]);

        if(this.hasPagingBtns){
            // Next button
            if(!this.btnNextPageHtml){
                var btn_next = Dom.create(
                    tf.fltTypeInp,
                    ['id', this.prfxBtnNext+tf.id],
                    ['type', 'button'],
                    ['value', this.btnNextPageText],
                    ['title', 'Next']
                );
                btn_next.className = this.btnPageCssClass;
                Event.add(btn_next, 'click', evt.next);
                btnNextSpan.appendChild(btn_next);
            } else {
                btnNextSpan.innerHTML = this.btnNextPageHtml;
                Event.add(btnNextSpan, 'click', evt.next);
            }
            // Previous button
            if(!this.btnPrevPageHtml){
                var btn_prev = Dom.create(
                    tf.fltTypeInp,
                    ['id', this.prfxBtnPrev+tf.id],
                    ['type', 'button'],
                    ['value', this.btnPrevPageText],
                    ['title', 'Previous']
                );
                btn_prev.className = this.btnPageCssClass;
                Event.add(btn_prev, 'click', evt.prev);
                btnPrevSpan.appendChild(btn_prev);
            } else {
                btnPrevSpan.innerHTML = this.btnPrevPageHtml;
                Event.add(btnPrevSpan, 'click', evt.prev);
            }
            // Last button
            if(!this.btnLastPageHtml){
                var btn_last = Dom.create(
                    tf.fltTypeInp,
                    ['id', this.prfxBtnLast+tf.id],
                    ['type', 'button'],
                    ['value', this.btnLastPageText],
                    ['title', 'Last']
                );
                btn_last.className = this.btnPageCssClass;
                Event.add(btn_last, 'click', evt.last);
                btnLastSpan.appendChild(btn_last);
            } else {
                btnLastSpan.innerHTML = this.btnLastPageHtml;
                Event.add(btnLastSpan, 'click', evt.last);
            }
            // First button
            if(!this.btnFirstPageHtml){
                var btn_first = Dom.create(
                    tf.fltTypeInp,
                    ['id', this.prfxBtnFirst+tf.id],
                    ['type', 'button'],
                    ['value', this.btnFirstPageText],
                    ['title', 'First']
                );
                btn_first.className = this.btnPageCssClass;
                Event.add(btn_first, 'click', evt.first);
                btnFirstSpan.appendChild(btn_first);
            } else {
                btnFirstSpan.innerHTML = this.btnFirstPageHtml;
                Event.add(btnFirstSpan, 'click', evt.first);
            }
        }

        // paging elements (buttons+drop-down list) are added to defined element
        if(!this.pagingTgtId){
            tf.setToolbar();
        }
        var targetEl = !this.pagingTgtId ? tf.mDiv : Dom.id(this.pagingTgtId);
        targetEl.appendChild(btnFirstSpan);
        targetEl.appendChild(btnPrevSpan);

        var pgBeforeSpan = Dom.create(
            'span',['id', this.prfxPgBeforeSpan+tf.id] );
        pgBeforeSpan.appendChild( Dom.text(this.pageText) );
        pgBeforeSpan.className = this.nbPgSpanCssClass;
        targetEl.appendChild(pgBeforeSpan);
        targetEl.appendChild(slcPages);
        var pgAfterSpan = Dom.create(
            'span',['id', this.prfxPgAfterSpan+tf.id]);
        pgAfterSpan.appendChild( Dom.text(this.ofText) );
        pgAfterSpan.className = this.nbPgSpanCssClass;
        targetEl.appendChild(pgAfterSpan);
        var pgspan = Dom.create( 'span',['id', this.prfxPgSpan+tf.id] );
        pgspan.className = this.nbPgSpanCssClass;
        pgspan.appendChild( Dom.text(' '+this.nbPages+' ') );
        targetEl.appendChild(pgspan);
        targetEl.appendChild(btnNextSpan);
        targetEl.appendChild(btnLastSpan);
        this.pagingSlc = Dom.id(this.prfxSlcPages+tf.id);

        if(!tf.rememberGridValues){
            this.setPagingInfo();
        }
        if(!tf.fltGrid){
            tf.validateAllRows();
            this.setPagingInfo(tf.validRowsIndex);
        }

        this.initialized = true;
    }

    /**
     * Reset paging when filters are already instantiated
     * @param {Boolean} filterTable Execute filtering once paging instanciated
     */
    reset(filterTable=false){
        var tf = this.tf;
        if(!tf.hasGrid() || this.isEnabled()){
            return;
        }
        this.enable();
        this.init();
        tf.resetValues();
        if(filterTable){
            tf.filter();
        }
    }

    /**
     * Calculate number of pages based on valid rows
     * Refresh paging select according to number of pages
     * @param {Array} validRows Collection of valid rows
     */
    setPagingInfo(validRows=[]){
        var tf = this.tf;
        var rows = tf.tbl.rows;
        var mdiv = !this.pagingTgtId ? tf.mDiv : Dom.id(this.pagingTgtId);
        var pgspan = Dom.id(this.prfxPgSpan+tf.id);

        //store valid rows indexes
        tf.validRowsIndex = validRows;

        if(validRows.length === 0){
            //counts rows to be grouped
            for(var j=tf.refRow; j<tf.nbRows; j++){
                var row = rows[j];
                if(!row){
                    continue;
                }

                var isRowValid = row.getAttribute('validRow');
                if(Types.isNull(isRowValid) || Boolean(isRowValid==='true')){
                    tf.validRowsIndex.push(j);
                }
            }
        }

        //calculate nb of pages
        this.nbPages = Math.ceil(tf.validRowsIndex.length/this.pagingLength);
        //refresh page nb span
        pgspan.innerHTML = this.nbPages;
        //select clearing shortcut
        if(this.pageSelectorType === tf.fltTypeSlc){
            this.pagingSlc.innerHTML = '';
        }

        if(this.nbPages>0){
            mdiv.style.visibility = 'visible';
            if(this.pageSelectorType === tf.fltTypeSlc){
                for(var z=0; z<this.nbPages; z++){
                    var opt = Dom.createOpt(z+1, z*this.pagingLength, false);
                    this.pagingSlc.options[z] = opt;
                }
            } else{
                //input type
                this.pagingSlc.value = this.currentPageNb;
            }

        } else {
            /*** if no results paging select and buttons are hidden ***/
            mdiv.style.visibility = 'hidden';
        }
        this.groupByPage(tf.validRowsIndex);
    }

    /**
     * Group table rows by page and display valid rows
     * @param  {Array} validRows Collection of valid rows
     */
    groupByPage(validRows){
        var tf = this.tf;
        var alternateRows =  tf.feature('alternateRows');
        var rows = tf.tbl.rows;
        var endPagingRow = parseInt(this.startPagingRow, 10) +
            parseInt(this.pagingLength, 10);

        //store valid rows indexes
        if(validRows){
            tf.validRowsIndex = validRows;
        }

        //this loop shows valid rows of current page
        for(var h=0, len=tf.validRowsIndex.length; h<len; h++){
            var validRowIdx = tf.validRowsIndex[h];
            var r = rows[validRowIdx];
            var isRowValid = r.getAttribute('validRow');

            if(h>=this.startPagingRow && h<endPagingRow){
                if(Types.isNull(isRowValid) || Boolean(isRowValid==='true')){
                    r.style.display = '';
                }
                if(tf.alternateRows && alternateRows){
                    alternateRows.setRowBg(validRowIdx, h);
                }
            } else {
                r.style.display = 'none';
                if(tf.alternateRows && alternateRows){
                    alternateRows.removeRowBg(validRowIdx);
                }
            }
        }

        tf.nbVisibleRows = tf.validRowsIndex.length;
        //re-applies filter behaviours after filtering process
        tf.applyProps();
    }

    /**
     * Return the current page number
     * @return {Number} Page number
     */
    getPage(){
        return this.currentPageNb;
    }

    /**
     * Show page based on passed param value (string or number):
     * @param {String} or {Number} cmd possible string values: 'next',
     * 'previous', 'last', 'first' or page number as per param
     */
    setPage(cmd){
        var tf = this.tf;
        if(!tf.hasGrid() || !this.isEnabled()){
            return;
        }
        var btnEvt = this.evt,
            cmdtype = typeof cmd;
        if(cmdtype==='string'){
            switch(Str.lower(cmd)){
                case 'next':
                    btnEvt.next();
                break;
                case 'previous':
                    btnEvt.prev();
                break;
                case 'last':
                    btnEvt.last();
                break;
                case 'first':
                    btnEvt.first();
                break;
                default:
                    btnEvt.next();
                break;
            }
        }
        else if(cmdtype==='number'){
            this.changePage(cmd-1);
        }
    }

    /**
     * Generates UI elements for the number of results per page drop-down
     */
    setResultsPerPage(){
        var tf = this.tf;
        var evt = this.evt;

        if(!tf.hasGrid() && !tf.isFirstLoad){
            return;
        }
        if(this.resultsPerPageSlc || !this.resultsPerPage){
            return;
        }

        evt.slcResultsChange = (ev) => {
            this.changeResultsPerPage();
            ev.target.blur();
        };

        var slcR = Dom.create(
            tf.fltTypeSlc, ['id', this.prfxSlcResults+tf.id]);
        slcR.className = this.resultsSlcCssClass;
        var slcRText = this.resultsPerPage[0],
            slcROpts = this.resultsPerPage[1];
        var slcRSpan = Dom.create(
            'span',['id', this.prfxSlcResultsTxt+tf.id]);
        slcRSpan.className = this.resultsSpanCssClass;

        // results per page select is added to external element
        if(!this.resultsPerPageTgtId){
            tf.setToolbar();
        }
        var targetEl = !this.resultsPerPageTgtId ?
            tf.rDiv : Dom.id(this.resultsPerPageTgtId);
        slcRSpan.appendChild(Dom.text(slcRText));

        var help = tf.feature('help');
        if(help && help.btn){
            help.btn.parentNode.insertBefore(slcRSpan, help.btn);
            help.btn.parentNode.insertBefore(slcR, help.btn);
        } else {
            targetEl.appendChild(slcRSpan);
            targetEl.appendChild(slcR);
        }

        for(var r=0; r<slcROpts.length; r++){
            var currOpt = new Option(slcROpts[r], slcROpts[r], false, false);
            slcR.options[r] = currOpt;
        }
        Event.add(slcR, 'change', evt.slcResultsChange);
        this.resultsPerPageSlc = slcR;
    }

    /**
     * Remove number of results per page UI elements
     */
    removeResultsPerPage(){
        var tf = this.tf;
        if(!tf.hasGrid() || !this.resultsPerPageSlc || !this.resultsPerPage){
            return;
        }
        var slcR = this.resultsPerPageSlc,
            slcRSpan = Dom.id(this.prfxSlcResultsTxt+tf.id);
        if(slcR){
            slcR.parentNode.removeChild(slcR);
        }
        if(slcRSpan){
            slcRSpan.parentNode.removeChild(slcRSpan);
        }
        this.resultsPerPageSlc = null;
    }

    /**
     * Change the page asynchronously according to passed index
     * @param  {Number} index Index of the page (0-n)
     */
    changePage(index){
        var tf = this.tf;
        var evt = tf.Evt;
        tf.EvtManager(evt.name.changepage, { pgIndex:index });
    }

    /**
     * Change rows asynchronously according to page results
     */
    changeResultsPerPage(){
        var tf = this.tf;
        var evt = tf.Evt;
        tf.EvtManager(evt.name.changeresultsperpage);
    }

    /**
     * Re-set asynchronously page nb at page re-load
     */
    resetPage(){
        var tf = this.tf;
        var evt = tf.Evt;
        tf.EvtManager(evt.name.resetpage);
    }

    /**
     * Re-set asynchronously page length at page re-load
     */
    resetPageLength(){
        var tf = this.tf;
        var evt = tf.Evt;
        tf.EvtManager(evt.name.resetpagelength);
    }

    /**
     * Change the page according to passed index
     * @param  {Number} index Index of the page (0-n)
     */
    _changePage(index){
        var tf = this.tf;

        if(!this.isEnabled()){
            return;
        }
        if(index === null){
            index = this.pageSelectorType===tf.fltTypeSlc ?
                this.pagingSlc.options.selectedIndex : (this.pagingSlc.value-1);
        }
        if( index>=0 && index<=(this.nbPages-1) ){
            if(this.onBeforeChangePage){
                this.onBeforeChangePage.call(null, this, index);
            }
            this.currentPageNb = parseInt(index, 10)+1;
            if(this.pageSelectorType===tf.fltTypeSlc){
                this.pagingSlc.options[index].selected = true;
            } else {
                this.pagingSlc.value = this.currentPageNb;
            }

            if(tf.rememberPageNb){
                tf.feature('store').savePageNb(tf.pgNbCookie);
            }
            this.startPagingRow = (this.pageSelectorType===tf.fltTypeSlc) ?
                this.pagingSlc.value : (index*this.pagingLength);

            this.groupByPage();

            if(this.onAfterChangePage){
                this.onAfterChangePage.call(null, this, index);
            }
        }
    }

    /**
     * Change rows according to page results drop-down
     * TODO: accept a parameter setting the results per page length
     */
    _changeResultsPerPage(){
        var tf = this.tf;

        if(!this.isEnabled()){
            return;
        }
        var slcR = this.resultsPerPageSlc;
        var slcPagesSelIndex = (this.pageSelectorType===tf.fltTypeSlc) ?
                this.pagingSlc.selectedIndex :
                parseInt(this.pagingSlc.value-1, 10);
        this.pagingLength = parseInt(slcR.options[slcR.selectedIndex].value,10);
        this.startPagingRow = this.pagingLength*slcPagesSelIndex;

        if(!isNaN(this.pagingLength)){
            if(this.startPagingRow >= tf.nbFilterableRows){
                this.startPagingRow = (tf.nbFilterableRows-this.pagingLength);
            }
            this.setPagingInfo();

            if(this.pageSelectorType===tf.fltTypeSlc){
                var slcIndex =
                    (this.pagingSlc.options.length-1<=slcPagesSelIndex ) ?
                    (this.pagingSlc.options.length-1) : slcPagesSelIndex;
                this.pagingSlc.options[slcIndex].selected = true;
            }
            if(tf.rememberPageLen){
                tf.feature('store').savePageLength(tf.pgLenCookie);
            }
        }
    }

    /**
     * Re-set page nb at page re-load
     */
    _resetPage(name){
        var tf = this.tf;
        var pgnb = tf.feature('store').getPageNb(name);
        if(pgnb!==''){
            this.changePage((pgnb-1));
        }
    }

    /**
     * Re-set page length value at page re-load
     */
    _resetPageLength(name){
        var tf = this.tf;
        if(!this.isEnabled()){
            return;
        }
        var pglenIndex = tf.feature('store').getPageLength(name);

        if(pglenIndex!==''){
            this.resultsPerPageSlc.options[pglenIndex].selected = true;
            this.changeResultsPerPage();
        }
    }

    /**
     * Remove paging feature
     */
    destroy(){
        var tf = this.tf;

        if(!this.initialized){
            return;
        }
        // btns containers
        var btnNextSpan = Dom.id(this.prfxBtnNextSpan+tf.id);
        var btnPrevSpan = Dom.id(this.prfxBtnPrevSpan+tf.id);
        var btnLastSpan = Dom.id(this.prfxBtnLastSpan+tf.id);
        var btnFirstSpan = Dom.id(this.prfxBtnFirstSpan+tf.id);
        //span containing 'Page' text
        var pgBeforeSpan = Dom.id(this.prfxPgBeforeSpan+tf.id);
        //span containing 'of' text
        var pgAfterSpan = Dom.id(this.prfxPgAfterSpan+tf.id);
        //span containing nb of pages
        var pgspan = Dom.id(this.prfxPgSpan+tf.id);

        var evt = this.evt;

        if(this.pagingSlc){
            if(this.pageSelectorType === tf.fltTypeSlc){
                Event.remove(this.pagingSlc, 'change', evt.slcPagesChange);
            }
            else if(this.pageSelectorType === tf.fltTypeInp){
                Event.remove(this.pagingSlc, 'keypress', evt._detectKey);
            }
            this.pagingSlc.parentNode.removeChild(this.pagingSlc);
        }

        if(btnNextSpan){
            Event.remove(btnNextSpan, 'click', evt.next);
            btnNextSpan.parentNode.removeChild(btnNextSpan);
        }

        if(btnPrevSpan){
            Event.remove(btnPrevSpan, 'click', evt.prev);
            btnPrevSpan.parentNode.removeChild(btnPrevSpan);
        }

        if(btnLastSpan){
            Event.remove(btnLastSpan, 'click', evt.last);
            btnLastSpan.parentNode.removeChild(btnLastSpan);
        }

        if(btnFirstSpan){
            Event.remove(btnFirstSpan, 'click', evt.first);
            btnFirstSpan.parentNode.removeChild(btnFirstSpan);
        }

        if(pgBeforeSpan){
            pgBeforeSpan.parentNode.removeChild(pgBeforeSpan);
        }

        if(pgAfterSpan){
            pgAfterSpan.parentNode.removeChild(pgAfterSpan);
        }

        if(pgspan){
            pgspan.parentNode.removeChild(pgspan);
        }

        if(this.hasResultsPerPage){
            this.removeResultsPerPage();
        }

        this.pagingSlc = null;
        this.nbPages = 0;
        this.disable();
        this.initialized = false;
    }
}
