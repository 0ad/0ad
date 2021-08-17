import Dom from '../../dom';

export default class AdapterEzEditTable {
    /**
     * Adapter module for ezEditTable, an external library providing advanced
     * grid features (selection and edition):
     * http://codecanyon.net/item/ezedittable-enhance-html-tables/2425123?ref=koalyptus
     *
     * @param {Object} tf TableFilter instance
     */
    constructor(tf, cfg){
        // ezEditTable config
        this.initialized = false;
        this.desc = cfg.description || 'ezEditTable adapter';
        this.filename = cfg.filename || 'ezEditTable.js';
        this.vendorPath = cfg.vendor_path;
        this.loadStylesheet = Boolean(cfg.load_stylesheet);
        this.stylesheet = cfg.stylesheet || this.vendorPath + 'ezEditTable.css';
        this.stylesheetName = cfg.stylesheet_name || 'ezEditTableCss';
        this.err = 'Failed to instantiate EditTable object.\n"ezEditTable" ' +
            'dependency not found.';
        // Enable the ezEditTable's scroll into view behaviour if grid layout on
        cfg.scroll_into_view =  cfg.scroll_into_view===false ?
            false : tf.gridLayout;

        this._ezEditTable = null;
        this.cfg = cfg;
        this.tf = tf;
    }

    /**
     * Conditionally load ezEditTable library and set advanced grid
     * @return {[type]} [description]
     */
    init(){
        var tf = this.tf;
        if(window.EditTable){
            this._setAdvancedGrid();
        } else {
            var path = this.vendorPath + this.filename;
            tf.import(this.filename, path, ()=> { this._setAdvancedGrid(); });
        }
        if(this.loadStylesheet && !tf.isImported(this.stylesheet, 'link')){
            tf.import(this.stylesheetName, this.stylesheet, null, 'link');
        }
    }

    /**
     * Instantiate ezEditTable component for advanced grid features
     */
    _setAdvancedGrid(){
        var tf = this.tf;

        //start row for EditTable constructor needs to be calculated
        var startRow,
            cfg = this.cfg,
            thead = Dom.tag(tf.tbl, 'thead');

        //if thead exists and startRow not specified, startRow is calculated
        //automatically by EditTable
        if(thead.length > 0 && !cfg.startRow){
            startRow = undefined;
        }
        //otherwise startRow config property if any or TableFilter refRow
        else{
            startRow = cfg.startRow || tf.refRow;
        }

        cfg.base_path = cfg.base_path  || tf.basePath + 'ezEditTable/';
        var editable = cfg.editable;
        var selectable = cfg.selection;

        if(selectable){
            cfg.default_selection = cfg.default_selection || 'row';
        }
        //CSS Styles
        cfg.active_cell_css = cfg.active_cell_css || 'ezETSelectedCell';

        var _lastValidRowIndex = 0;
        var _lastRowIndex = 0;

        if(selectable){
            //Row navigation needs to be calculated according to TableFilter's
            //validRowsIndex array
            var onAfterSelection = function(et, selectedElm, e){
                var slc = et.Selection;
                //Next valid filtered row needs to be selected
                var doSelect = function(nextRowIndex){
                    if(et.defaultSelection === 'row'){
                        slc.SelectRowByIndex(nextRowIndex);
                    } else {
                        et.ClearSelections();
                        var cellIndex = selectedElm.cellIndex,
                            row = tf.tbl.rows[nextRowIndex];
                        if(et.defaultSelection === 'both'){
                            slc.SelectRowByIndex(nextRowIndex);
                        }
                        if(row){
                            slc.SelectCell(row.cells[cellIndex]);
                        }
                    }
                    //Table is filtered
                    if(tf.validRowsIndex.length !== tf.getRowsNb()){
                        var r = tf.tbl.rows[nextRowIndex];
                        if(r){
                            r.scrollIntoView(false);
                        }
                        if(cell){
                            if(cell.cellIndex === (tf.getCellsNb()-1) &&
                                tf.gridLayout){
                                tf.tblCont.scrollLeft = 100000000;
                            }
                            else if(cell.cellIndex===0 && tf.gridLayout){
                                tf.tblCont.scrollLeft = 0;
                            } else {
                                cell.scrollIntoView(false);
                            }
                        }
                    }
                };

                //table is not filtered
                if(!tf.validRowsIndex){
                    return;
                }
                var validIndexes = tf.validRowsIndex,
                    validIdxLen = validIndexes.length,
                    row = et.defaultSelection !== 'row' ?
                        selectedElm.parentNode : selectedElm,
                    //cell for default_selection = 'both' or 'cell'
                    cell = selectedElm.nodeName==='TD' ? selectedElm : null,
                    keyCode = e !== undefined ? et.Event.GetKey(e) : 0,
                    isRowValid = validIndexes.indexOf(row.rowIndex) !== -1,
                    nextRowIndex,
                    paging = tf.feature('paging'),
                    //pgup/pgdown keys
                    d = (keyCode === 34 || keyCode === 33 ?
                        (paging && paging.pagingLength || et.nbRowsPerPage) :1);

                //If next row is not valid, next valid filtered row needs to be
                //calculated
                if(!isRowValid){
                    //Selection direction up/down
                    if(row.rowIndex>_lastRowIndex){
                        //last row
                        if(row.rowIndex >= validIndexes[validIdxLen-1]){
                            nextRowIndex = validIndexes[validIdxLen-1];
                        } else {
                            var calcRowIndex = (_lastValidRowIndex + d);
                            if(calcRowIndex > (validIdxLen-1)){
                                nextRowIndex = validIndexes[validIdxLen-1];
                            } else {
                                nextRowIndex = validIndexes[calcRowIndex];
                            }
                        }
                    } else{
                        //first row
                        if(row.rowIndex <= validIndexes[0]){
                            nextRowIndex = validIndexes[0];
                        } else {
                            var v = validIndexes[_lastValidRowIndex - d];
                            nextRowIndex = v ? v : validIndexes[0];
                        }
                    }
                    _lastRowIndex = row.rowIndex;
                    doSelect(nextRowIndex);
                } else {
                    //If filtered row is valid, special calculation for
                    //pgup/pgdown keys
                    if(keyCode!==34 && keyCode!==33){
                        _lastValidRowIndex = validIndexes.indexOf(row.rowIndex);
                        _lastRowIndex = row.rowIndex;
                    } else {
                        if(keyCode === 34){ //pgdown
                            //last row
                            if((_lastValidRowIndex + d) <= (validIdxLen-1)){
                                nextRowIndex = validIndexes[
                                _lastValidRowIndex + d];
                            } else {
                                nextRowIndex = [validIdxLen-1];
                            }
                        } else { //pgup
                            //first row
                            if((_lastValidRowIndex - d) <= validIndexes[0]){
                                nextRowIndex = validIndexes[0];
                            } else {
                                nextRowIndex = validIndexes[
                                    _lastValidRowIndex - d];
                            }
                        }
                        _lastRowIndex = nextRowIndex;
                        _lastValidRowIndex = validIndexes.indexOf(nextRowIndex);
                        doSelect(nextRowIndex);
                    }
                }
            };

            //Page navigation has to be enforced whenever selected row is out of
            //the current page range
            var onBeforeSelection = function(et, selectedElm){
                var row = et.defaultSelection !== 'row' ?
                    selectedElm.parentNode : selectedElm;
                if(tf.paging){
                    if(tf.feature('paging').nbPages > 1){
                        var paging = tf.feature('paging');
                        //page length is re-assigned in case it has changed
                        et.nbRowsPerPage = paging.pagingLength;
                        var validIndexes = tf.validRowsIndex,
                            validIdxLen = validIndexes.length,
                            pagingEndRow = parseInt(paging.startPagingRow, 10) +
                                parseInt(paging.pagingLength, 10);
                        var rowIndex = row.rowIndex;

                        if((rowIndex === validIndexes[validIdxLen-1]) &&
                            paging.currentPageNb!==paging.nbPages){
                            paging.setPage('last');
                        }
                        else if((rowIndex == validIndexes[0]) &&
                            paging.currentPageNb!==1){
                            paging.setPage('first');
                        }
                        else if(rowIndex > validIndexes[pagingEndRow-1] &&
                            rowIndex < validIndexes[validIdxLen-1]){
                            paging.setPage('next');
                        }
                        else if(
                            rowIndex < validIndexes[paging.startPagingRow] &&
                            rowIndex > validIndexes[0]){
                            paging.setPage('previous');
                        }
                    }
                }
            };

            //Selected row needs to be visible when paging is activated
            if(tf.paging){
                tf.feature('paging').onAfterChangePage = function(paging){
                    var advGrid = paging.tf.extension('advancedGrid');
                    var et = advGrid._ezEditTable;
                    var slc = et.Selection;
                    var row = slc.GetActiveRow();
                    if(row){
                        row.scrollIntoView(false);
                    }
                    var cell = slc.GetActiveCell();
                    if(cell){
                        cell.scrollIntoView(false);
                    }
                };
            }

            //Rows navigation when rows are filtered is performed with the
            //EditTable row selection callback events
            if(cfg.default_selection==='row'){
                var fnB = cfg.on_before_selected_row;
                cfg.on_before_selected_row = function(){
                    onBeforeSelection(arguments[0], arguments[1], arguments[2]);
                    if(fnB){
                        fnB.call(
                            null, arguments[0], arguments[1], arguments[2]);
                    }
                };
                var fnA = cfg.on_after_selected_row;
                cfg.on_after_selected_row = function(){
                    onAfterSelection(arguments[0], arguments[1], arguments[2]);
                    if(fnA){
                        fnA.call(
                            null, arguments[0], arguments[1], arguments[2]);
                    }
                };
            } else {
                var fnD = cfg.on_before_selected_cell;
                cfg.on_before_selected_cell = function(){
                    onBeforeSelection(arguments[0], arguments[1], arguments[2]);
                    if(fnD){
                        fnD.call(
                            null, arguments[0], arguments[1], arguments[2]);
                    }
                };
                var fnC = cfg.on_after_selected_cell;
                cfg.on_after_selected_cell = function(){
                    onAfterSelection(arguments[0], arguments[1], arguments[2]);
                    if(fnC){
                        fnC.call(
                            null, arguments[0], arguments[1], arguments[2]);
                    }
                };
            }
        }
        if(editable){
            //Added or removed rows, TF rows number needs to be re-calculated
            var fnE = cfg.on_added_dom_row;
            cfg.on_added_dom_row = function(){
                tf.nbFilterableRows++;
                if(!tf.paging){
                    tf.feature('rowsCounter').refresh();
                } else {
                    tf.nbRows++;
                    tf.nbVisibleRows++;
                    tf.nbFilterableRows++;
                    tf.paging=false;
                    tf.feature('paging').destroy();
                    tf.feature('paging').reset();
                }
                if(tf.alternateRows){
                    tf.feature('alternateRows').init();
                }
                if(fnE){
                    fnE.call(null, arguments[0], arguments[1], arguments[2]);
                }
            };
            if(cfg.actions && cfg.actions['delete']){
                var fnF = cfg.actions['delete'].on_after_submit;
                cfg.actions['delete'].on_after_submit = function(){
                    tf.nbFilterableRows--;
                    if(!tf.paging){
                        tf.feature('rowsCounter').refresh();
                    } else {
                        tf.nbRows--;
                        tf.nbVisibleRows--;
                        tf.nbFilterableRows--;
                        tf.paging=false;
                        tf.feature('paging').destroy();
                        tf.feature('paging').reset(false);
                    }
                    if(tf.alternateRows){
                        tf.feature('alternateRows').init();
                    }
                    if(fnF){
                        fnF.call(null, arguments[0], arguments[1]);
                    }
                };
            }
        }

        try{
            this._ezEditTable = new EditTable(tf.id, cfg, startRow);
            this._ezEditTable.Init();
        } catch(e) { throw new Error(this.err); }

        this.initialized = true;
    }

    /**
     * Reset advanced grid when previously removed
     */
    reset(){
        var ezEditTable = this._ezEditTable;
        if(ezEditTable){
            if(this.cfg.selection){
                ezEditTable.Selection.Set();
            }
            if(this.cfg.editable){
                ezEditTable.Editable.Set();
            }
        }
    }

    /**
     * Remove advanced grid
     */
    destroy(){
        var ezEditTable = this._ezEditTable;
        if(ezEditTable){
            if(this.cfg.selection){
                ezEditTable.Selection.ClearSelections();
                ezEditTable.Selection.Remove();
            }
            if(this.cfg.editable){
                ezEditTable.Editable.Remove();
            }
        }
        this.initialized = false;
    }
}
