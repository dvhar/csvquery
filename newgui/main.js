function fileclick(clicked){
	let querybox = document.querySelector('#queryTextEntry');
	let browser = clicked.closest('.fileBrowser');
	let start = querybox.value.substring(0,querybox.selectionStart);
	let end = querybox.value.substring(querybox.selectionEnd, 10000000);
	querybox.value = start +" '"+ clicked.innerText +"' "+ end;
	browser.classList.add('hidden');
}

function populateDirs(clicked, ret){
	console.log(clicked.content);
	if (ret){
		let browser = clicked.closest('.fileBrowser');
		let textinput = browser.querySelector('.pathinput');
		let filelist = clicked.closest('.filelist');
		textinput.value = ret.path;
		filelist.innerText = null;
		const makespan = (path,type,updir=false) => {
			let span = document.createElement('span');
			span.classList.add(type, 'dropdown');
			if (updir) {
				span.classList.add(type, 'updir');
				span.innerHTML = '&#8592';
				span.clickData = path;
			} else {
				span.innerText = path;
				span.clickData = path;
			}
			if (type === 'browseDir')
				span.onclick = ()=>dirclick(span);
			else if (browser.id === 'openDropdown')
				span.ondblclick = ()=>fileclick(span);
			filelist.appendChild(span);
		};
		makespan(ret.parent, 'browseDir', true);
		ret.dirs.forEach(path => makespan(path, 'browseDir'));
		ret.files.forEach(path => makespan(path, 'browsefile'));
	}
}
function dirclick(clicked, both=false){

	let payload = {
		sessionId: "12345",
		path: clicked.clickData,
		mode: 'open',
	};
	var req = new Request("/info?info=fileClick", {
		method: 'POST',
		cache: "no-cache",
		redirect: 'follow',
		referrer: "no-referrer",
		headers: new Headers({ "Content-Type": "application/json", }),
		body: JSON.stringify(payload),
	});
	fetch(req).then(res=>{
		if (res.status >= 400){
			return false;
		} else {
			return res.json();
		}
	}).then(ret=>{
		console.log(ret);
		if (both){
			document.querySelectorAll('.startdir').forEach(fl=>populateDirs(fl, ret));
		} else {
			populateDirs(clicked, ret);
		}
	});
}

function restoretable(opt){
	let res = opt.closest('.singleResult');
	let tbody = res.querySelector('tbody');
	tbody.innerText = null;
	res.fulldata.rows.forEach(row => tbody.appendChild(row));
}

function filtertable(opt, idx){
	let res = opt.closest('.singleResult');
	let tbody = res.querySelector('tbody');
	tbody.innerText = null;
	res.fulldata.rows.forEach(row => {
		if (row.children[idx].textContent == opt.textContent)
			tbody.appendChild(row);
	});
	resize(res); //TODO: bug wien only 1 row
}

function populatefilter(opt, idx){
	let res = opt.closest('.singleResult');
	let filtervalues = res.querySelector('.filtervalue');
	let uniques = new Set();
	let newopt = document.createElement('option');
	res.fulldata.rows.forEach(r => uniques.add(r.children[idx].textContent));
	filtervalues.textContent = null;
	filtervalues.size = uniques.size+1;
	newopt.textContent = '*';
	newopt.onclick = ()=>restoretable(newopt);
	filtervalues.appendChild(newopt);
	Array.from(uniques).sort().forEach(u => {
		let newopt = document.createElement('option');
		newopt.textContent = u;
		newopt.onclick = ()=>filtertable(newopt,idx);
		filtervalues.appendChild(newopt);
	});
}

function toggleColumn(opt,idx){
	let res = opt.closest('.singleResult');
	res.fulldata.cols[idx].classList.toggle('hidden');
	res.fulldata.types[idx].classList.toggle('hidden');
	res.fulldata.rows.forEach(row => row.childNodes[idx].classList.toggle('hidden'));
	opt.classList.toggle('hiddenColumn');
	resize(res);
}

function sorttable(e){
	let col = e.target;
	let idx = col.idx;
	const getCellValue = (tr) => tr.children[idx].innerText || tr.children[idx].textContent;
	const comparer = (asc) => (a, b) => ((v1, v2) => 
		v1 !== '' && v2 !== '' && !isNaN(v1) && !isNaN(v2) ? v1 - v2 : v1.toString().localeCompare(v2)
		)(getCellValue(asc ? a : b, idx), getCellValue(asc ? b : a, idx));
	let tbody = col.closest('.singleResult').querySelector('tbody');
	Array.from(tbody.querySelectorAll('tr'))
		.sort(comparer(this.asc = !this.asc))
		.forEach(tr => tbody.appendChild(tr) );
}

function resize(res){
	let tbody = res.querySelector('tbody');
	let thead = res.querySelector('thead');
	let theaddiv = thead.closest('.resultHeadDiv');
	let tbodydiv = tbody.closest('.resultBodyDiv');
	//get header table and body table cells to line up and scroll when needed
	if (tbody.childNodes.length > 1 && tbody.childNodes[1].childNodes.length > 0){
		let trow = tbody.childNodes[1].childNodes
		for (let i in trow){
			let bcell = trow[i];
			let hcell = thead.childNodes[1].childNodes[i];
			if (bcell.offsetWidth && hcell.offsetWidth){
				let newWidth = Math.max(bcell.offsetWidth, hcell.offsetWidth);
				bcell.style.minWidth = hcell.style.minWidth = `${newWidth+1}px`;
			}
		}
	}
	let num = (tbody.offsetHeight <= tbodydiv.offsetHeight) ? 4:15;
	theaddiv.style.maxWidth = tbodydiv.style.maxWidth =  `${Math.min(tbody.offsetWidth+num,window.innerWidth)}px`;
}

function postProcess(res){
	resize(res);
	let tbody = res.querySelector('tbody');
	let thead = res.querySelector('thead');
	let theaddiv = thead.closest('.resultHeadDiv');
	let tbodydiv = tbody.closest('.resultBodyDiv');
	tbodydiv.addEventListener('scroll',()=>theaddiv.scrollLeft = tbodydiv.scrollLeft);
	thead.addEventListener('click',sorttable);
	res.fulldata = {
		rows: Array.from(tbody.children),
		cols: Array.from(thead.querySelector('.colnames').children),
		types: Array.from(thead.querySelector('.coltypes').children)
	};
	res.fulldata.types.forEach((td,i) => td.idx = i);
	res.fulldata.cols.forEach((tr,i) => tr.idx = i);
	// initialize menus on backend?
}

function showTopDrop(but){
	const butmap = {
		saveBut: 'saveDropdown',
		openBut: 'openDropdown',
		helpBut: 'helpDropdown'
	};
	document.querySelectorAll('.dropdown').forEach(dd => {
		if (dd.id === butmap[but.id]) {
			dd.classList.remove('hidden');
		} else {
			dd.classList.add('hidden');
		}
	});
}

var historyList = ['select * from previousfile.csv',''];
var currentHist = historyList.length;
function historyClick(direction){
	let queryTextEntry = document.querySelector('#queryTextEntry');
	let histNumber = document.querySelector('#histNumber');
	if (direction === 1 && currentHist < historyList.length){
		currentHist++;
		queryTextEntry.value = historyList[currentHist-1];
		histNumber.innerText = currentHist;
	}
	if (direction === -1 && currentHist > 1){
		if (currentHist === historyList.length)
			historyList[currentHist-1] = queryTextEntry.value;
		currentHist--;
		queryTextEntry.value = historyList[currentHist-1];
		histNumber.innerText = currentHist;
	}
}

dirclick({clickData:'/home/d/gits/csvquery/build'}, true);
document.addEventListener('click',e=>{ //TODO: more efficient click handling
	if (e.target.classList.contains('dropbutton'))
		return;
	let dropdown = e.target.closest('.dropdown');
	if (dropdown == null){
		document.querySelectorAll('.dropdown').forEach(dd => dd.classList.add('hidden'));
	}
});

function submitQuery(){
	let query = document.querySelector('#queryTextEntry').value;
	if (query === ''){
		console.log("no query text");
		alert("no query!");
		return
	}
	console.log('query:',query);
	let payload = {
		sessionId: "12345",
		query,
		fileIO: 0, 
		savePath: "", 
	};
	var req = new Request("/query/", {
		method: 'POST',
		mode: 'cors',
		cache: "no-cache",
		redirect: 'follow',
		referrer: "no-referrer",
		headers: new Headers({ "Content-Type": "application/json", }),
		body: JSON.stringify(payload),
	});
	fetch(req).then(
		res=>res.status >= 400 ? false : res.text()
	).then(dat=>{
		if (dat) {
			document.querySelector('#results').innerHTML = dat;
			document.querySelectorAll('.singleResult').forEach(res => postProcess(res));
		}
	});
}
