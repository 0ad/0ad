import Dom from '../dom';
import Str from '../string';

export class HighlightKeyword{

    /**
     * HighlightKeyword, highlight matched keyword
     * @param {Object} tf TableFilter instance
     */
    constructor(tf) {
        var f = tf.config();
        //defines css class for highlighting
        this.highlightCssClass = f.highlight_css_class || 'keyword';
        this.highlightedNodes = [];

        this.tf = tf;
    }

    /**
     * highlight occurences of searched term in passed node
     * @param  {Node} node
     * @param  {String} word     Searched term
     * @param  {String} cssClass Css class name
     */
    highlight(node, word, cssClass){
        // Iterate into this nodes childNodes
        if(node.hasChildNodes){
            var children = node.childNodes;
            for(var i=0; i<children.length; i++){
                this.highlight(children[i], word, cssClass);
            }
        }

        if(node.nodeType === 3){
            var tempNodeVal = Str.lower(node.nodeValue);
            var tempWordVal = Str.lower(word);
            if(tempNodeVal.indexOf(tempWordVal) != -1){
                var pn = node.parentNode;
                if(pn && pn.className != cssClass){
                    // word not highlighted yet
                    var nv = node.nodeValue,
                        ni = tempNodeVal.indexOf(tempWordVal),
                        // Create a load of replacement nodes
                        before = Dom.text(nv.substr(0, ni)),
                        docWordVal = nv.substr(ni,word.length),
                        after = Dom.text(nv.substr(ni+word.length)),
                        hiwordtext = Dom.text(docWordVal),
                        hiword = Dom.create('span');
                    hiword.className = cssClass;
                    hiword.appendChild(hiwordtext);
                    pn.insertBefore(before,node);
                    pn.insertBefore(hiword,node);
                    pn.insertBefore(after,node);
                    pn.removeChild(node);
                    this.highlightedNodes.push(hiword.firstChild);
                }
            }
        }
    }

    /**
     * Removes highlight to nodes matching passed string
     * @param  {String} word
     * @param  {String} cssClass Css class to remove
     */
    unhighlight(word, cssClass){
        var arrRemove = [];
        var highlightedNodes = this.highlightedNodes;
        for(var i=0; i<highlightedNodes.length; i++){
            var n = highlightedNodes[i];
            if(!n){
                continue;
            }
            var tempNodeVal = Str.lower(n.nodeValue),
                tempWordVal = Str.lower(word);
            if(tempNodeVal.indexOf(tempWordVal) !== -1){
                var pn = n.parentNode;
                if(pn && pn.className === cssClass){
                    var prevSib = pn.previousSibling,
                        nextSib = pn.nextSibling;
                    if(!prevSib || !nextSib){ continue; }
                    nextSib.nodeValue = prevSib.nodeValue + n.nodeValue +
                        nextSib.nodeValue;
                    prevSib.nodeValue = '';
                    n.nodeValue = '';
                    arrRemove.push(i);
                }
            }
        }
        for(var k=0; k<arrRemove.length; k++){
            highlightedNodes.splice(arrRemove[k], 1);
        }
    }

    /**
     * Clear all occurrences of highlighted nodes
     */
    unhighlightAll(){
        if(!this.tf.highlightKeywords || !this.tf.searchArgs){
            return;
        }
        for(var y=0; y<this.tf.searchArgs.length; y++){
            this.unhighlight(
                this.tf.searchArgs[y], this.highlightCssClass);
        }
        this.highlightedNodes = [];
    }
}