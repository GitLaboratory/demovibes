
var tabwords=new Array();
var tablastpos=0;
var tablastlen=0;
var tablastprefix='';
var tablastidx=-1;

function tabgetprefix(value,pos){
	var prefix='';
	for (var i=pos-1;i>=0 && value.substring(i,i+1)!=' ';i--){
		prefix=value.substring(i,i+1)+prefix;
	}
	return prefix;
}

function tabgetnextword(){
	if (tablastidx==-1){
		if (tabwords.length==0) {
			return '';
		}
		tablastidx=0;
	}else{
		tablastidx++;
		if (tablastidx==tabwords.length) {
			// no more words...
			var tmp=tablastprefix;
			tablastidx=-1;
			tablastprefix='';
			return tmp;
		}
	}
	return tabwords[tablastidx];
}

function tabreset() {
	tablastidx=-1;
	tablastprefix='';
}

function tabcomplete(obj,keycode) {
	if (keycode!=9) {
		tabreset();
		return true;
	}
	try {
		if (obj.selectionStart!=obj.selectionEnd) {
			return false;
		}

		var prefix='';
		if (tablastprefix==''){
			prefix=tabgetprefix(obj.value,obj.selectionStart);
		}else{
			prefix=tablastprefix;
		}
		if (prefix=='') {
			return false;
		}
				
		if (tablastidx!=-1){
			obj.value=obj.value.substring(0,tablastpos-tablastprefix.length)+tablastprefix+obj.value.substring(tablastpos+tablastlen-tablastprefix.length);
			obj.selectionStart=tablastpos;
			obj.selectionEnd=tablastpos;
		}else{
			var req=ajaxget('/demovibes/ajax/words/'+prefix+'/',null,false);
			if (req.status == 200) {
				tabwords=req.responseText.split(',');
				if (tabwords[0]=='') {
					tabwords=new Array(); // empty
				}
			}else{
				tabwords=new Array(); // empty
			}
			if (tabwords.length==0) {
				return false;
			}
		}
		
		var word=tabgetnextword();
		
		var pos=obj.selectionStart;
		obj.value=obj.value.substring(0,pos-prefix.length)+word+obj.value.substring(pos);
		obj.selectionStart=pos+word.length-prefix.length;
		obj.selectionEnd=pos+word.length-prefix.length;
		obj.focus();
		
		tablastpos=pos;
		tablastlen=word.length;
		tablastprefix=prefix;
		return false;
			
	}catch(e){
		
		return false;
	
	} // ignore
	return false;
}
