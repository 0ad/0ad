import Types from '../../types';
import Dom from '../../dom';
import Event from '../../event';
import DateHelper from '../../date';
import Helpers from '../../helpers';

export default class AdapterSortableTable{

    /**
     * SortableTable Adapter module
     * @param {Object} tf TableFilter instance
     */
    constructor(tf, opts){
        this.initialized = false;
        this.name = opts.name;
        this.desc = opts.description || 'Sortable table';

        //indicates if paging is enabled
        this.isPaged = false;

        //indicates if tables was sorted
        this.sorted = false;

        this.sortTypes = Types.isArray(opts.types) ? opts.types : [];
        this.sortColAtStart = Types.isArray(opts.sort_col_at_start) ?
            opts.sort_col_at_start : null;
        this.asyncSort = Boolean(opts.async_sort);
        this.triggerIds = Types.isArray(opts.trigger_ids) ?
            opts.trigger_ids : [];

        // edit .sort-arrow.descending / .sort-arrow.ascending in
        // tablefilter.css to reflect any path change
        this.imgPath = opts.images_path || tf.themesPath;
        this.imgBlank = opts.image_blank || 'blank.png';
        this.imgClassName = opts.image_class_name || 'sort-arrow';
        this.imgAscClassName = opts.image_asc_class_name || 'ascending';
        this.imgDescClassName = opts.image_desc_class_name ||'descending';
        //cell attribute storing custom key
        this.customKey = opts.custom_key || 'data-tf-sortKey';

        /*** TF additional events ***/
        //additional paging events for alternating background
        // o.Evt._Paging.nextEvt = function(){
        // if(o.sorted && o.alternateRows) o.Filter();
        // }
        // o.Evt._Paging.prevEvt = o.Evt._Paging.nextEvt;
        // o.Evt._Paging.firstEvt = o.Evt._Paging.nextEvt;
        // o.Evt._Paging.lastEvt = o.Evt._Paging.nextEvt;
        // o.Evt._OnSlcPagesChangeEvt = o.Evt._Paging.nextEvt;

        // callback invoked after sort is loaded and instanciated
        this.onSortLoaded = Types.isFn(opts.on_sort_loaded) ?
            opts.on_sort_loaded : null;
        // callback invoked before table is sorted
        this.onBeforeSort = Types.isFn(opts.on_before_sort) ?
            opts.on_before_sort : null;
        // callback invoked after table is sorted
        this.onAfterSort = Types.isFn(opts.on_after_sort) ?
            opts.on_after_sort : null;

        this.tf = tf;
    }

    init(){
        let tf = this.tf;
        let adpt = this;

        // SortableTable class sanity check (sortabletable.js)
        if(Types.isUndef(SortableTable)){
            throw new Error('SortableTable class not found.');
        }

        this.overrideSortableTable();
        this.setSortTypes();

        //Column sort at start
        let sortColAtStart = adpt.sortColAtStart;
        if(sortColAtStart){
            this.stt.sort(sortColAtStart[0], sortColAtStart[1]);
        }

        if(this.onSortLoaded){
            this.onSortLoaded.call(null, tf, this);
        }

        /*** SortableTable callbacks ***/
        this.stt.onbeforesort = function(){
            if(adpt.onBeforeSort){
                adpt.onBeforeSort.call(null, tf, adpt.stt.sortColumn);
            }

            /*** sort behaviour for paging ***/
            if(tf.paging){
                adpt.isPaged = true;
                tf.paging = false;
                tf.feature('paging').destroy();
            }
        };

        this.stt.onsort = function(){
            adpt.sorted = true;

            //rows alternating bg issue
            // TODO: move into AlternateRows component
            if(tf.alternateRows){
                let rows = tf.tbl.rows, c = 0;

                let setClass = function(row, i, removeOnly){
                    if(Types.isUndef(removeOnly)){
                        removeOnly = false;
                    }
                    let altRows = tf.feature('alternateRows'),
                        oddCls = altRows.oddCss,
                        evenCls = altRows.evenCss;
                    Dom.removeClass(row, oddCls);
                    Dom.removeClass(row, evenCls);

                    if(!removeOnly){
                        Dom.addClass(row, i % 2 ? oddCls : evenCls);
                    }
                };

                for (let i = tf.refRow; i < tf.nbRows; i++){
                    let isRowValid = rows[i].getAttribute('validRow');
                    if(tf.paging && rows[i].style.display === ''){
                        setClass(rows[i], c);
                        c++;
                    } else {
                        if((isRowValid==='true' || isRowValid===null) &&
                            rows[i].style.display === ''){
                            setClass(rows[i], c);
                            c++;
                        } else {
                            setClass(rows[i], c, true);
                        }
                    }
                }
            }
            //sort behaviour for paging
            if(adpt.isPaged){
                let paginator = tf.feature('paging');
                paginator.reset(false);
                paginator.setPage(paginator.getPage());
                adpt.isPaged = false;
            }

            if(adpt.onAfterSort){
                adpt.onAfterSort.call(null, tf, adpt.stt.sortColumn);
            }
        };

        this.initialized = true;
    }

    /**
     * Sort specified column
     * @param {Number} colIdx Column index
     * @param {Boolean} desc Optional: descending manner
     */
    sortByColumnIndex(colIdx, desc){
        this.stt.sort(colIdx, desc);
    }

    overrideSortableTable(){
        let adpt = this,
            tf = this.tf;

        /**
         * Overrides headerOnclick method in order to handle th event
         * @param  {Object} e [description]
         */
        SortableTable.prototype.headerOnclick = function(evt){
            if(!adpt.initialized){
                return;
            }

            // find Header element
            let el = evt.target || evt.srcElement;

            while(el.tagName !== 'TD' && el.tagName !== 'TH'){
                el = el.parentNode;
            }

            this.sort(
                SortableTable.msie ?
                    SortableTable.getCellIndex(el) : el.cellIndex
            );
        };

        /**
         * Overrides getCellIndex IE returns wrong cellIndex when columns are
         * hidden
         * @param  {Object} oTd TD element
         * @return {Number}     Cell index
         */
        SortableTable.getCellIndex = function(oTd){
            let cells = oTd.parentNode.cells,
                l = cells.length, i;
            for (i = 0; cells[i] != oTd && i < l; i++){}
            return i;
        };

        /**
         * Overrides initHeader in order to handle filters row position
         * @param  {Array} oSortTypes
         */
        SortableTable.prototype.initHeader = function(oSortTypes){
            let stt = this;
            if (!stt.tHead){
                if(tf.gridLayout){
                    stt.tHead = tf.feature('gridLayout').headTbl.tHead;
                } else {
                    return;
                }
            }

            stt.headersRow = tf.headersRow;
            let cells = stt.tHead.rows[stt.headersRow].cells;
            stt.sortTypes = oSortTypes || [];
            let l = cells.length;
            let img, c;

            for (let i = 0; i < l; i++) {
                c = cells[i];
                if (stt.sortTypes[i] !== null && stt.sortTypes[i] !== 'None'){
                    c.style.cursor = 'pointer';
                    img = Dom.create('img',
                        ['src', adpt.imgPath + adpt.imgBlank]);
                    c.appendChild(img);
                    if (stt.sortTypes[i] !== null){
                        c.setAttribute( '_sortType', stt.sortTypes[i]);
                    }
                    Event.add(c, 'click', stt._headerOnclick);
                } else {
                    c.setAttribute('_sortType', oSortTypes[i]);
                    c._sortType = 'None';
                }
            }
            stt.updateHeaderArrows();
        };

        /**
         * Overrides updateHeaderArrows in order to handle arrows indicators
         */
        SortableTable.prototype.updateHeaderArrows = function(){
            let stt = this;
            let cells, l, img;

            // external headers
            if(adpt.asyncSort && adpt.triggerIds.length > 0){
                let triggers = adpt.triggerIds;
                cells = [];
                l = triggers.length;
                for(let j=0; j<triggers.length; j++){
                    cells.push(Dom.id(triggers[j]));
                }
            } else {
                if(!this.tHead){
                    return;
                }
                cells = stt.tHead.rows[stt.headersRow].cells;
                l = cells.length;
            }
            for(let i = 0; i < l; i++){
                let cellAttr = cells[i].getAttribute('_sortType');
                if(cellAttr !== null && cellAttr !== 'None'){
                    img = cells[i].lastChild || cells[i];
                    if(img.nodeName.toLowerCase() !== 'img'){
                        img = Dom.create('img',
                            ['src', adpt.imgPath + adpt.imgBlank]);
                        cells[i].appendChild(img);
                    }
                    if (i === stt.sortColumn){
                        img.className = adpt.imgClassName +' '+
                            (this.descending ?
                                adpt.imgDescClassName :
                                adpt.imgAscClassName);
                    } else{
                        img.className = adpt.imgClassName;
                    }
                }
            }
        };

        /**
         * Overrides getRowValue for custom key value feature
         * @param  {Object} oRow    Row element
         * @param  {String} sType
         * @param  {Number} nColumn
         * @return {String}
         */
        SortableTable.prototype.getRowValue = function(oRow, sType, nColumn){
            let stt = this;
            // if we have defined a custom getRowValue use that
            let sortTypeInfo = stt._sortTypeInfo[sType];
            if (sortTypeInfo && sortTypeInfo.getRowValue){
                return sortTypeInfo.getRowValue(oRow, nColumn);
            }
            let c = oRow.cells[nColumn];
            let s = SortableTable.getInnerText(c);
            return stt.getValueFromString(s, sType);
        };

        /**
         * Overrides getInnerText in order to avoid Firefox unexpected sorting
         * behaviour with untrimmed text elements
         * @param  {Object} oNode DOM element
         * @return {String}       DOM element inner text
         */
        SortableTable.getInnerText = function(oNode){
            if(!oNode){
                return;
            }
            if(oNode.getAttribute(adpt.customKey)){
                return oNode.getAttribute(adpt.customKey);
            } else {
                return Dom.getText(oNode);
            }
        };
    }

    addSortType(){
        var args = arguments;
        SortableTable.prototype.addSortType(args[0], args[1], args[2], args[3]);
    }

    setSortTypes(){
        let tf = this.tf,
            sortTypes = this.sortTypes,
            _sortTypes = [];

        for(let i=0; i<tf.nbCells; i++){
            let colType;

            if(sortTypes[i]){
                colType = sortTypes[i].toLowerCase();
                if(colType === 'none'){
                    colType = 'None';
                }
            } else { // resolve column types
                if(tf.hasColNbFormat && tf.colNbFormat[i] !== null){
                    colType = tf.colNbFormat[i].toLowerCase();
                } else if(tf.hasColDateType && tf.colDateType[i] !== null){
                    colType = tf.colDateType[i].toLowerCase()+'date';
                } else {
                    colType = 'String';
                }
            }
            _sortTypes.push(colType);
        }

        //Public TF method to add sort type

        //Custom sort types
        this.addSortType('number', Number);
        this.addSortType('caseinsensitivestring', SortableTable.toUpperCase);
        this.addSortType('date', SortableTable.toDate);
        this.addSortType('string');
        this.addSortType('us', usNumberConverter);
        this.addSortType('eu', euNumberConverter);
        this.addSortType('dmydate', dmyDateConverter );
        this.addSortType('ymddate', ymdDateConverter);
        this.addSortType('mdydate', mdyDateConverter);
        this.addSortType('ddmmmyyyydate', ddmmmyyyyDateConverter);
        this.addSortType('ipaddress', ipAddress, sortIP);

        this.stt = new SortableTable(tf.tbl, _sortTypes);

        /*** external table headers adapter ***/
        if(this.asyncSort && this.triggerIds.length > 0){
            let triggers = this.triggerIds;
            for(let j=0; j<triggers.length; j++){
                if(triggers[j] === null){
                    continue;
                }
                let trigger = Dom.id(triggers[j]);
                if(trigger){
                    trigger.style.cursor = 'pointer';

                    Event.add(trigger, 'click', (evt) => {
                        let elm = evt.target;
                        if(!this.tf.sort){
                            return;
                        }
                        this.stt.asyncSort(triggers.indexOf(elm.id));
                    });
                    trigger.setAttribute('_sortType', _sortTypes[j]);
                }
            }
        }
    }

    /**
     * Destroy sort
     */
    destroy(){
        let tf = this.tf;
        this.sorted = false;
        this.initialized = false;
        this.stt.destroy();

        let ids = tf.getFiltersId();
        for (let idx = 0; idx < ids.length; idx++){
            let header = tf.getHeaderElement(idx);
            let img = Dom.tag(header, 'img');

            if(img.length === 1){
                header.removeChild(img[0]);
            }
        }
    }

}

//Converters
function usNumberConverter(s){
    return Helpers.removeNbFormat(s, 'us');
}
function euNumberConverter(s){
    return Helpers.removeNbFormat(s, 'eu');
}
function dateConverter(s, format){
    return DateHelper.format(s, format);
}
function dmyDateConverter(s){
    return dateConverter(s, 'DMY');
}
function mdyDateConverter(s){
    return dateConverter(s, 'MDY');
}
function ymdDateConverter(s){
    return dateConverter(s, 'YMD');
}
function ddmmmyyyyDateConverter(s){
    return dateConverter(s, 'DDMMMYYYY');
}

function ipAddress(value){
    let vals = value.split('.');
    for (let x in vals) {
        let val = vals[x];
        while (3 > val.length){
            val = '0'+val;
        }
        vals[x] = val;
    }
    return vals.join('.');
}

function sortIP(a,b){
    let aa = ipAddress(a.value.toLowerCase());
    let bb = ipAddress(b.value.toLowerCase());
    if (aa==bb){
        return 0;
    } else if (aa<bb){
        return -1;
    } else {
        return 1;
    }
}
