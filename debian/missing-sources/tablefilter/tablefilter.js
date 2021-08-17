import Event from './event';
import Dom from './dom';
import Str from './string';
import Cookie from './cookie';
import Types from './types';
import Arr from './array';
import DateHelper from './date';
import Helpers from './helpers';

// Features
import {Store} from './modules/store';
import {GridLayout} from './modules/gridLayout';
import {Loader} from './modules/loader';
import {HighlightKeyword} from './modules/highlightKeywords';
import {PopupFilter} from './modules/popupFilter';
import {Dropdown} from './modules/dropdown';
import {CheckList} from './modules/checkList';
import {RowsCounter} from './modules/rowsCounter';
import {StatusBar} from './modules/statusBar';
import {Paging} from './modules/paging';
import {ClearButton} from './modules/clearButton';
import {Help} from './modules/help';
import {AlternateRows} from './modules/alternateRows';

var global = window,
    isValidDate = DateHelper.isValid,
    formatDate = DateHelper.format,
    doc = global.document;

export class TableFilter{

    /**
     * TableFilter object constructor
     * requires `table` or `id` arguments, `row` and `configuration` optional
     * @param {DOMElement} table Table DOM element
     * @param {String} id Table id
     * @param {Number} row index indicating the 1st row
     * @param {Object} configuration object
     */
    constructor(...args) {
        if(args.length === 0){ return; }

        this.id = null;
        this.version = '{VERSION}';
        this.year = new Date().getFullYear();
        this.tbl = null;
        this.startRow = null;
        this.refRow = null;
        this.headersRow = null;
        this.cfg = {};
        this.nbFilterableRows = null;
        this.nbRows = null;
        this.nbCells = null;
        this._hasGrid = false;

        // TODO: use for-of with babel plug-in
        args.forEach((arg)=> {
            let argtype = typeof arg;
            if(argtype === 'object' && arg && arg.nodeName === 'TABLE'){
                this.tbl = arg;
                this.id = arg.id || `tf_${new Date().getTime()}_`;
            } else if(argtype === 'string'){
                this.id = arg;
                this.tbl = Dom.id(arg);
            } else if(argtype === 'number'){
                this.startRow = arg;
            } else if(argtype === 'object'){
                this.cfg = arg;
            }
        });

        if(!this.tbl || this.tbl.nodeName != 'TABLE' || this.getRowsNb() === 0){
            throw new Error(
                'Could not instantiate TableFilter: HTML table not found.');
        }

        // configuration object
        let f = this.cfg;

        //Start row et cols nb
        this.refRow = this.startRow === null ? 2 : (this.startRow+1);
        try{ this.nbCells = this.getCellsNb(this.refRow); }
        catch(e){ this.nbCells = this.getCellsNb(0); }

        //default script base path
        this.basePath = f.base_path || 'tablefilter/';

        /*** filter types ***/
        this.fltTypeInp = 'input';
        this.fltTypeSlc = 'select';
        this.fltTypeMulti = 'multiple';
        this.fltTypeCheckList = 'checklist';
        this.fltTypeNone = 'none';

        /*** filters' grid properties ***/

        //enables/disables filter grid
        this.fltGrid = f.grid === false ? false : true;

        //enables/disables grid layout (fixed headers)
        this.gridLayout = Boolean(f.grid_layout);

        this.filtersRowIndex = isNaN(f.filters_row_index) ?
            0 : f.filters_row_index;
        this.headersRow = isNaN(f.headers_row_index) ?
            (this.filtersRowIndex === 0 ? 1 : 0) : f.headers_row_index;

        if(this.gridLayout){
            if(this.headersRow > 1){
                this.filtersRowIndex = this.headersRow+1;
            } else {
                this.filtersRowIndex = 1;
                this.headersRow = 0;
            }
        }

        //defines tag of the cells containing filters (td/th)
        this.fltCellTag = f.filters_cell_tag!=='th' ||
            f.filters_cell_tag!=='td' ? 'td' : f.filters_cell_tag;

        //stores filters ids
        this.fltIds = [];
        //stores filters DOM elements
        this.fltElms = [];
        //stores filters values
        this.searchArgs = null;
        //stores valid rows indexes (rows visible upon filtering)
        this.validRowsIndex = null;
        //stores filters row element
        this.fltGridEl = null;
        //is first load boolean
        this.isFirstLoad = true;
        //container div for paging elements, reset btn etc.
        this.infDiv = null;
        //div for rows counter
        this.lDiv = null;
        //div for reset button and results per page select
        this.rDiv = null;
        //div for paging elements
        this.mDiv = null;

        //defines css class for div containing paging elements, rows counter etc
        this.infDivCssClass = f.inf_div_css_class || 'inf';
        //defines css class for left div
        this.lDivCssClass = f.left_div_css_class || 'ldiv';
        //defines css class for right div
        this.rDivCssClass =  f.right_div_css_class || 'rdiv';
        //defines css class for mid div
        this.mDivCssClass = f.middle_div_css_class || 'mdiv';
        //table container div css class
        this.contDivCssClass = f.content_div_css_class || 'cont';

        /*** filters' grid appearance ***/
        //stylesheet file
        this.stylePath = f.style_path || this.basePath + 'style/';
        this.stylesheet = f.stylesheet || this.stylePath+'tablefilter.css';
        this.stylesheetId = this.id + '_style';
        //defines css class for filters row
        this.fltsRowCssClass = f.flts_row_css_class || 'fltrow';
         //enables/disables icons (paging, reset button)
        this.enableIcons = f.enable_icons===false ? false : true;
        //enables/disbles rows alternating bg colors
        this.alternateRows = Boolean(f.alternate_rows);
        //defines widths of columns
        this.hasColWidths = Types.isArray(f.col_widths);
        this.colWidths = this.hasColWidths ? f.col_widths : null;
        //defines css class for filters
        this.fltCssClass = f.flt_css_class || 'flt';
        //defines css class for multiple selects filters
        this.fltMultiCssClass = f.flt_multi_css_class || 'flt_multi';
        //defines css class for filters
        this.fltSmallCssClass = f.flt_small_css_class || 'flt_s';
        //defines css class for single-filter
        this.singleFltCssClass = f.single_flt_css_class || 'single_flt';

        /*** filters' grid behaviours ***/
        //enables/disables enter key
        this.enterKey = f.enter_key===false ? false : true;
        //calls function before filtering starts
        this.onBeforeFilter = Types.isFn(f.on_before_filter) ?
            f.on_before_filter : null;
        //calls function after filtering
        this.onAfterFilter = Types.isFn(f.on_after_filter) ?
            f.on_after_filter : null;
        //enables/disables case sensitivity
        this.caseSensitive = Boolean(f.case_sensitive);
        //has exact match per column
        this.hasExactMatchByCol = Types.isArray(f.columns_exact_match);
        this.exactMatchByCol = this.hasExactMatchByCol ?
            f.columns_exact_match : [];
        //enables/disbles exact match for search
        this.exactMatch = Boolean(f.exact_match);
        //refreshes drop-down lists upon validation
        this.linkedFilters = Boolean(f.linked_filters);
        //wheter excluded options are disabled
        this.disableExcludedOptions = Boolean(f.disable_excluded_options);
        //stores active filter element
        this.activeFlt = null;
        //id of active filter
        this.activeFilterId = null;
        //enables always visible rows
        this.hasVisibleRows = Boolean(f.rows_always_visible);
        //array containing always visible rows
        this.visibleRows = this.hasVisibleRows ? f.rows_always_visible : [];
        //enables/disables external filters generation
        this.isExternalFlt = Boolean(f.external_flt_grid);
        //array containing ids of external elements containing filters
        this.externalFltTgtIds = f.external_flt_grid_ids || null;
        //stores filters elements if isExternalFlt is true
        this.externalFltEls = [];
        //delays any filtering process if loader true
        this.execDelay = !isNaN(f.exec_delay) ? parseInt(f.exec_delay,10) : 100;
        //calls function when filters grid loaded
        this.onFiltersLoaded = Types.isFn(f.on_filters_loaded) ?
            f.on_filters_loaded : null;
        //enables/disables single filter search
        this.singleSearchFlt = Boolean(f.single_filter);
        //calls function after row is validated
        this.onRowValidated = Types.isFn(f.on_row_validated) ?
            f.on_row_validated : null;
        //array defining columns for customCellData event
        this.customCellDataCols = f.custom_cell_data_cols ?
            f.custom_cell_data_cols : [];
        //calls custom function for retrieving cell data
        this.customCellData = Types.isFn(f.custom_cell_data) ?
            f.custom_cell_data : null;
        //input watermark text array
        this.watermark = f.watermark || '';
        this.isWatermarkArray = Types.isArray(this.watermark);
        //id of toolbar container element
        this.toolBarTgtId = f.toolbar_target_id || null;
        //enables/disables help div
        this.help = Types.isUndef(f.help_instructions) ?
            undefined : Boolean(f.help_instructions);
        //popup filters
        this.popupFilters = Boolean(f.popup_filters);
        //active columns color
        this.markActiveColumns = Boolean(f.mark_active_columns);
        //defines css class for active column header
        this.activeColumnsCssClass = f.active_columns_css_class ||
            'activeHeader';
        //calls function before active column header is marked
        this.onBeforeActiveColumn = Types.isFn(f.on_before_active_column) ?
            f.on_before_active_column : null;
        //calls function after active column header is marked
        this.onAfterActiveColumn = Types.isFn(f.on_after_active_column) ?
            f.on_after_active_column : null;

        /*** select filter's customisation and behaviours ***/
        //defines 1st option text
        this.displayAllText = f.display_all_text || 'Clear';
        //enables/disables empty option in combo-box filters
        this.enableEmptyOption = Boolean(f.enable_empty_option);
        //defines empty option text
        this.emptyText = f.empty_text || '(Empty)';
        //enables/disables non empty option in combo-box filters
        this.enableNonEmptyOption = Boolean(f.enable_non_empty_option);
        //defines empty option text
        this.nonEmptyText = f.non_empty_text || '(Non empty)';
        //enables/disables onChange event on combo-box
        this.onSlcChange = f.on_change===false ? false : true;
        //enables/disables select options sorting
        this.sortSlc = f.sort_select===false ? false : true;
        //enables/disables ascending numeric options sorting
        this.isSortNumAsc = Boolean(f.sort_num_asc);
        this.sortNumAsc = this.isSortNumAsc ? f.sort_num_asc : null;
        //enables/disables descending numeric options sorting
        this.isSortNumDesc = Boolean(f.sort_num_desc);
        this.sortNumDesc = this.isSortNumDesc ? f.sort_num_desc : null;
        //Select filters are populated on demand
        this.loadFltOnDemand = Boolean(f.load_filters_on_demand);
        this.hasCustomOptions = Types.isObj(f.custom_options);
        this.customOptions = f.custom_options;

        /*** Filter operators ***/
        this.rgxOperator = f.regexp_operator || 'rgx:';
        this.emOperator = f.empty_operator || '[empty]';
        this.nmOperator = f.nonempty_operator || '[nonempty]';
        this.orOperator = f.or_operator || '||';
        this.anOperator = f.and_operator || '&&';
        this.grOperator = f.greater_operator || '>';
        this.lwOperator = f.lower_operator || '<';
        this.leOperator = f.lower_equal_operator || '<=';
        this.geOperator = f.greater_equal_operator || '>=';
        this.dfOperator = f.different_operator || '!';
        this.lkOperator = f.like_operator || '*';
        this.eqOperator = f.equal_operator || '=';
        this.stOperator = f.start_with_operator || '{';
        this.enOperator = f.end_with_operator || '}';
        this.curExp = f.cur_exp || '^[¥£€$]';
        this.separator = f.separator || ',';

        /*** rows counter ***/
        //show/hides rows counter
        this.rowsCounter = Boolean(f.rows_counter);

        /*** status bar ***/
        //show/hides status bar
        this.statusBar = Boolean(f.status_bar);

        /*** loader ***/
        //enables/disables loader/spinner indicator
        this.loader = Boolean(f.loader);

        /*** validation - reset buttons/links ***/
        //show/hides filter's validation button
        this.displayBtn = Boolean(f.btn);
        //defines validation button text
        this.btnText = f.btn_text || (!this.enableIcons ? 'Go' : '');
        //defines css class for validation button
        this.btnCssClass = f.btn_css_class ||
            (!this.enableIcons ? 'btnflt' : 'btnflt_icon');
        //show/hides reset link
        this.btnReset = Boolean(f.btn_reset);
        //defines css class for reset button
        this.btnResetCssClass = f.btn_reset_css_class || 'reset';
        //callback function before filters are cleared
        this.onBeforeReset = Types.isFn(f.on_before_reset) ?
            f.on_before_reset : null;
        //callback function after filters are cleared
        this.onAfterReset = Types.isFn(f.on_after_reset) ?
            f.on_after_reset : null;

        /*** paging ***/
        //enables/disables table paging
        this.paging = Boolean(f.paging);
        this.nbVisibleRows = 0; //nb visible rows
        this.nbHiddenRows = 0; //nb hidden rows

        /*** autofilter on typing ***/
        //enables/disables auto filtering, table is filtered when user stops
        //typing
        this.autoFilter = Boolean(f.auto_filter);
        //onkeyup delay timer (msecs)
        this.autoFilterDelay = !isNaN(f.auto_filter_delay) ?
            f.auto_filter_delay : 900;
        //typing indicator
        this.isUserTyping = null;
        this.autoFilterTimer = null;

        /*** keyword highlighting ***/
        //enables/disables keyword highlighting
        this.highlightKeywords = Boolean(f.highlight_keywords);

        /*** data types ***/
        //defines default date type (european DMY)
        this.defaultDateType = f.default_date_type || 'DMY';
        //defines default thousands separator
        //US = ',' EU = '.'
        this.thousandsSeparator = f.thousands_separator || ',';
        //defines default decimal separator
        //US & javascript = '.' EU = ','
        this.decimalSeparator = f.decimal_separator || '.';
        //enables number format per column
        this.hasColNbFormat = Types.isArray(f.col_number_format);
        //array containing columns nb formats
        this.colNbFormat = this.hasColNbFormat ? f.col_number_format : null;
        //enables date type per column
        this.hasColDateType = Types.isArray(f.col_date_type);
        //array containing columns date type
        this.colDateType = this.hasColDateType ? f.col_date_type : null;

        /*** status messages ***/
        //filtering
        this.msgFilter = f.msg_filter || 'Filtering data...';
        //populating drop-downs
        this.msgPopulate = f.msg_populate || 'Populating filter...';
        //populating drop-downs
        this.msgPopulateCheckList = f.msg_populate_checklist ||
            'Populating list...';
        //changing paging page
        this.msgChangePage = f.msg_change_page || 'Collecting paging data...';
        //clearing filters
        this.msgClear = f.msg_clear || 'Clearing filters...';
        //changing nb results/page
        this.msgChangeResults = f.msg_change_results ||
            'Changing results per page...';
        //re-setting grid values
        this.msgResetValues = f.msg_reset_grid_values ||
            'Re-setting filters values...';
        //re-setting page
        this.msgResetPage = f.msg_reset_page || 'Re-setting page...';
        //re-setting page length
        this.msgResetPageLength = f.msg_reset_page_length ||
            'Re-setting page length...';
        //table sorting
        this.msgSort = f.msg_sort || 'Sorting data...';
        //extensions loading
        this.msgLoadExtensions = f.msg_load_extensions ||
            'Loading extensions...';
        //themes loading
        this.msgLoadThemes = f.msg_load_themes || 'Loading theme(s)...';

        /*** ids prefixes ***/
        //css class name added to table
        this.prfxTf = 'TF';
        //filters (inputs - selects)
        this.prfxFlt = 'flt';
        //validation button
        this.prfxValButton = 'btn';
        //container div for paging elements, rows counter etc.
        this.prfxInfDiv = 'inf_';
        //left div
        this.prfxLDiv = 'ldiv_';
        //right div
        this.prfxRDiv = 'rdiv_';
        //middle div
        this.prfxMDiv = 'mdiv_';
        //filter values cookie
        this.prfxCookieFltsValues = 'tf_flts_';
        //page nb cookie
        this.prfxCookiePageNb = 'tf_pgnb_';
        //page length cookie
        this.prfxCookiePageLen = 'tf_pglen_';

        /*** cookies ***/
        this.hasStoredValues = false;
        //remembers filters values on page load
        this.rememberGridValues = Boolean(f.remember_grid_values);
        //cookie storing filter values
        this.fltsValuesCookie = this.prfxCookieFltsValues + this.id;
        //remembers page nb on page load
        this.rememberPageNb = this.paging && f.remember_page_number;
        //cookie storing page nb
        this.pgNbCookie = this.prfxCookiePageNb + this.id;
        //remembers page length on page load
        this.rememberPageLen = this.paging && f.remember_page_length;
        //cookie storing page length
        this.pgLenCookie = this.prfxCookiePageLen + this.id;

        /*** extensions ***/
        //imports external script
        this.extensions = f.extensions;
        this.hasExtensions = Types.isArray(this.extensions);

        /*** themes ***/
        this.enableDefaultTheme = Boolean(f.enable_default_theme);
        //imports themes
        this.hasThemes = (this.enableDefaultTheme || Types.isArray(f.themes));
        this.themes = f.themes || [];
        //themes path
        this.themesPath = f.themes_path || this.stylePath + 'themes/';

        // Features registry
        this.Mod = {};

        // Extensions registry
        this.ExtRegistry = {};

        /*** TF events ***/
        this.Evt = {
            name: {
                filter: 'Filter',
                dropdown: 'DropDown',
                checklist: 'CheckList',
                changepage: 'ChangePage',
                clear: 'Clear',
                changeresultsperpage: 'ChangeResults',
                resetvalues: 'ResetValues',
                resetpage: 'ResetPage',
                resetpagelength: 'ResetPageLength',
                loadextensions: 'LoadExtensions',
                loadthemes: 'LoadThemes'
            },

            // Detect <enter> key
            detectKey(e) {
                if(!this.enterKey){ return; }
                let _ev = e || global.event;
                if(_ev){
                    let key = Event.keyCode(_ev);
                    if(key===13){
                        this.filter();
                        Event.cancel(_ev);
                        Event.stop(_ev);
                    } else {
                        this.isUserTyping = true;
                        global.clearInterval(this.autoFilterTimer);
                        this.autoFilterTimer = null;
                    }
                }
            },
            // if auto-filter on, detect user is typing and filter columns
            onKeyUp(e) {
                if(!this.autoFilter){
                    return;
                }
                let _ev = e || global.event;
                let key = Event.keyCode(_ev);
                this.isUserTyping = false;

                function filter() {
                    /*jshint validthis:true */
                    global.clearInterval(this.autoFilterTimer);
                    this.autoFilterTimer = null;
                    if(!this.isUserTyping){
                        this.filter();
                        this.isUserTyping = null;
                    }
                }

                if(key!==13 && key!==9 && key!==27 && key!==38 && key!==40) {
                    if(this.autoFilterTimer === null){
                        this.autoFilterTimer = global.setInterval(
                            filter.bind(this), this.autoFilterDelay);
                    }
                } else {
                    global.clearInterval(this.autoFilterTimer);
                    this.autoFilterTimer = null;
                }
            },
            // if auto-filter on, detect user is typing
            onKeyDown() {
                if(!this.autoFilter) { return; }
                this.isUserTyping = true;
            },
            // if auto-filter on, clear interval on filter blur
            onInpBlur() {
                if(this.autoFilter){
                    this.isUserTyping = false;
                    global.clearInterval(this.autoFilterTimer);
                }
                // TODO: hack to prevent ezEditTable enter key event hijaking.
                // Needs to be fixed in the vendor's library
                if(this.hasExtension('advancedGrid')){
                    var advGrid = this.extension('advancedGrid');
                    var ezEditTable = advGrid._ezEditTable;
                    if(advGrid.cfg.editable){
                        ezEditTable.Editable.Set();
                    }
                    if(advGrid.cfg.selection){
                        ezEditTable.Selection.Set();
                    }
                }
            },
            // set focused text-box filter as active
            onInpFocus(e) {
                let _ev = e || global.event;
                let elm = Event.target(_ev);
                this.activeFilterId = elm.getAttribute('id');
                this.activeFlt = Dom.id(this.activeFilterId);
                if(this.popupFilters){
                    Event.cancel(_ev);
                    Event.stop(_ev);
                }
                // TODO: hack to prevent ezEditTable enter key event hijaking.
                // Needs to be fixed in the vendor's library
                if(this.hasExtension('advancedGrid')){
                    var advGrid = this.extension('advancedGrid');
                    var ezEditTable = advGrid._ezEditTable;
                    if(advGrid.cfg.editable){
                        ezEditTable.Editable.Remove();
                    }
                    if(advGrid.cfg.selection){
                        ezEditTable.Selection.Remove();
                    }
                }
            },
            // set focused drop-down filter as active
            onSlcFocus(e) {
                let _ev = e || global.event;
                let elm = Event.target(_ev);
                this.activeFilterId = elm.getAttribute('id');
                this.activeFlt = Dom.id(this.activeFilterId);
                // select is populated when element has focus
                if(this.loadFltOnDemand && elm.getAttribute('filled') === '0'){
                    let ct = elm.getAttribute('ct');
                    this.Mod.dropdown._build(ct);
                }
                if(this.popupFilters){
                    Event.cancel(_ev);
                    Event.stop(_ev);
                }
            },
            // filter columns on drop-down filter change
            onSlcChange(e) {
                if(!this.activeFlt){ return; }
                let _ev = e || global.event;
                if(this.popupFilters){ Event.stop(_ev); }
                if(this.onSlcChange){ this.filter(); }
            },
            // fill checklist filter on click if required
            onCheckListClick(e) {
                let _ev = e || global.event;
                let elm = Event.target(_ev);
                if(this.loadFltOnDemand && elm.getAttribute('filled') === '0'){
                    let ct = elm.getAttribute('ct');
                    this.Mod.checkList._build(ct);
                    this.Mod.checkList.checkListDiv[ct].onclick = null;
                    this.Mod.checkList.checkListDiv[ct].title = '';
                }
            },
            // filter when validation button clicked
            onBtnClick() {
                this.filter();
            }
        };
    }

    /**
     * Initialise filtering grid bar behaviours and layout
     *
     * TODO: decompose in smaller methods
     */
    init(){
        if(this._hasGrid){
            return;
        }
        if(!this.tbl){
            this.tbl = Dom.id(this.id);
        }
        if(this.gridLayout){
            this.refRow = this.startRow===null ? 0 : this.startRow;
        }
        if(this.popupFilters &&
            ((this.filtersRowIndex === 0 && this.headersRow === 1) ||
            this.gridLayout)){
            this.headersRow = 0;
        }

        let Mod = this.Mod;
        let n = this.singleSearchFlt ? 1 : this.nbCells,
            inpclass;

        //loads stylesheet if not imported
        this.import(this.stylesheetId, this.stylesheet, null, 'link');

        //loads theme
        if(this.hasThemes){ this._loadThemes(); }

        if(this.rememberGridValues || this.rememberPageNb ||
            this.rememberPageLen){
            Mod.store = new Store(this);
        }

        if(this.gridLayout){
            Mod.gridLayout = new GridLayout(this);
            Mod.gridLayout.init();
        }

        if(this.loader){
            if(!Mod.loader){
                Mod.loader = new Loader(this);
                Mod.loader.init();
            }
        }

        if(this.highlightKeywords){
            Mod.highlightKeyword = new HighlightKeyword(this);
        }

        if(this.popupFilters){
            if(!Mod.popupFilter){
                Mod.popupFilter = new PopupFilter(this);
            }
            Mod.popupFilter.init();
        }

        //filters grid is not generated
        if(!this.fltGrid){
            this.refRow = this.refRow-1;
            if(this.gridLayout){
                this.refRow = 0;
            }
            this.nbFilterableRows = this.getRowsNb();
            this.nbVisibleRows = this.nbFilterableRows;
            this.nbRows = this.nbFilterableRows + this.refRow;
        } else {
            if(this.isFirstLoad){
                let fltrow;
                if(!this.gridLayout){
                    let thead = Dom.tag(this.tbl, 'thead');
                    if(thead.length > 0){
                        fltrow = thead[0].insertRow(this.filtersRowIndex);
                    } else {
                        fltrow = this.tbl.insertRow(this.filtersRowIndex);
                    }

                    if(this.headersRow > 1 &&
                        this.filtersRowIndex <= this.headersRow &&
                        !this.popupFilters){
                        this.headersRow++;
                    }
                    if(this.popupFilters){
                        this.headersRow++;
                    }

                    fltrow.className = this.fltsRowCssClass;

                    if(this.isExternalFlt || this.popupFilters){
                        fltrow.style.display = 'none';
                    }
                }

                this.nbFilterableRows = this.getRowsNb();
                this.nbVisibleRows = this.nbFilterableRows;
                this.nbRows = this.tbl.rows.length;

                for(let i=0; i<n; i++){// this loop adds filters

                    if(this.popupFilters){
                        Mod.popupFilter.build(i);
                    }

                    let fltcell = Dom.create(this.fltCellTag),
                        col = this.getFilterType(i),
                        externalFltTgtId =
                            this.isExternalFlt && this.externalFltTgtIds ?
                            this.externalFltTgtIds[i] : null;

                    if(this.singleSearchFlt){
                        fltcell.colSpan = this.nbCells;
                    }
                    if(!this.gridLayout){
                        fltrow.appendChild(fltcell);
                    }
                    inpclass = (i==n-1 && this.displayBtn) ?
                        this.fltSmallCssClass : this.fltCssClass;

                    //only 1 input for single search
                    if(this.singleSearchFlt){
                        col = this.fltTypeInp;
                        inpclass = this.singleFltCssClass;
                    }

                    //drop-down filters
                    if(col===this.fltTypeSlc || col===this.fltTypeMulti){
                        if(!Mod.dropdown){
                            Mod.dropdown = new Dropdown(this);
                        }
                        let dropdown = Mod.dropdown;

                        let slc = Dom.create(this.fltTypeSlc,
                                ['id', this.prfxFlt+i+'_'+this.id],
                                ['ct', i], ['filled', '0']
                            );

                        if(col===this.fltTypeMulti){
                            slc.multiple = this.fltTypeMulti;
                            slc.title = dropdown.multipleSlcTooltip;
                        }
                        slc.className = Str.lower(col)===this.fltTypeSlc ?
                            inpclass : this.fltMultiCssClass;// for ie<=6

                        //filter is appended in desired external element
                        if(externalFltTgtId){
                            Dom.id(externalFltTgtId).appendChild(slc);
                            this.externalFltEls.push(slc);
                        } else {
                            fltcell.appendChild(slc);
                        }

                        this.fltIds.push(this.prfxFlt+i+'_'+this.id);

                        if(!this.loadFltOnDemand){
                            dropdown._build(i);
                        }

                        Event.add(slc, 'keypress',
                            this.Evt.detectKey.bind(this));
                        Event.add(slc, 'change',
                            this.Evt.onSlcChange.bind(this));
                        Event.add(slc, 'focus', this.Evt.onSlcFocus.bind(this));

                        //1st option is created here since dropdown.build isn't
                        //invoked
                        if(this.loadFltOnDemand){
                            let opt0 = Dom.createOpt(this.displayAllText, '');
                            slc.appendChild(opt0);
                        }
                    }
                    // checklist
                    else if(col===this.fltTypeCheckList){
                        let checkList;
                        Mod.checkList = new CheckList(this);
                        checkList = Mod.checkList;

                        let divCont = Dom.create('div',
                            ['id', checkList.prfxCheckListDiv+i+'_'+this.id],
                            ['ct', i], ['filled', '0']);
                        divCont.className = checkList.checkListDivCssClass;

                        //filter is appended in desired element
                        if(externalFltTgtId){
                            Dom.id(externalFltTgtId).appendChild(divCont);
                            this.externalFltEls.push(divCont);
                        } else {
                            fltcell.appendChild(divCont);
                        }

                        checkList.checkListDiv[i] = divCont;
                        this.fltIds.push(this.prfxFlt+i+'_'+this.id);
                        if(!this.loadFltOnDemand){
                            checkList._build(i);
                        }

                        if(this.loadFltOnDemand){
                            Event.add(divCont, 'click',
                                this.Evt.onCheckListClick.bind(this));
                            divCont.appendChild(
                                Dom.text(checkList.activateCheckListTxt));
                        }
                    }

                    else{
                        //show/hide input
                        let inptype = col===this.fltTypeInp ? 'text' : 'hidden';
                        let inp = Dom.create(this.fltTypeInp,
                            ['id',this.prfxFlt+i+'_'+this.id],
                            ['type',inptype], ['ct',i]);
                        if(inptype!=='hidden' && this.watermark){
                            inp.setAttribute(
                                'placeholder',
                                this.isWatermarkArray ?
                                    (this.watermark[i] || '') : this.watermark
                            );
                        }
                        inp.className = inpclass;
                        Event.add(inp, 'focus', this.Evt.onInpFocus.bind(this));

                        //filter is appended in desired element
                        if(externalFltTgtId){
                            Dom.id(externalFltTgtId).appendChild(inp);
                            this.externalFltEls.push(inp);
                        } else {
                            fltcell.appendChild(inp);
                        }

                        this.fltIds.push(this.prfxFlt+i+'_'+this.id);

                        Event.add(inp, 'keypress',
                            this.Evt.detectKey.bind(this));
                        Event.add(inp, 'keydown',
                            this.Evt.onKeyDown.bind(this));
                        Event.add(inp, 'keyup', this.Evt.onKeyUp.bind(this));
                        Event.add(inp, 'blur', this.Evt.onInpBlur.bind(this));

                        if(this.rememberGridValues){
                            let flts_values = this.Mod.store.getFilterValues(
                                this.fltsValuesCookie);
                            if(flts_values[i]!=' '){
                                this.setFilterValue(i, flts_values[i], false);
                            }
                        }
                    }
                    // this adds submit button
                    if(i==n-1 && this.displayBtn){
                        let btn = Dom.create(this.fltTypeInp,
                            ['id',this.prfxValButton+i+'_'+this.id],
                            ['type','button'], ['value',this.btnText]);
                        btn.className = this.btnCssClass;

                        //filter is appended in desired element
                        if(externalFltTgtId){
                            Dom.id(externalFltTgtId).appendChild(btn);
                        } else{
                            fltcell.appendChild(btn);
                        }

                        Event.add(btn, 'click', this.Evt.onBtnClick.bind(this));
                    }//if

                }// for i

            } else {
                this._resetGrid();
            }//if isFirstLoad

        }//if this.fltGrid

        /* Filter behaviours */
        if(this.hasVisibleRows){
            this.enforceVisibility();
        }
        if(this.rowsCounter){
            Mod.rowsCounter = new RowsCounter(this);
            Mod.rowsCounter.init();
        }
        if(this.statusBar){
            Mod.statusBar = new StatusBar(this);
            Mod.statusBar.init();
        }
        if(this.paging || Mod.paging){
            if(!Mod.paging){
                Mod.paging = new Paging(this);
                Mod.paging.init();
            }
            Mod.paging.reset();
        }
        if(this.btnReset){
            Mod.clearButton = new ClearButton(this);
            Mod.clearButton.init();
        }
        if(this.help){
            if(!Mod.help){
                Mod.help = new Help(this);
            }
            Mod.help.init();
        }
        if(this.hasColWidths && !this.gridLayout){
            this.setColWidths();
        }
        if(this.alternateRows){
            Mod.alternateRows = new AlternateRows(this);
            Mod.alternateRows.init();
        }

        this.isFirstLoad = false;
        this._hasGrid = true;

        if(this.rememberGridValues || this.rememberPageLen ||
            this.rememberPageNb){
            this.resetValues();
        }

        //TF css class is added to table
        if(!this.gridLayout){
            Dom.addClass(this.tbl, this.prfxTf);
        }

        if(this.loader){
            Mod.loader.show('none');
        }

        /* Loads extensions */
        if(this.hasExtensions){
            this.initExtensions();
        }

        if(this.onFiltersLoaded){
            this.onFiltersLoaded.call(null, this);
        }
    }

    /**
     * Manages state messages
     * @param {String} evt Event name
     * @param {Object} cfg Config object
     */
    EvtManager(evt,
        cfg={ slcIndex: null, slcExternal: false, slcId: null, pgIndex: null }){
        let slcIndex = cfg.slcIndex;
        let slcExternal = cfg.slcExternal;
        let slcId = cfg.slcId;
        let pgIndex = cfg.pgIndex;
        let cpt = this.Mod;

        function efx(){
            /*jshint validthis:true */
            let ev = this.Evt.name;

            switch(evt){
                case ev.filter:
                    this._filter();
                break;
                case ev.dropdown:
                    if(this.linkedFilters){
                        cpt.dropdown._build(slcIndex, true);
                    } else {
                        cpt.dropdown._build(
                            slcIndex, false, slcExternal, slcId);
                    }
                break;
                case ev.checklist:
                    cpt.checkList._build(slcIndex, slcExternal, slcId);
                break;
                case ev.changepage:
                    cpt.paging._changePage(pgIndex);
                break;
                case ev.clear:
                    this._clearFilters();
                    this._filter();
                break;
                case ev.changeresultsperpage:
                    cpt.paging._changeResultsPerPage();
                break;
                case ev.resetvalues:
                    this._resetValues();
                    this._filter();
                break;
                case ev.resetpage:
                    cpt.paging._resetPage(this.pgNbCookie);
                break;
                case ev.resetpagelength:
                    cpt.paging._resetPageLength(this.pgLenCookie);
                break;
                case ev.loadextensions:
                    this._loadExtensions();
                break;
                case ev.loadthemes:
                    this._loadThemes();
                break;
            }
            if(this.statusBar){
                cpt.statusBar.message('');
            }
            if(this.loader){
                cpt.loader.show('none');
            }
        }

        if(!this.loader && !this.statusBar && !this.linkedFilters) {
            efx.call(this);
        } else {
            if(this.loader){
                cpt.loader.show('');
            }
            if(this.statusBar){
                cpt.statusBar.message(this['msg'+evt]);
            }
            global.setTimeout(efx.bind(this), this.execDelay);
        }
    }

    /**
     * Return a feature instance for a given name
     * @param  {String} name Name of the feature
     * @return {Object}
     */
    feature(name){
        return this.Mod[name];
    }

    /**
     * Initialise all the extensions defined in the configuration object
     */
    initExtensions(){
        let exts = this.extensions;

        for(let i=0, len=exts.length; i<len; i++){
            let ext = exts[i];
            if(!this.ExtRegistry[ext.name]){
                this.loadExtension(ext);
            }
        }
    }

    /**
     * Load an extension module
     * @param  {Object} ext Extension config object
     */
    loadExtension(ext){
        if(!ext || !ext.name){
            return;
        }

        let name = ext.name;
        let path = ext.path;
        let modulePath;

        if(name && path){
            modulePath = ext.path + name;
        } else {
            name = name.replace('.js', '');
            modulePath = 'extensions/{}/{}'.replace(/{}/g, name);
        }

        // Trick to set config's publicPath dynamically for Webpack...
        __webpack_public_path__ = this.basePath;

        require(['./' + modulePath], (mod)=> {
            let inst = new mod(this, ext);
            inst.init();
            this.ExtRegistry[name] = inst;
        });
    }

    /**
     * Get an extension instance
     * @param  {String} name Name of the extension
     * @return {Object}      Extension instance
     */
    extension(name){
        return this.ExtRegistry[name];
    }

    /**
     * Check passed extension name exists
     * @param  {String}  name Name of the extension
     * @return {Boolean}
     */
    hasExtension(name){
        return !Types.isEmpty(this.ExtRegistry[name]);
    }

    /**
     * Destroy all the extensions defined in the configuration object
     */
    destroyExtensions(){
        let exts = this.extensions;

        for(let i=0, len=exts.length; i<len; i++){
            let ext = exts[i];
            let extInstance = this.ExtRegistry[ext.name];
            if(extInstance){
                extInstance.destroy();
                this.ExtRegistry[ext.name] = null;
            }
        }
    }

    loadThemes(){
        this.EvtManager(this.Evt.name.loadthemes);
    }

    /**
     * Load themes defined in the configuration object
     */
    _loadThemes(){
        let themes = this.themes;
        //Default theme config
        if(this.enableDefaultTheme){
            let defaultTheme = { name: 'default' };
            this.themes.push(defaultTheme);
        }
        if(Types.isArray(themes)){
            for(let i=0, len=themes.length; i<len; i++){
                let theme = themes[i];
                let name = theme.name;
                let path = theme.path;
                let styleId = this.prfxTf + name;
                if(name && !path){
                    path = this.themesPath + name + '/' + name + '.css';
                }
                else if(!name && theme.path){
                    name = 'theme{0}'.replace('{0}', i);
                }

                if(!this.isImported(path, 'link')){
                    this.import(styleId, path, null, 'link');
                }
            }
        }

        //Some elements need to be overriden for default theme
        //Reset button
        this.btnResetText = null;
        this.btnResetHtml = '<input type="button" value="" class="' +
            this.btnResetCssClass+'" title="Clear filters" />';

        //Paging buttons
        this.btnPrevPageHtml = '<input type="button" value="" class="' +
            this.btnPageCssClass+' previousPage" title="Previous page" />';
        this.btnNextPageHtml = '<input type="button" value="" class="' +
            this.btnPageCssClass+' nextPage" title="Next page" />';
        this.btnFirstPageHtml = '<input type="button" value="" class="' +
            this.btnPageCssClass+' firstPage" title="First page" />';
        this.btnLastPageHtml = '<input type="button" value="" class="' +
            this.btnPageCssClass+' lastPage" title="Last page" />';

        //Loader
        this.loader = true;
        this.loaderHtml = '<div class="defaultLoader"></div>';
        this.loaderText = null;
    }

    /**
     * Return stylesheet DOM element for a given theme name
     * @return {DOMElement} stylesheet element
     */
    getStylesheet(name='default'){
        return Dom.id(this.prfxTf + name);
    }

    /**
     * Destroy filter grid
     */
    destroy(){
        if(!this._hasGrid){
            return;
        }
        let rows = this.tbl.rows,
            Mod = this.Mod;

        this._clearFilters();

        if(this.isExternalFlt && !this.popupFilters){
            this.removeExternalFlts();
        }
        if(this.infDiv){
            this.removeToolbar();
        }
        if(this.highlightKeywords){
            Mod.highlightKeyword.unhighlightAll();
        }
        if(this.markActiveColumns){
            this.clearActiveColumns();
        }
        if(this.hasExtensions){
            this.destroyExtensions();
        }

        for(let j=this.refRow; j<this.nbRows; j++){
            // validate row
            this.validateRow(j, true);

            //removes alternating colors
            if(this.alternateRows){
                Mod.alternateRows.removeRowBg(j);
            }

        }//for j

        if(this.fltGrid && !this.gridLayout){
            this.fltGridEl = rows[this.filtersRowIndex];
            this.tbl.deleteRow(this.filtersRowIndex);
        }

        // Destroy modules
        Object.keys(Mod).forEach(function(key) {
            var feature = Mod[key];
            if(feature && Types.isFn(feature.destroy)){
                feature.destroy();
            }
        });

        Dom.removeClass(this.tbl, this.prfxTf);
        this.nbHiddenRows = 0;
        this.validRowsIndex = null;
        this.activeFlt = null;
        this._hasGrid = false;
        this.tbl = null;
    }

    /**
     * Generate container element for paging, reset button, rows counter etc.
     */
    setToolbar(){
        if(this.infDiv){
            return;
        }

        /*** container div ***/
        let infdiv = Dom.create('div', ['id', this.prfxInfDiv+this.id]);
        infdiv.className = this.infDivCssClass;

        //custom container
        if(this.toolBarTgtId){
            Dom.id(this.toolBarTgtId).appendChild(infdiv);
        }
        //grid-layout
        else if(this.gridLayout){
            let gridLayout = this.Mod.gridLayout;
            gridLayout.tblMainCont.appendChild(infdiv);
            infdiv.className = gridLayout.gridInfDivCssClass;
        }
        //default location: just above the table
        else{
            var cont = Dom.create('caption');
            cont.appendChild(infdiv);
            this.tbl.insertBefore(cont, this.tbl.firstChild);
        }
        this.infDiv = Dom.id(this.prfxInfDiv+this.id);

        /*** left div containing rows # displayer ***/
        let ldiv = Dom.create('div', ['id', this.prfxLDiv+this.id]);
        ldiv.className = this.lDivCssClass;
        infdiv.appendChild(ldiv);
        this.lDiv = Dom.id(this.prfxLDiv+this.id);

        /***    right div containing reset button
                + nb results per page select    ***/
        let rdiv = Dom.create('div', ['id', this.prfxRDiv+this.id]);
        rdiv.className = this.rDivCssClass;
        infdiv.appendChild(rdiv);
        this.rDiv = Dom.id(this.prfxRDiv+this.id);

        /*** mid div containing paging elements ***/
        let mdiv = Dom.create('div', ['id', this.prfxMDiv+this.id]);
        mdiv.className = this.mDivCssClass;
        infdiv.appendChild(mdiv);
        this.mDiv = Dom.id(this.prfxMDiv+this.id);

        // Enable help instructions by default if topbar is generated and not
        // explicitely set to false
        if(Types.isUndef(this.help)){
            if(!this.Mod.help){
                this.Mod.help = new Help(this);
            }
            this.Mod.help.init();
            this.help = true;
        }
    }

    /**
     * Remove toolbar container element
     */
    removeToolbar(){
        if(!this.infDiv){
            return;
        }
        this.infDiv.parentNode.removeChild(this.infDiv);
        this.infDiv = null;

        let tbl = this.tbl;
        let captions = Dom.tag(tbl, 'caption');
        if(captions.length > 0){
            [].forEach.call(captions, function(elm) {
                tbl.removeChild(elm);
            });
        }
    }

    /**
     * Remove all the external column filters
     */
    removeExternalFlts(){
        if(!this.isExternalFlt || !this.externalFltTgtIds){
            return;
        }
        let ids = this.externalFltTgtIds,
            len = ids.length;
        for(let ct=0; ct<len; ct++){
            let externalFltTgtId = ids[ct],
                externalFlt = Dom.id(externalFltTgtId);
            if(externalFlt){
                externalFlt.innerHTML = '';
            }
        }
    }

    /**
     * Check if given column implements a filter with custom options
     * @param  {Number}  colIndex Column's index
     * @return {Boolean}
     */
    isCustomOptions(colIndex) {
        return this.hasCustomOptions &&
            this.customOptions.cols.indexOf(colIndex) != -1;
    }

    /**
     * Returns an array [[value0, value1 ...],[text0, text1 ...]] with the
     * custom options values and texts
     * @param  {Number} colIndex Column's index
     * @return {Array}
     */
    getCustomOptions(colIndex){
        if(Types.isEmpty(colIndex) || !this.isCustomOptions(colIndex)){
            return;
        }

        let customOptions = this.customOptions;
        let cols = customOptions.cols;
        let optTxt = [], optArray = [];
        let index = cols.indexOf(colIndex);
        let slcValues = customOptions.values[index];
        let slcTexts = customOptions.texts[index];
        let slcSort = customOptions.sorts[index];

        for(let r=0, len=slcValues.length; r<len; r++){
            optArray.push(slcValues[r]);
            if(slcTexts[r]){
                optTxt.push(slcTexts[r]);
            } else {
                optTxt.push(slcValues[r]);
            }
        }
        if(slcSort){
            optArray.sort();
            optTxt.sort();
        }
        return [optArray, optTxt];
    }

    resetValues(){
        this.EvtManager(this.Evt.name.resetvalues);
    }

    /**
     * Reset persisted filter values
     */
    _resetValues(){
        //only loadFltOnDemand
        if(this.rememberGridValues && this.loadFltOnDemand){
            this._resetGridValues(this.fltsValuesCookie);
        }
        if(this.rememberPageLen && this.Mod.paging){
            this.Mod.paging.resetPageLength(this.pgLenCookie);
        }
        if(this.rememberPageNb && this.Mod.paging){
            this.Mod.paging.resetPage(this.pgNbCookie);
        }
    }

    /**
     * Reset persisted filter values when load filters on demand feature is
     * enabled
     * @param  {String} name cookie name storing filter values
     */
    _resetGridValues(name){
        if(!this.loadFltOnDemand){
            return;
        }
        let fltsValues = this.Mod.store.getFilterValues(name),
            slcFltsIndex = this.getFiltersByType(this.fltTypeSlc, true),
            multiFltsIndex = this.getFiltersByType(this.fltTypeMulti, true);

        //if the number of columns is the same as before page reload
        if(Number(fltsValues[(fltsValues.length-1)]) === this.fltIds.length){
            for(let i=0; i<(fltsValues.length - 1); i++){
                if(fltsValues[i]===' '){
                    continue;
                }
                let s, opt;
                let fltType = this.getFilterType(i);
                // if loadFltOnDemand, drop-down needs to contain stored
                // value(s) for filtering
                if(fltType===this.fltTypeSlc || fltType===this.fltTypeMulti){
                    let slc = Dom.id( this.fltIds[i] );
                    slc.options[0].selected = false;

                    //selects
                    if(slcFltsIndex.indexOf(i) != -1){
                        opt = Dom.createOpt(fltsValues[i],fltsValues[i],true);
                        slc.appendChild(opt);
                        this.hasStoredValues = true;
                    }
                    //multiple select
                    if(multiFltsIndex.indexOf(i) != -1){
                        s = fltsValues[i].split(' '+this.orOperator+' ');
                        for(let j=0, len=s.length; j<len; j++){
                            if(s[j]===''){
                                continue;
                            }
                            opt = Dom.createOpt(s[j],s[j],true);
                            slc.appendChild(opt);
                            this.hasStoredValues = true;
                        }
                    }// if multiFltsIndex
                }
                else if(fltType===this.fltTypeCheckList){
                    let checkList = this.Mod.checkList;
                    let divChk = checkList.checkListDiv[i];
                    divChk.title = divChk.innerHTML;
                    divChk.innerHTML = '';

                    let ul = Dom.create(
                        'ul',['id',this.fltIds[i]],['colIndex',i]);
                    ul.className = checkList.checkListCssClass;

                    let li0 = Dom.createCheckItem(
                        this.fltIds[i]+'_0', '', this.displayAllText);
                    li0.className = checkList.checkListItemCssClass;
                    ul.appendChild(li0);

                    divChk.appendChild(ul);

                    s = fltsValues[i].split(' '+this.orOperator+' ');
                    for(let j=0, len=s.length; j<len; j++){
                        if(s[j]===''){
                            continue;
                        }
                        let li = Dom.createCheckItem(
                            this.fltIds[i]+'_'+(j+1), s[j], s[j]);
                        li.className = checkList.checkListItemCssClass;
                        ul.appendChild(li);
                        li.check.checked = true;
                        checkList.setCheckListValues(li.check);
                        this.hasStoredValues = true;
                    }
                }
            }//end for

            if(!this.hasStoredValues && this.paging){
                this.Mod.paging.setPagingInfo();
            }
        }//end if
    }

    filter(){
        this.EvtManager(this.Evt.name.filter);
    }

    /**
     * Filter the table by retrieving the data from each cell in every single
     * row and comparing it to the search term for current column. A row is
     * hidden when all the search terms are not found in inspected row.
     *
     * TODO: Reduce complexity of this massive method
     */
    _filter(){
        if(!this.fltGrid || (!this._hasGrid && !this.isFirstLoad)){
            return;
        }
        //invoke onbefore callback
        if(this.onBeforeFilter){
            this.onBeforeFilter.call(null, this);
        }

        let row = this.tbl.rows,
            Mod = this.Mod,
            hiddenrows = 0;

        this.validRowsIndex = [];

        // removes keyword highlighting
        if(this.highlightKeywords){
            Mod.highlightKeyword.unhighlightAll();
        }
        //removes popup filters active icons
        if(this.popupFilters){
            Mod.popupFilter.buildIcons();
        }
        //removes active column header class
        if(this.markActiveColumns){
            this.clearActiveColumns();
        }
        // search args re-init
        this.searchArgs = this.getFiltersValue();

        var num_cell_data, nbFormat;
        var re_le = new RegExp(this.leOperator),
            re_ge = new RegExp(this.geOperator),
            re_l = new RegExp(this.lwOperator),
            re_g = new RegExp(this.grOperator),
            re_d = new RegExp(this.dfOperator),
            re_lk = new RegExp(Str.rgxEsc(this.lkOperator)),
            re_eq = new RegExp(this.eqOperator),
            re_st = new RegExp(this.stOperator),
            re_en = new RegExp(this.enOperator),
            // re_an = new RegExp(this.anOperator),
            // re_cr = new RegExp(this.curExp),
            re_em = this.emOperator,
            re_nm = this.nmOperator,
            re_re = new RegExp(Str.rgxEsc(this.rgxOperator));

        //keyword highlighting
        function highlight(str, ok, cell){
            /*jshint validthis:true */
            if(this.highlightKeywords && ok){
                str = str.replace(re_lk, '');
                str = str.replace(re_eq, '');
                str = str.replace(re_st, '');
                str = str.replace(re_en, '');
                let w = str;
                if(re_le.test(str) || re_ge.test(str) || re_l.test(str) ||
                    re_g.test(str) || re_d.test(str)){
                    w = Dom.getText(cell);
                }
                if(w !== ''){
                    Mod.highlightKeyword.highlight(
                        cell, w, Mod.highlightKeyword.highlightCssClass);
                }
            }
        }

        //looks for search argument in current row
        function hasArg(sA, cell_data, j){
            /*jshint validthis:true */
            let occurence,
                removeNbFormat = Helpers.removeNbFormat;
            //Search arg operator tests
            let hasLO = re_l.test(sA),
                hasLE = re_le.test(sA),
                hasGR = re_g.test(sA),
                hasGE = re_ge.test(sA),
                hasDF = re_d.test(sA),
                hasEQ = re_eq.test(sA),
                hasLK = re_lk.test(sA),
                // hasAN = re_an.test(sA),
                hasST = re_st.test(sA),
                hasEN = re_en.test(sA),
                hasEM = (re_em === sA),
                hasNM = (re_nm === sA),
                hasRE = re_re.test(sA);

            //Search arg dates tests
            let isLDate = hasLO && isValidDate(sA.replace(re_l,''), dtType);
            let isLEDate = hasLE && isValidDate(sA.replace(re_le,''), dtType);
            let isGDate = hasGR && isValidDate(sA.replace(re_g,''), dtType);
            let isGEDate = hasGE && isValidDate(sA.replace(re_ge,''), dtType);
            let isDFDate = hasDF && isValidDate(sA.replace(re_d,''), dtType);
            let isEQDate = hasEQ && isValidDate(sA.replace(re_eq,''), dtType);

            let dte1, dte2;
            //dates
            if(isValidDate(cell_data,dtType)){
                dte1 = formatDate(cell_data,dtType);
                // lower date
                if(isLDate){
                    dte2 = formatDate(sA.replace(re_l,''), dtType);
                    occurence = dte1 < dte2;
                }
                // lower equal date
                else if(isLEDate){
                    dte2 = formatDate(sA.replace(re_le,''), dtType);
                    occurence = dte1 <= dte2;
                }
                // greater equal date
                else if(isGEDate){
                    dte2 = formatDate(sA.replace(re_ge,''), dtType);
                    occurence = dte1 >= dte2;
                }
                // greater date
                else if(isGDate){
                    dte2 = formatDate(sA.replace(re_g,''), dtType);
                    occurence = dte1 > dte2;
                }
                // different date
                else if(isDFDate){
                    dte2 = formatDate(sA.replace(re_d,''), dtType);
                    occurence = dte1.toString() != dte2.toString();
                }
                // equal date
                else if(isEQDate){
                    dte2 = formatDate(sA.replace(re_eq,''), dtType);
                    occurence = dte1.toString() == dte2.toString();
                }
                // searched keyword with * operator doesn't have to be a date
                else if(re_lk.test(sA)){// like date
                    occurence = this._containsStr(
                        sA.replace(re_lk,''), cell_data, false);
                }
                else if(isValidDate(sA,dtType)){
                    dte2 = formatDate(sA,dtType);
                    occurence = dte1.toString() == dte2.toString();
                }
                //empty
                else if(hasEM){
                    occurence = Str.isEmpty(cell_data);
                }
                //non-empty
                else if(hasNM){
                    occurence = !Str.isEmpty(cell_data);
                }
            }

            else{
                //first numbers need to be formated
                if(this.hasColNbFormat && this.colNbFormat[j]){
                    num_cell_data = removeNbFormat(
                        cell_data, this.colNbFormat[j]);
                    nbFormat = this.colNbFormat[j];
                } else {
                    if(this.thousandsSeparator === ',' &&
                        this.decimalSeparator === '.'){
                        num_cell_data = removeNbFormat(cell_data, 'us');
                        nbFormat = 'us';
                    } else {
                        num_cell_data = removeNbFormat(cell_data, 'eu');
                        nbFormat = 'eu';
                    }
                }

                // first checks if there is any operator (<,>,<=,>=,!,*,=,{,},
                // rgx:)
                // lower equal
                if(hasLE){
                    occurence = num_cell_data <= removeNbFormat(
                        sA.replace(re_le, ''), nbFormat);
                }
                //greater equal
                else if(hasGE){
                    occurence = num_cell_data >= removeNbFormat(
                        sA.replace(re_ge, ''), nbFormat);
                }
                //lower
                else if(hasLO){
                    occurence = num_cell_data < removeNbFormat(
                        sA.replace(re_l, ''), nbFormat);
                }
                //greater
                else if(hasGR){
                    occurence = num_cell_data > removeNbFormat(
                        sA.replace(re_g, ''), nbFormat);
                }
                //different
                else if(hasDF){
                    occurence = this._containsStr(
                        sA.replace(re_d, ''), cell_data) ? false : true;
                }
                //like
                else if(hasLK){
                    occurence = this._containsStr(
                        sA.replace(re_lk, ''), cell_data, false);
                }
                //equal
                else if(hasEQ){
                    occurence = this._containsStr(
                        sA.replace(re_eq, ''), cell_data, true);
                }
                //starts with
                else if(hasST){
                    occurence = cell_data.indexOf(sA.replace(re_st, ''))===0 ?
                        true : false;
                }
                //ends with
                else if(hasEN){
                    let searchArg = sA.replace(re_en, '');
                    occurence =
                        cell_data.lastIndexOf(searchArg,cell_data.length-1) ===
                        (cell_data.length-1)-(searchArg.length-1) &&
                        cell_data.lastIndexOf(
                            searchArg, cell_data.length-1) > -1 ? true : false;
                }
                //empty
                else if(hasEM){
                    occurence = Str.isEmpty(cell_data);
                }
                //non-empty
                else if(hasNM){
                    occurence = !Str.isEmpty(cell_data);
                }
                //regexp
                else if(hasRE){
                    //in case regexp fires an exception
                    try{
                        //operator is removed
                        let srchArg = sA.replace(re_re,'');
                        let rgx = new RegExp(srchArg);
                        occurence = rgx.test(cell_data);
                    } catch(e) { occurence = false; }
                } else {
                    occurence = this._containsStr(sA, cell_data,
                        this.isExactMatch(j));
                }

            }//else
            return occurence;
        }//fn

        for(let k=this.refRow; k<this.nbRows; k++){
            /*** if table already filtered some rows are not visible ***/
            if(row[k].style.display === 'none'){
                row[k].style.display = '';
            }

            let cell = row[k].cells,
                nchilds = cell.length;

            // checks if row has exact cell #
            if(nchilds !== this.nbCells){
                continue;
            }

            let occurence = [],
                isRowValid = true,
                //only for single filter search
                singleFltRowValid = false;

            // this loop retrieves cell data
            for(let j=0; j<nchilds; j++){
                //searched keyword
                let sA = this.searchArgs[this.singleSearchFlt ? 0 : j];
                var dtType = this.hasColDateType ?
                        this.colDateType[j] : this.defaultDateType;
                if(sA === ''){
                    continue;
                }

                let cell_data = Str.matchCase(this.getCellData(cell[j]),
                    this.caseSensitive);

                //multiple search parameter operator ||
                let sAOrSplit = sA.split(this.orOperator),
                //multiple search || parameter boolean
                hasMultiOrSA = (sAOrSplit.length>1) ? true : false,
                //multiple search parameter operator &&
                sAAndSplit = sA.split(this.anOperator),
                //multiple search && parameter boolean
                hasMultiAndSA = sAAndSplit.length>1 ? true : false;

                //multiple sarch parameters
                if(hasMultiOrSA || hasMultiAndSA){
                    let cS,
                        occur = false,
                        s = hasMultiOrSA ? sAOrSplit : sAAndSplit;
                    for(let w=0, len=s.length; w<len; w++){
                        cS = Str.trim(s[w]);
                        occur = hasArg.call(this, cS, cell_data, j);
                        highlight.call(this, cS, occur, cell[j]);
                        if(hasMultiOrSA && occur){
                            break;
                        }
                        if(hasMultiAndSA && !occur){
                            break;
                        }
                    }
                    occurence[j] = occur;
                }
                //single search parameter
                else {
                    occurence[j] =
                        hasArg.call(this, Str.trim(sA), cell_data, j);
                    highlight.call(this, sA, occurence[j], cell[j]);
                }//else single param

                if(!occurence[j]){
                    isRowValid = false;
                }
                if(this.singleSearchFlt && occurence[j]){
                    singleFltRowValid = true;
                }
                if(this.popupFilters){
                    Mod.popupFilter.buildIcon(j, true);
                }
                if(this.markActiveColumns){
                    if(k === this.refRow){
                        if(this.onBeforeActiveColumn){
                            this.onBeforeActiveColumn.call(null, this, j);
                        }
                        Dom.addClass(
                            this.getHeaderElement(j),
                            this.activeColumnsCssClass);
                        if(this.onAfterActiveColumn){
                            this.onAfterActiveColumn.call(null, this, j);
                        }
                    }
                }
            }//for j

            if(this.singleSearchFlt && singleFltRowValid){
                isRowValid = true;
            }

            if(!isRowValid){
                this.validateRow(k, false);
                if(Mod.alternateRows){
                    Mod.alternateRows.removeRowBg(k);
                }
                // always visible rows need to be counted as valid
                if(this.hasVisibleRows && this.visibleRows.indexOf(k) !== -1){
                    this.validRowsIndex.push(k);
                } else {
                    hiddenrows++;
                }
            } else {
                this.validateRow(k, true);
                this.validRowsIndex.push(k);
                if(this.alternateRows){
                    Mod.alternateRows.setRowBg(k, this.validRowsIndex.length);
                }
                if(this.onRowValidated){
                    this.onRowValidated.call(null, this, k);
                }
            }
        }// for k

        this.nbVisibleRows = this.validRowsIndex.length;
        this.nbHiddenRows = hiddenrows;

        if(this.rememberGridValues){
            Mod.store.saveFilterValues(this.fltsValuesCookie);
        }
        //applies filter props after filtering process
        if(!this.paging){
            this.applyProps();
        } else {
            // Shouldn't need to care of that here...
            // TODO: provide a method in paging module
            Mod.paging.startPagingRow = 0;
            Mod.paging.currentPageNb = 1;
            //
            Mod.paging.setPagingInfo(this.validRowsIndex);
        }
        //invokes onafter callback
        if(this.onAfterFilter){
            this.onAfterFilter.call(null,this);
        }
    }

    /**
     * Re-apply the features/behaviour concerned by filtering/paging operation
     *
     * NOTE: this will disappear whenever custom events in place
     */
    applyProps(){
        let Mod = this.Mod;

        //shows rows always visible
        if(this.hasVisibleRows){
            this.enforceVisibility();
        }
        //columns operations
        if(this.hasExtension('colOps')){
            this.extension('colOps').calc();
        }

        //re-populates drop-down filters
        if(this.linkedFilters){
            this.linkFilters();
        }

        if(this.rowsCounter){
            Mod.rowsCounter.refresh(this.nbVisibleRows);
        }

        if(this.popupFilters){
            Mod.popupFilter.closeAll();
        }
    }

    /**
     * Return the data of a specified colum
     * @param  {Number} colIndex Column index
     * @param  {Boolean} includeHeaders  Optional: include headers row
     * @param  {Boolean} num     Optional: return unformatted number
     * @param  {Array} exclude   Optional: list of row indexes to be excluded
     * @return {Array}           Flat list of data for a column
     */
    getColValues(colIndex, includeHeaders=false, num=false, exclude=[]){
        if(!this.fltGrid){
            return;
        }
        let row = this.tbl.rows,
            colValues = [];

        if(includeHeaders){
            colValues.push(this.getHeadersText()[colIndex]);
        }

        for(let i=this.refRow; i<this.nbRows; i++){
            let isExludedRow = false;
            // checks if current row index appears in exclude array
            if(exclude.length > 0){
                isExludedRow = exclude.indexOf(i) != -1;
            }
            let cell = row[i].cells,
                nchilds = cell.length;

            // checks if row has exact cell # and is not excluded
            if(nchilds === this.nbCells && !isExludedRow){
                // this loop retrieves cell data
                for(let j=0; j<nchilds; j++){
                    if(j != colIndex || row[i].style.display !== ''){
                        continue;
                    }
                    let cell_data = this.getCellData(cell[j]),
                        nbFormat = this.colNbFormat ?
                            this.colNbFormat[colIndex] : null,
                        data = num ?
                                Helpers.removeNbFormat(cell_data, nbFormat) :
                                cell_data;
                    colValues.push(data);
                }
            }
        }
        return colValues;
    }

    /**
     * Return the filter's value of a specified column
     * @param  {Number} index Column index
     * @return {String}       Filter value
     */
    getFilterValue(index){
        if(!this.fltGrid){
            return;
        }
        let fltValue,
            flt = this.getFilterElement(index);
        if(!flt){
            return '';
        }

        let fltColType = this.getFilterType(index);
        if(fltColType !== this.fltTypeMulti &&
            fltColType !== this.fltTypeCheckList){
            fltValue = flt.value;
        }
        //mutiple select
        else if(fltColType === this.fltTypeMulti){
            fltValue = '';
            for(let j=0, len=flt.options.length; j<len; j++){
                if(flt.options[j].selected){
                    fltValue = fltValue.concat(
                        flt.options[j].value+' ' +
                        this.orOperator + ' '
                    );
                }
            }
            //removes last operator ||
            fltValue = fltValue.substr(0, fltValue.length-4);
        }
        //checklist
        else if(fltColType === this.fltTypeCheckList){
            if(flt.getAttribute('value') !== null){
                fltValue = flt.getAttribute('value');
                //removes last operator ||
                fltValue = fltValue.substr(0, fltValue.length-3);
            } else{
                fltValue = '';
            }
        }
        return fltValue;
    }

    /**
     * Return the filters' values
     * @return {Array} List of filters' values
     */
    getFiltersValue(){
        if(!this.fltGrid){
            return;
        }
        let searchArgs = [];
        for(let i=0, len=this.fltIds.length; i<len; i++){
            searchArgs.push(
                Str.trim(
                    Str.matchCase(this.getFilterValue(i), this.caseSensitive))
            );
        }
        return searchArgs;
    }

    /**
     * Return the ID of the filter of a specified column
     * @param  {Number} index Column's index
     * @return {String}       ID of the filter element
     */
    getFilterId(index){
        if(!this.fltGrid){
            return;
        }
        return this.fltIds[index];
    }

    /**
     * Return the list of ids of filters matching a specified type.
     * Note: hidden filters are also returned
     *
     * @param  {String} type  Filter type string ('input', 'select', 'multiple',
     *                        'checklist')
     * @param  {Boolean} bool If true returns columns indexes instead of IDs
     * @return {[type]}       List of element IDs or column indexes
     */
    getFiltersByType(type, bool){
        if(!this.fltGrid){
            return;
        }
        let arr = [];
        for(let i=0, len=this.fltIds.length; i<len; i++){
            let fltType = this.getFilterType(i);
            if(fltType === Str.lower(type)){
                let a = bool ? i : this.fltIds[i];
                arr.push(a);
            }
        }
        return arr;
    }

    /**
     * Return the filter's DOM element for a given column
     * @param  {Number} index     Column's index
     * @return {DOMElement}
     */
    getFilterElement(index){
        let fltId = this.fltIds[index];
        return Dom.id(fltId);
    }

    /**
     * Return the number of cells for a given row index
     * @param  {Number} rowIndex Index of the row
     * @return {Number}          Number of cells
     */
    getCellsNb(rowIndex=0){
        let tr = this.tbl.rows[rowIndex];
        return tr.cells.length;
    }

    /**
     * Return the number of filterable rows starting from reference row if
     * defined
     * @param  {Boolean} includeHeaders Include the headers row
     * @return {Number}                 Number of filterable rows
     */
    getRowsNb(includeHeaders){
        let s = Types.isUndef(this.refRow) ? 0 : this.refRow,
            ntrs = this.tbl.rows.length;
        if(includeHeaders){ s = 0; }
        return parseInt(ntrs-s, 10);
    }

    /**
     * Return the data of a given cell
     * @param  {DOMElement} cell Cell's DOM object
     * @return {String}
     */
    getCellData(cell){
        var idx = cell.cellIndex;
        //Check for customCellData callback
        if(this.customCellData && this.customCellDataCols.indexOf(idx) != -1){
            return this.customCellData.call(null, this, cell, idx);
        } else {
            return Dom.getText(cell);
        }
    }

    /**
     * Return the table data with following format:
     * [
     *     [rowIndex, [value0, value1...]],
     *     [rowIndex, [value0, value1...]]
     * ]
     * @param  {Boolean} includeHeaders  Optional: include headers row
     * @return {Array}
     *
     * TODO: provide an API returning data in JSON format
     */
    getTableData(includeHeaders=false){
        let rows = this.tbl.rows;
        let tblData = [];
        if(includeHeaders){
            tblData.push([this.getHeadersRowIndex(), this.getHeadersText()]);
        }
        for(let k=this.refRow; k<this.nbRows; k++){
            let rowData = [k, []];
            let cells = rows[k].cells;
            for(let j=0, len=cells.length; j<len; j++){
                let cellData = this.getCellData(cells[j]);
                rowData[1].push(cellData);
            }
            tblData.push(rowData);
        }
        return tblData;
    }

    /**
     * Return the filtered data with following format:
     * [
     *     [rowIndex, [value0, value1...]],
     *     [rowIndex, [value0, value1...]]
     * ]
     * @param  {Boolean} includeHeaders  Optional: include headers row
     * @return {Array}
     *
     * TODO: provide an API returning data in JSON format
     */
    getFilteredData(includeHeaders=false){
        if(!this.validRowsIndex){
            return [];
        }
        let rows = this.tbl.rows,
            filteredData = [];
        if(includeHeaders){
            filteredData.push([this.getHeadersRowIndex(),
                this.getHeadersText()]);
        }

        let validRows = this.getValidRows(true);
        for(let i=0; i<validRows.length; i++){
            let rData = [this.validRowsIndex[i], []],
                cells = rows[this.validRowsIndex[i]].cells;
            for(let k=0; k<cells.length; k++){
                let cellData = this.getCellData(cells[k]);
                rData[1].push(cellData);
            }
            filteredData.push(rData);
        }
        return filteredData;
    }

    /**
     * Return the filtered data for a given column index
     * @param  {Number} colIndex Colmun's index
     * @param  {Boolean} includeHeaders  Optional: include headers row
     * @return {Array}           Flat list of values ['val0','val1','val2'...]
     *
     * TODO: provide an API returning data in JSON format
     */
    getFilteredDataCol(colIndex, includeHeaders=false){
        if(Types.isUndef(colIndex)){
            return [];
        }
        let data =  this.getFilteredData(),
            colData = [];
        if(includeHeaders){
            colData.push(this.getHeadersText()[colIndex]);
        }
        for(let i=0, len=data.length; i<len; i++){
            let r = data[i],
                //cols values of current row
                d = r[1],
                //data of searched column
                c = d[colIndex];
            colData.push(c);
        }
        return colData;
    }

    /**
     * Get the display value of a row
     * @param  {RowElement} DOM element of the row
     * @return {String}     Usually 'none' or ''
     */
    getRowDisplay(row){
        if(!Types.isObj(row)){
            return null;
        }
        return row.style.display;
    }

    /**
     * Validate/invalidate row by setting the 'validRow' attribute on the row
     * @param  {Number}  rowIndex Index of the row
     * @param  {Boolean} isValid
     */
    validateRow(rowIndex, isValid){
        let row = this.tbl.rows[rowIndex];
        if(!row || typeof isValid !== 'boolean'){
            return;
        }

        // always visible rows are valid
        if(this.hasVisibleRows && this.visibleRows.indexOf(rowIndex) !== -1){
            isValid = true;
        }

        let displayFlag = isValid ? '' : 'none',
            validFlag = isValid ? 'true' : 'false';
        row.style.display = displayFlag;

        if(this.paging){
            row.setAttribute('validRow', validFlag);
        }
    }

    /**
     * Validate all filterable rows
     */
    validateAllRows(){
        if(!this._hasGrid){
            return;
        }
        this.validRowsIndex = [];
        for(let k=this.refRow; k<this.nbFilterableRows; k++){
            this.validateRow(k, true);
            this.validRowsIndex.push(k);
        }
    }

    /**
     * Set search value to a given filter
     * @param {Number} index     Column's index
     * @param {String} searcharg Search term
     */
    setFilterValue(index, searcharg=''){
        if((!this.fltGrid && !this.isFirstLoad) ||
            !this.getFilterElement(index)){
            return;
        }
        let slc = this.getFilterElement(index),
            fltColType = this.getFilterType(index);

        if(fltColType !== this.fltTypeMulti &&
            fltColType != this.fltTypeCheckList){
            slc.value = searcharg;
        }
        //multiple selects
        else if(fltColType === this.fltTypeMulti){
            let s = searcharg.split(' '+this.orOperator+' ');
            // let ct = 0; //keywords counter
            for(let j=0, len=slc.options.length; j<len; j++){
                let option = slc.options[j];
                if(s==='' || s[0]===''){
                    option.selected = false;
                }
                if(option.value===''){
                    option.selected = false;
                }
                if(option.value!=='' &&
                    Arr.has(s, option.value, true)){
                    option.selected = true;
                }//if
            }//for j
        }
        //checklist
        else if(fltColType === this.fltTypeCheckList){
            searcharg = Str.matchCase(searcharg, this.caseSensitive);
            let sarg = searcharg.split(' '+this.orOperator+' ');
            let lisNb = Dom.tag(slc,'li').length;

            slc.setAttribute('value', '');
            slc.setAttribute('indexes', '');

            for(let k=0; k<lisNb; k++){
                let li = Dom.tag(slc,'li')[k],
                    lbl = Dom.tag(li,'label')[0],
                    chk = Dom.tag(li,'input')[0],
                    lblTxt = Str.matchCase(
                        Dom.getText(lbl), this.caseSensitive);
                if(lblTxt !== '' && Arr.has(sarg, lblTxt, true)){
                    chk.checked = true;
                    this.Mod.checkList.setCheckListValues(chk);
                }
                else{
                    chk.checked = false;
                    this.Mod.checkList.setCheckListValues(chk);
                }
            }
        }
    }

    /**
     * Set them columns' widths as per configuration
     * @param {Number} rowIndex Optional row index to apply the widths to
     * @param {Element} tbl DOM element
     */
    setColWidths(rowIndex, tbl){
        if(!this.fltGrid || !this.hasColWidths){
            return;
        }
        tbl = tbl || this.tbl;
        let rIndex;
        if(rowIndex===undefined){
            rIndex = tbl.rows[0].style.display!='none' ? 0 : 1;
        } else{
            rIndex = rowIndex;
        }

        setWidths.call(this);

        function setWidths(){
            /*jshint validthis:true */
            let nbCols = this.nbCells;
            let colWidths = this.colWidths;
            let colTags = Dom.tag(tbl, 'col');
            let tblHasColTag = colTags.length > 0;
            let frag = !tblHasColTag ? doc.createDocumentFragment() : null;
            for(let k=0; k<nbCols; k++){
                let col;
                if(tblHasColTag){
                    col = colTags[k];
                } else {
                    col = Dom.create('col', ['id', this.id+'_col_'+k]);
                    frag.appendChild(col);
                }
                col.style.width = colWidths[k];
            }
            if(!tblHasColTag){
                tbl.insertBefore(frag, tbl.firstChild);
            }
        }
    }

    /**
     * Makes defined rows always visible
     */
    enforceVisibility(){
        if(!this.hasVisibleRows){
            return;
        }
        for(let i=0, len=this.visibleRows.length; i<len; i++){
            let row = this.visibleRows[i];
            //row index cannot be > nrows
            if(row <= this.nbRows){
                this.validateRow(row, true);
            }
        }
    }

    clearFilters(){
        this.EvtManager(this.Evt.name.clear);
    }

    /**
     * Clear all the filters' values
     */
    _clearFilters(){
        if(!this.fltGrid){
            return;
        }
        if(this.onBeforeReset){
            this.onBeforeReset.call(null, this, this.getFiltersValue());
        }
        for(let i=0, len=this.fltIds.length; i<len; i++){
            this.setFilterValue(i, '');
        }
        if(this.linkedFilters){
            this.linkFilters();
        }
        if(this.rememberPageLen){ Cookie.remove(this.pgLenCookie); }
        if(this.rememberPageNb){ Cookie.remove(this.pgNbCookie); }
        if(this.onAfterReset){ this.onAfterReset.call(null, this); }
    }

    /**
     * Clears filtered columns visual indicator (background color)
     */
    clearActiveColumns(){
        for(let i=0, len=this.getCellsNb(this.headersRow); i<len; i++){
            Dom.removeClass(
                this.getHeaderElement(i), this.activeColumnsCssClass);
        }
    }

    /**
     * Refresh the filters subject to linking ('select', 'multiple',
     * 'checklist' type)
     */
    linkFilters(){
        if(!this.activeFilterId){
            return;
        }
        let slcA1 = this.getFiltersByType(this.fltTypeSlc, true),
            slcA2 = this.getFiltersByType(this.fltTypeMulti, true),
            slcA3 = this.getFiltersByType(this.fltTypeCheckList, true),
            slcIndex = slcA1.concat(slcA2);
        slcIndex = slcIndex.concat(slcA3);

        let activeFlt = this.activeFilterId.split('_')[0];
        activeFlt = activeFlt.split(this.prfxFlt)[1];
        let slcSelectedValue;
        for(let i=0, len=slcIndex.length; i<len; i++){
            let curSlc = Dom.id(this.fltIds[slcIndex[i]]);
            slcSelectedValue = this.getFilterValue(slcIndex[i]);

            // Welcome to cyclomatic complexity hell :)
            // TODO: simplify/refactor if statement
            if(activeFlt!==slcIndex[i] ||
                (this.paging && slcA1.indexOf(slcIndex[i]) != -1 &&
                    activeFlt === slcIndex[i] ) ||
                (!this.paging && (slcA3.indexOf(slcIndex[i]) != -1 ||
                    slcA2.indexOf(slcIndex[i]) != -1)) ||
                slcSelectedValue === this.displayAllText ){

                if(slcA3.indexOf(slcIndex[i]) != -1){
                    this.Mod.checkList.checkListDiv[slcIndex[i]].innerHTML = '';
                } else {
                    curSlc.innerHTML = '';
                }

                //1st option needs to be inserted
                if(this.loadFltOnDemand) {
                    let opt0 = Dom.createOpt(this.displayAllText, '');
                    if(curSlc){
                        curSlc.appendChild(opt0);
                    }
                }

                if(slcA3.indexOf(slcIndex[i]) != -1){
                    this.Mod.checkList._build(slcIndex[i]);
                } else {
                    this.Mod.dropdown._build(slcIndex[i], true);
                }

                this.setFilterValue(slcIndex[i], slcSelectedValue);
            }
        }// for i
    }

    /**
     * Re-generate the filters grid bar when previously removed
     */
    _resetGrid(){
        if(this.isFirstLoad){
            return;
        }

        let Mod = this.Mod;
        let tbl = this.tbl;
        let rows = tbl.rows;
        let filtersRowIndex = this.filtersRowIndex;
        let filtersRow = rows[filtersRowIndex];

        // grid was removed, grid row element is stored in fltGridEl property
        if(!this.gridLayout){
            // If table has a thead ensure the filters row is appended in the
            // thead element
            if(tbl.tHead){
                var tempRow = tbl.tHead.insertRow(this.filtersRowIndex);
                tbl.tHead.replaceChild(this.fltGridEl, tempRow);
            } else {
                filtersRow.parentNode.insertBefore(this.fltGridEl, filtersRow);
            }
        }

        // filters are appended in external placeholders elements
        if(this.isExternalFlt){
            let externalFltTgtIds = this.externalFltTgtIds;
            for(let ct=0, len=externalFltTgtIds.length; ct<len; ct++){
                let extFlt = Dom.id(externalFltTgtIds[ct]);

                if(!extFlt){ continue; }

                let externalFltEl = this.externalFltEls[ct];
                extFlt.appendChild(externalFltEl);
                let colFltType = this.getFilterType(ct);
                //IE special treatment for gridLayout, appended filters are
                //empty
                if(this.gridLayout &&
                    externalFltEl.innerHTML === '' &&
                    colFltType !== this.fltTypeInp){
                    if(colFltType === this.fltTypeSlc ||
                        colFltType === this.fltTypeMulti){
                        Mod.dropdown.build(ct);
                    }
                    if(colFltType === this.fltTypeCheckList){
                        Mod.checkList.build(ct);
                    }
                }
            }
        }

        this.nbFilterableRows = this.getRowsNb();
        this.nbVisibleRows = this.nbFilterableRows;
        this.nbRows = rows.length;

        if(this.popupFilters){
            this.headersRow++;
            Mod.popupFilter.reset();
        }

        if(!this.gridLayout){
            Dom.addClass(this.tbl, this.prfxTf);
        }
        this._hasGrid = true;
    }

    /**
     * Determines if passed filter column implements exact query match
     * @param  {Number}  colIndex [description]
     * @return {Boolean}          [description]
     */
    isExactMatch(colIndex){
        let fltType = this.getFilterType(colIndex);
        return this.exactMatchByCol[colIndex] || this.exactMatch ||
            (fltType!==this.fltTypeInp);
    }

    /**
     * Checks if passed data contains the searched arg
     * @param  {String} arg         Search term
     * @param  {String} data        Data string
     * @param  {Boolean} exactMatch Exact match
     * @return {Boolean]}
     *
     * TODO: move into string module, remove fltType in order to decouple it
     * from TableFilter module
     */
    _containsStr(arg, data, exactMatch){
        // Improved by Cedric Wartel (cwl)
        // automatic exact match for selects and special characters are now
        // filtered
        let regexp,
            modifier = this.caseSensitive ? 'g' : 'gi';
        if(exactMatch){
            regexp = new RegExp(
                '(^\\s*)'+ Str.rgxEsc(arg) +'(\\s*$)', modifier);
        } else {
            regexp = new RegExp(Str.rgxEsc(arg), modifier);
        }
        return regexp.test(data);
    }

    /**
     * Check if passed script or stylesheet is already imported
     * @param  {String}  filePath Ressource path
     * @param  {String}  type     Possible values: 'script' or 'link'
     * @return {Boolean}
     */
    isImported(filePath, type){
        let imported = false,
            importType = !type ? 'script' : type,
            attr = importType == 'script' ? 'src' : 'href',
            files = Dom.tag(doc, importType);
        for (let i=0, len=files.length; i<len; i++){
            if(files[i][attr] === undefined){
                continue;
            }
            if(files[i][attr].match(filePath)){
                imported = true;
                break;
            }
        }
        return imported;
    }

    /**
     * Import script or stylesheet
     * @param  {String}   fileId   Ressource ID
     * @param  {String}   filePath Ressource path
     * @param  {Function} callback Callback
     * @param  {String}   type     Possible values: 'script' or 'link'
     */
    import(fileId, filePath, callback, type){
        let ftype = !type ? 'script' : type,
            imported = this.isImported(filePath, ftype);
        if(imported){
            return;
        }
        let o = this,
            isLoaded = false,
            file,
            head = Dom.tag(doc, 'head')[0];

        if(Str.lower(ftype) === 'link'){
            file = Dom.create(
                'link',
                ['id', fileId], ['type', 'text/css'],
                ['rel', 'stylesheet'], ['href', filePath]
            );
        } else {
            file = Dom.create(
                'script', ['id', fileId],
                ['type', 'text/javascript'], ['src', filePath]
            );
        }

        //Browser <> IE onload event works only for scripts, not for stylesheets
        file.onload = file.onreadystatechange = function(){
            if(!isLoaded &&
                (!this.readyState || this.readyState === 'loaded' ||
                    this.readyState === 'complete')){
                isLoaded = true;
                if(typeof callback === 'function'){
                    callback.call(null, o);
                }
            }
        };
        file.onerror = function(){
            throw new Error('TF script could not load: ' + filePath);
        };
        head.appendChild(file);
    }

    /**
     * Check if table has filters grid
     * @return {Boolean}
     */
    hasGrid(){
        return this._hasGrid;
    }

    /**
     * Get list of filter IDs
     * @return {[type]} [description]
     */
    getFiltersId(){
        return this.fltIds || [];
    }

    /**
     * Get filtered (valid) rows indexes
     * @param  {Boolean} reCalc Force calculation of filtered rows list
     * @return {Array}          List of row indexes
     */
    getValidRows(reCalc){
        if(!reCalc){
            return this.validRowsIndex;
        }

        this.validRowsIndex = [];
        for(let k=this.refRow; k<this.getRowsNb(true); k++){
            let r = this.tbl.rows[k];
            if(!this.paging){
                if(this.getRowDisplay(r) !== 'none'){
                    this.validRowsIndex.push(r.rowIndex);
                }
            } else {
                if(r.getAttribute('validRow') === 'true' ||
                    r.getAttribute('validRow') === null){
                    this.validRowsIndex.push(r.rowIndex);
                }
            }
        }
        return this.validRowsIndex;
    }

    /**
     * Get the index of the row containing the filters
     * @return {Number}
     */
    getFiltersRowIndex(){
        return this.filtersRowIndex;
    }

    /**
     * Get the index of the headers row
     * @return {Number}
     */
    getHeadersRowIndex(){
        return this.headersRow;
    }

    /**
     * Get the row index from where the filtering process start (1st filterable
     * row)
     * @return {Number}
     */
    getStartRowIndex(){
        return this.refRow;
    }

    /**
     * Get the index of the last row
     * @return {Number}
     */
    getLastRowIndex(){
        if(!this._hasGrid){
            return;
        }
        return (this.nbRows-1);
    }

    /**
     * Get the header DOM element for a given column index
     * @param  {Number} colIndex Column index
     * @return {Object}
     */
    getHeaderElement(colIndex){
        let table = this.gridLayout ? this.Mod.gridLayout.headTbl : this.tbl;
        let tHead = Dom.tag(table, 'thead');
        let headersRow = this.headersRow;
        let header;
        for(let i=0; i<this.nbCells; i++){
            if(i !== colIndex){
                continue;
            }
            if(tHead.length === 0){
                header = table.rows[headersRow].cells[i];
            }
            if(tHead.length === 1){
                header = tHead[0].rows[headersRow].cells[i];
            }
            break;
        }
        return header;
    }

    /**
     * Return the list of headers' text
     * @return {Array} list of headers' text
     */
    getHeadersText(){
        let headers = [];
        for(let j=0; j<this.nbCells; j++){
            let header = this.getHeaderElement(j);
            let headerText = Dom.getText(header);
            headers.push(headerText);
        }
        return headers;
    }

    /**
     * Return the filter type for a specified column
     * @param  {Number} colIndex Column's index
     * @return {String}
     */
    getFilterType(colIndex){
        let colType = this.cfg['col_'+colIndex];
        return !colType ? this.fltTypeInp : Str.lower(colType);
    }

    /**
     * Get the total number of filterable rows
     * @return {Number}
     */
    getFilterableRowsNb(){
        return this.getRowsNb(false);
    }

    /**
     * Get the configuration object (literal object)
     * @return {Object}
     */
    config(){
        return this.cfg;
    }
}
