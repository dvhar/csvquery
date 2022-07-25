function getidx(arr, comp){
	for (var i in arr){
			if (comp(arr[i]))
				return i;
		}
	return 0;
}

function fileclick(clicked){
	let querybox = document.querySelector('#queryTextEntry');
	let browser = clicked.closest('.fileBrowser');
	let start = querybox.value.substring(0,querybox.selectionStart);
	let end = querybox.value.substring(querybox.selectionEnd, 10000000);
	querybox.value = start +" '"+ clicked.innerText +"' "+ end;
	browser.classList.add('hidden');
}

function dirclick(clicked){
	// fetch and return data, then:
	let ret = { //sample return payload
		path: '/home/user/somedir/',
		parent: '/home/user/',
		files: ['/home/user/somedir/file2.csv','/home/user/somedir/file3.csv'],
		dirs: ['/home/user/somedir/dir2/','/home/user/somedir/otherdir/']
	};
	let browser = clicked.closest('.fileBrowser');
	let textinput = browser.querySelector('.pathinput');
	let filelist = clicked.closest('.filelist');
	let updir = filelist.children[0];
	textinput.value = ret.path;
	filelist.innerText = null;
	const makespan = (path,type) => {
		let span = document.createElement('span');
		span.classList.add(type, 'dropdown');
		span.innerText = path;
		if (type === 'browseDir')
			span.onclick = ()=>dirclick(span);
		else if (browser.id === 'openDropdown')
			span.ondblclick = ()=>fileclick(span);
		filelist.appendChild(span);
	};
	filelist.appendChild(updir);
	ret.dirs.forEach(path => makespan(path, 'browseDir'));
	ret.files.forEach(path => makespan(path, 'browsefile'));
}

function restoretable(opt){
	let res = opt.closest('.singleResult');
	let tbody = res.querySelector('tbody');
	tbody.innerText = null;
	res.fulldata.rows.forEach(row => tbody.appendChild(row));
}

function filtertable(opt){
	let res = opt.closest('.singleResult');
	let idx = getidx(Array.from(res.querySelector('.filtername').children), (o)=>o.selected);
	let tbody = res.querySelector('tbody');
	tbody.innerText = null;
	res.fulldata.rows.forEach(row => {
		if (row.children[idx].textContent == opt.textContent)
			tbody.appendChild(row);
	});
}

function populatefilter(opt){
	let res = opt.closest('.singleResult');
	let idx = getidx(res.fulldata.cols, c=>{return c.innerText == opt.innerText});
	let filtervalues = res.querySelector('.filtervalue');
	filtervalues.textContent = null;
	filtervalues.size = res.fulldata.rows.length+1;
	let newopt = document.createElement('option');
	newopt.textContent = '*';
	newopt.onclick = ()=>restoretable(newopt);
	filtervalues.appendChild(newopt);
	// todo: sorted unique values
	res.fulldata.rows.forEach(r => {
		let newopt = document.createElement('option');
		newopt.textContent = r.children[idx].textContent;
		newopt.onclick = ()=>filtertable(newopt);
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
	tbodydiv.addEventListener('scroll',function(){ theaddiv.scrollLeft = tbodydiv.scrollLeft; });
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
	let querytextEntry = document.querySelector('#queryTextEntry');
	let histNumber = document.querySelector('#histNumber');
	if (direction === 1 && currentHist < historyList.length){
		currentHist++;
		querytextEntry.value = historyList[currentHist-1];
		histNumber.innerText = currentHist;
	}
	if (direction === -1 && currentHist > 1){
		if (currentHist === historyList.length)
			historyList[currentHist-1] = querytextEntry.value;
		currentHist--;
		querytextEntry.value = historyList[currentHist-1];
		histNumber.innerText = currentHist;
	}
}

document.querySelectorAll('.singleResult').forEach(res => postProcess(res));
document.addEventListener('click',e=>{
	if (e.target.classList.contains('dropbutton'))
		return;
	let dropdown = e.target.closest('.dropdown');
	if (dropdown == null){
		document.querySelectorAll('.dropdown').forEach(dd => dd.classList.add('hidden'));
	}
});
