import {Feature} from './feature';
import Dom from '../dom';
import Types from '../types';

export class RowsCounter extends Feature{

    /**
     * Rows counter
     * @param {Object} tf TableFilter instance
     */
    constructor(tf){
        super(tf, 'rowsCounter');

        // TableFilter configuration
        var f = this.config;

        //id of custom container element
        this.rowsCounterTgtId = f.rows_counter_target_id || null;
        //element containing tot nb rows
        this.rowsCounterDiv = null;
        //element containing tot nb rows label
        this.rowsCounterSpan = null;
        //defines rows counter text
        this.rowsCounterText = f.rows_counter_text || 'Rows: ';
        this.fromToTextSeparator = f.from_to_text_separator || '-';
        this.overText = f.over_text || ' / ';
        //defines css class rows counter
        this.totRowsCssClass = f.tot_rows_css_class || 'tot';
        //rows counter div
        this.prfxCounter = 'counter_';
        //nb displayed rows label
        this.prfxTotRows = 'totrows_span_';
        //label preceding nb rows label
        this.prfxTotRowsTxt = 'totRowsTextSpan_';
        //callback raised before counter is refreshed
        this.onBeforeRefreshCounter = Types.isFn(f.on_before_refresh_counter) ?
            f.on_before_refresh_counter : null;
        //callback raised after counter is refreshed
        this.onAfterRefreshCounter = Types.isFn(f.on_after_refresh_counter) ?
            f.on_after_refresh_counter : null;
    }

    init(){
        if(this.initialized){
            return;
        }

        var tf = this.tf;

        //rows counter container
        var countDiv = Dom.create('div', ['id', this.prfxCounter+tf.id]);
        countDiv.className = this.totRowsCssClass;
        //rows counter label
        var countSpan = Dom.create('span', ['id', this.prfxTotRows+tf.id]);
        var countText = Dom.create('span', ['id', this.prfxTotRowsTxt+tf.id]);
        countText.appendChild(Dom.text(this.rowsCounterText));

        // counter is added to defined element
        if(!this.rowsCounterTgtId){
            tf.setToolbar();
        }
        var targetEl = !this.rowsCounterTgtId ?
                tf.lDiv : Dom.id( this.rowsCounterTgtId );

        //default container: 'lDiv'
        if(!this.rowsCounterTgtId){
            countDiv.appendChild(countText);
            countDiv.appendChild(countSpan);
            targetEl.appendChild(countDiv);
        }
        else{
            //custom container, no need to append statusDiv
            targetEl.appendChild(countText);
            targetEl.appendChild(countSpan);
        }
        this.rowsCounterDiv = countDiv;
        this.rowsCounterSpan = countSpan;

        this.initialized = true;
        this.refresh();
    }

    refresh(p){
        if(!this.rowsCounterSpan){
            return;
        }

        var tf = this.tf;

        if(this.onBeforeRefreshCounter){
            this.onBeforeRefreshCounter.call(null, tf, this.rowsCounterSpan);
        }

        var totTxt;
        if(!tf.paging){
            if(p && p !== ''){
                totTxt = p;
            } else{
                totTxt = tf.nbFilterableRows - tf.nbHiddenRows;
            }
        } else {
            var paging = tf.feature('paging');
            if(paging){
                //paging start row
                var paging_start_row = parseInt(paging.startPagingRow, 10) +
                        ((tf.nbVisibleRows>0) ? 1 : 0);
                var paging_end_row = (paging_start_row+paging.pagingLength)-1 <=
                        tf.nbVisibleRows ?
                        paging_start_row+paging.pagingLength-1 :
                        tf.nbVisibleRows;
                totTxt = paging_start_row + this.fromToTextSeparator +
                    paging_end_row + this.overText + tf.nbVisibleRows;
            }
        }

        this.rowsCounterSpan.innerHTML = totTxt;
        if(this.onAfterRefreshCounter){
            this.onAfterRefreshCounter.call(
                null, tf, this.rowsCounterSpan, totTxt);
        }
    }

    destroy(){
        if(!this.initialized){
            return;
        }

        if(!this.rowsCounterTgtId && this.rowsCounterDiv){
            this.rowsCounterDiv.parentNode.removeChild(this.rowsCounterDiv);
        } else {
            Dom.id(this.rowsCounterTgtId).innerHTML = '';
        }
        this.rowsCounterSpan = null;
        this.rowsCounterDiv = null;

        this.disable();
        this.initialized = false;
    }
}
