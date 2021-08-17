import Dom from '../dom';
import Arr from '../array';
import Str from '../string';
import Sort from '../sort';

export class Dropdown{

    /**
     * Dropdown UI component
     * @param {Object} tf TableFilter instance
     */
    constructor(tf){
        // Configuration object
        var f = tf.config();

        this.enableSlcResetFilter = f.enable_slc_reset_filter===false ?
            false : true;
        //defines empty option text
        this.nonEmptyText = f.non_empty_text || '(Non empty)';
        //sets select filling method: 'innerHTML' or 'createElement'
        this.slcFillingMethod = f.slc_filling_method || 'createElement';
        //IE only, tooltip text appearing on select before it is populated
        this.activateSlcTooltip =  f.activate_slc_tooltip ||
            'Click to activate';
        //tooltip text appearing on multiple select
        this.multipleSlcTooltip = f.multiple_slc_tooltip ||
            'Use Ctrl key for multiple selections';

        this.isCustom = null;
        this.opts = null;
        this.optsTxt = null;
        this.slcInnerHtml = null;

        this.tf = tf;
    }

    /**
     * Build drop-down filter UI asynchronously
     * @param  {Number}  colIndex   Column index
     * @param  {Boolean} isLinked Enable linked refresh behaviour
     * @param  {Boolean} isExternal Render in external container
     * @param  {String}  extSlcId   External container id
     */
    build(colIndex, isLinked, isExternal, extSlcId){
        var tf = this.tf;
        tf.EvtManager(
            tf.Evt.name.dropdown,
            {
                slcIndex: colIndex,
                slcRefreshed: isLinked,
                slcExternal: isExternal,
                slcId: extSlcId
            }
        );
    }

    /**
     * Build drop-down filter UI
     * @param  {Number}  colIndex    Column index
     * @param  {Boolean} isLinked Enable linked refresh behaviour
     * @param  {Boolean} isExternal  Render in external container
     * @param  {String}  extSlcId    External container id
     */
    _build(colIndex, isLinked=false, isExternal=false, extSlcId=null){
        var tf = this.tf;
        colIndex = parseInt(colIndex, 10);

        this.opts = [];
        this.optsTxt = [];
        this.slcInnerHtml = '';

        var slcId = tf.fltIds[colIndex];
        if((!Dom.id(slcId) && !isExternal) ||
            (!Dom.id(extSlcId) && isExternal)){
            return;
        }
        var slc = !isExternal ? Dom.id(slcId) : Dom.id(extSlcId),
            rows = tf.tbl.rows,
            matchCase = tf.matchCase;

        //custom select test
        this.isCustom = tf.isCustomOptions(colIndex);

        //custom selects text
        var activeFlt;
        if(isLinked && tf.activeFilterId){
            activeFlt = tf.activeFilterId.split('_')[0];
            activeFlt = activeFlt.split(tf.prfxFlt)[1];
        }

        /*** remember grid values ***/
        var fltsValues = [], fltArr = [];
        if(tf.rememberGridValues){
            fltsValues =
                tf.feature('store').getFilterValues(tf.fltsValuesCookie);
            if(fltsValues && !Str.isEmpty(fltsValues.toString())){
                if(this.isCustom){
                    fltArr.push(fltsValues[colIndex]);
                } else {
                    fltArr = fltsValues[colIndex].split(' '+tf.orOperator+' ');
                }
            }
        }

        var excludedOpts = null,
            filteredDataCol = null;
        if(isLinked && tf.disableExcludedOptions){
            excludedOpts = [];
            filteredDataCol = [];
        }

        for(var k=tf.refRow; k<tf.nbRows; k++){
            // always visible rows don't need to appear on selects as always
            // valid
            if(tf.hasVisibleRows && tf.visibleRows.indexOf(k) !== -1){
                continue;
            }

            var cell = rows[k].cells,
                nchilds = cell.length;

            // checks if row has exact cell #
            if(nchilds !== tf.nbCells || this.isCustom){
                continue;
            }

            // this loop retrieves cell data
            for(var j=0; j<nchilds; j++){
                // WTF: cyclomatic complexity hell
                if((colIndex===j &&
                    (!isLinked ||
                        (isLinked && tf.disableExcludedOptions))) ||
                    (colIndex==j && isLinked &&
                        ((rows[k].style.display === '' && !tf.paging) ||
                    (tf.paging && (!tf.validRowsIndex ||
                        (tf.validRowsIndex &&
                            tf.validRowsIndex.indexOf(k) != -1)) &&
                        ((activeFlt===undefined || activeFlt==colIndex)  ||
                            (activeFlt!=colIndex &&
                                tf.validRowsIndex.indexOf(k) != -1 ))) ))){
                    var cell_data = tf.getCellData(cell[j]),
                        //Vary Peter's patch
                        cell_string = Str.matchCase(cell_data, matchCase);

                    // checks if celldata is already in array
                    if(!Arr.has(this.opts, cell_string, matchCase)){
                        this.opts.push(cell_data);
                    }

                    if(isLinked && tf.disableExcludedOptions){
                        var filteredCol = filteredDataCol[j];
                        if(!filteredCol){
                            filteredCol = tf.getFilteredDataCol(j);
                        }
                        if(!Arr.has(filteredCol, cell_string, matchCase) &&
                            !Arr.has(
                                excludedOpts, cell_string, matchCase) &&
                            !this.isFirstLoad){
                            excludedOpts.push(cell_data);
                        }
                    }
                }//if colIndex==j
            }//for j
        }//for k

        //Retrieves custom values
        if(this.isCustom){
            var customValues = tf.getCustomOptions(colIndex);
            this.opts = customValues[0];
            this.optsTxt = customValues[1];
        }

        if(tf.sortSlc && !this.isCustom){
            if (!matchCase){
                this.opts.sort(Sort.ignoreCase);
                if(excludedOpts){
                    excludedOpts.sort(Sort.ignoreCase);
                }
            } else {
                this.opts.sort();
                if(excludedOpts){ excludedOpts.sort(); }
            }
        }

        //asc sort
        if(tf.sortNumAsc && tf.sortNumAsc.indexOf(colIndex) != -1){
            try{
                this.opts.sort( numSortAsc );
                if(excludedOpts){
                    excludedOpts.sort(numSortAsc);
                }
                if(this.isCustom){
                    this.optsTxt.sort(numSortAsc);
                }
            } catch(e) {
                this.opts.sort();
                if(excludedOpts){
                    excludedOpts.sort();
                }
                if(this.isCustom){
                    this.optsTxt.sort();
                }
            }//in case there are alphanumeric values
        }
        //desc sort
        if(tf.sortNumDesc && tf.sortNumDesc.indexOf(colIndex) != -1){
            try{
                this.opts.sort(numSortDesc);
                if(excludedOpts){
                    excludedOpts.sort(numSortDesc);
                }
                if(this.isCustom){
                    this.optsTxt.sort(numSortDesc);
                }
            } catch(e) {
                this.opts.sort();
                if(excludedOpts){
                    excludedOpts.sort();
                }
                if(this.isCustom){
                    this.optsTxt.sort();
                }
            }//in case there are alphanumeric values
        }

        //populates drop-down
        this.addOptions(
            colIndex, slc, isLinked, excludedOpts, fltsValues, fltArr);
    }

    /**
     * Add drop-down options
     * @param {Number} colIndex     Column index
     * @param {Object} slc          Select Dom element
     * @param {Boolean} isLinked    Enable linked refresh behaviour
     * @param {Array} excludedOpts  Array of excluded options
     * @param {Array} fltsValues    Collection of persisted filter values
     * @param {Array} fltArr        Collection of persisted filter values
     */
    addOptions(colIndex, slc, isLinked, excludedOpts, fltsValues, fltArr){
        var tf = this.tf,
            fillMethod = Str.lower(this.slcFillingMethod),
            slcValue = slc.value;

        slc.innerHTML = '';
        slc = this.addFirstOption(slc);

        for(var y=0; y<this.opts.length; y++){
            if(this.opts[y]===''){
                continue;
            }
            var val = this.opts[y]; //option value
            var lbl = this.isCustom ? this.optsTxt[y] : val; //option text
            var isDisabled = false;
            if(isLinked && tf.disableExcludedOptions &&
                Arr.has(
                    excludedOpts,
                    Str.matchCase(val, tf.matchCase),
                    tf.matchCase
                )){
                isDisabled = true;
            }

            if(fillMethod === 'innerhtml'){
                var slcAttr = '';
                if(tf.loadFltOnDemand && slcValue===this.opts[y]){
                    slcAttr = 'selected="selected"';
                }
                this.slcInnerHtml += '<option value="'+val+'" ' + slcAttr +
                    (isDisabled ? 'disabled="disabled"' : '')+ '>' +
                    lbl+'</option>';
            } else {
                var opt;
                //fill select on demand
                if(tf.loadFltOnDemand && slcValue===this.opts[y] &&
                    tf.getFilterType(colIndex) === tf.fltTypeSlc){
                    opt = Dom.createOpt(lbl, val, true);
                } else {
                    if(tf.getFilterType(colIndex) !== tf.fltTypeMulti){
                        opt = Dom.createOpt(
                            lbl,
                            val,
                            (fltsValues[colIndex]!==' ' &&
                                val===fltsValues[colIndex]) ? true : false
                        );
                    } else {
                        opt = Dom.createOpt(
                            lbl,
                            val,
                            (Arr.has(fltArr,
                                Str.matchCase(this.opts[y], tf.matchCase),
                                tf.matchCase) ||
                              fltArr.toString().indexOf(val)!== -1) ?
                                true : false
                        );
                    }
                }
                if(isDisabled){
                    opt.disabled = true;
                }
                slc.appendChild(opt);
            }
        }// for y

        if(fillMethod === 'innerhtml'){
            slc.innerHTML += this.slcInnerHtml;
        }
        slc.setAttribute('filled', '1');
    }

    /**
     * Add drop-down header option
     * @param {Object} slc Select DOM element
     */
    addFirstOption(slc){
        var tf = this.tf,
            fillMethod = Str.lower(this.slcFillingMethod);

        if(fillMethod === 'innerhtml'){
            this.slcInnerHtml += '<option value="">'+ tf.displayAllText +
                '</option>';
        }
        else {
            var opt0 = Dom.createOpt(
                (!this.enableSlcResetFilter ? '' : tf.displayAllText),'');
            if(!this.enableSlcResetFilter){
                opt0.style.display = 'none';
            }
            slc.appendChild(opt0);
            if(tf.enableEmptyOption){
                var opt1 = Dom.createOpt(tf.emptyText, tf.emOperator);
                slc.appendChild(opt1);
            }
            if(tf.enableNonEmptyOption){
                var opt2 = Dom.createOpt(tf.nonEmptyText, tf.nmOperator);
                slc.appendChild(opt2);
            }
        }
        return slc;
    }

}
