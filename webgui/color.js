var SQLHighlighter = (function() {
	var keywords = 'and,as,by,cross,distinct,end,from,group,having,inner,in,is,join,left,limit,not,null,on,order,or,outer,select,the,then,when,where,with,xor'
	var kwSet = {};
	keywords.split(',').forEach(function(k){ kwSet[k] = true; });

	var functions = 'abs,acos,asin,atan,avg,base64_decode,base64_encode,between,cbrt,ceil,count,date,day,dayname,dayofmonth,dayofweek,dayofyear,decrypt,else,end,exp,floor,float,format,from,group,having,hour,in,inc,int,is,len,like,limit,log,log10,log2,max,min,minute,mod,month,monthname,not,null,order,pow,rand,round,second,select,sin,sip,sqrt,sha1,sha256,stdev,stdevp,string,sum,substr,tan,text,to,top,then,when,where,xor'
	var fnSet = {};
	functions.split(',').forEach(function(f){ fnSet[f] = true; });

	// Also match 'top' as a keyword (used before the number in select top N)
	function escapeHtml(str) {
		return str.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
	}

	function tokenize(query) {
		var tokens = [];
		var i = 0;
		var len = query.length;

		while (i < len) {
			// Single-line comment: --
			if (query[i] === '-' && i+1 < len && query[i+1] === '-') {
				var end = query.indexOf('\n', i);
				if (end === -1) end = len;
				tokens.push({type: 'comment', value: query.substring(i, end)});
				i = end;
				continue;
			}
			// Block comment: /* */
			if (query[i] === '/' && i+1 < len && query[i+1] === '*') {
				var end2 = query.indexOf('*/', i+2);
				if (end2 === -1) end2 = len;
				else end2 += 2;
				tokens.push({type: 'comment', value: query.substring(i, end2)});
				i = end2;
				continue;
			}
			// String (single quote)
			if (query[i] === "'") {
				var j = i + 1;
				while (j < len && query[j] !== "'") {
					if (query[j] === '\\') j++; // skip escaped char
					j++;
				}
				if (j < len) j++; // closing quote
				tokens.push({type: 'string', value: query.substring(i, j)});
				i = j;
				continue;
			}
			// String (double quote)
			if (query[i] === '"') {
				var j2 = i + 1;
				while (j2 < len && query[j2] !== '"') { j2++; }
				if (j2 < len) j2++;
				tokens.push({type: 'string', value: query.substring(i, j2)});
				i = j2;
				continue;
			}
			// Number
			if (/[0-9]/.test(query[i])) {
				var j3 = i;
				while (j3 < len && /[0-9.eE+-]/.test(query[j3])) j3++;
				tokens.push({type: 'number', value: query.substring(i, j3)});
				i = j3;
				continue;
			}
			// Operators (multi-char first)
			if ('<>!='.indexOf(query[i]) !== -1) {
				var op2 = query.substring(i, i+2);
				if ('<>=!'.indexOf(op2[0]) !== -1 && '<>='.indexOf(op2[1]) !== -1) {
					tokens.push({type: 'operator', value: op2});
					i += 2;
				} else {
					tokens.push({type: 'operator', value: query[i]});
					i++;
				}
				continue;
			}
			// Single-char operators
			if ('=<>+-*/%^(),'.indexOf(query[i]) !== -1) {
				tokens.push({type: 'operator', value: query[i]});
				i++;
				continue;
			}
			// Parentheses
			if ('()'.indexOf(query[i]) !== -1) {
				tokens.push({type: 'parens', value: query[i]});
				i++;
				continue;
			}
			// File reference (.csv/.tsv filenames)
			if (/[a-zA-Z0-9_.\/*\-]/.test(query[i])) {
				var j6 = i;
				while (j6 < len && /[a-zA-Z0-9_.\/*\-]/.test(query[j6])) j6++;
				var maybeFile = query.substring(i, j6);
				if (/^[a-zA-Z0-9_.\/\*\-]+\.csv$/i.test(maybeFile) || /^[a-zA-Z0-9_.\/\*\-]+\.tsv$/i.test(maybeFile)) {
					tokens.push({type: 'file', value: maybeFile});
					i = j6;
					continue;
				}
			}
			// Word (identifier/keyword/function)
			if (/[a-zA-Z_]/.test(query[i])) {
				var j4 = i;
				while (j4 < len && /[a-zA-Z0-9_.]/.test(query[j4])) j4++;
				var word = query.substring(i, j4);
				var wordLower = word.toLowerCase();
				var tType = 'identifier';
				if (kwSet[wordLower]) {
					tType = 'keyword';
				} else if (fnSet[wordLower]) {
					tType = 'function';
				}
				tokens.push({type: tType, value: word});
				i = j4;
				continue;
			}
			// Whitespace
			if (/\s/.test(query[i])) {
				var j5 = i;
				while (j5 < len && /\s/.test(query[j5])) j5++;
				tokens.push({type: 'whitespace', value: query.substring(i, j5)});
				i = j5;
				continue;
			}
			// Everything else (dots, brackets, etc)
			tokens.push({type: 'other', value: query[i]});
			i++;
		}
		return tokens;
	}

	function highlight(query) {
		if (!query) return '';
		var tokens = tokenize(query);
		var html = '';
		for (var t = 0; t < tokens.length; t++) {
			var tok = tokens[t];
			var esc = escapeHtml(tok.value);
			if (tok.type === 'keyword') {
				html += '<span class="sqhl-keyword">' + esc + '</span>';
			} else if (tok.type === 'function') {
				html += '<span class="sqhl-function">' + esc + '</span>';
			} else if (tok.type === 'string') {
				html += '<span class="sqhl-string">' + esc + '</span>';
			} else if (tok.type === 'number') {
				html += '<span class="sqhl-number">' + esc + '</span>';
			} else if (tok.type === 'operator') {
				html += '<span class="sqhl-operator">' + esc + '</span>';
			} else if (tok.type === 'parens') {
				html += '<span class="sqhl-parens">' + esc + '</span>';
			} else if (tok.type === 'comment') {
				html += '<span class="sqhl-comment">' + esc + '</span>';
			} else if (tok.type === 'file') {
				html += '<span class="sqhl-file">' + esc + '</span>';
			} else if (tok.type === 'whitespace') {
				html += esc.replace(/ /g,'&nbsp;').replace(/\n/g,'<br>');
			} else {
				html += esc;
			}
		}
		return html + '\n'; // trailing newline for proper scrolling
	}

	return { highlight: highlight };
})();
