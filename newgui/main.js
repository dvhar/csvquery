function getidx(arr, comp){
	for (var i in arr){
			if (comp(arr[i]))
				return i;
		}
	return 0;
}

function fileclick(clicked, type){
	// data will be statically initialized
	let ret = { //sample return payload
		path: '/home/user/',
		parent: '/home/',
		mode: 'open',
		files: ['/home/user/file.csv'],
		dirs: ['/home/user/somedir/']
	};
	let browser = opt.closest('.fileBrowser');
}

function restoretable(opt){
	let res = opt.closest('.singleResult');
	let tbody = res.querySelector('tbody');
	tbody.innerText = '';
	res.fulldata.rows.forEach(row => tbody.appendChild(row));
}

function filtertable(opt){
	let res = opt.closest('.singleResult');
	let idx = getidx(Array.from(res.querySelector('.filtername').children), (o)=>o.selected);
	let tbody = res.querySelector('tbody');
	tbody.innerText = '';
	res.fulldata.rows.forEach(row => {
		if (row.children[idx].textContent == opt.textContent)
			tbody.appendChild(row);
	});
}

function populatefilter(opt){
	let res = opt.closest('.singleResult');
	let idx = getidx(res.fulldata.cols, c=>{return c.textContent == opt.textContent});
	let filtervalues = res.querySelector('.filtervalue');
	filtervalues.textContent = '';
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

function sorttable(th){
	const getCellValue = (tr, idx) => tr.children[idx].innerText || tr.children[idx].textContent;
	const comparer = (idx, asc) => (a, b) => ((v1, v2) => 
		v1 !== '' && v2 !== '' && !isNaN(v1) && !isNaN(v2) ? v1 - v2 : v1.toString().localeCompare(v2)
		)(getCellValue(asc ? a : b, idx), getCellValue(asc ? b : a, idx));
	let idx = Array.from(th.parentNode.children).indexOf(th);
	let tbody = th.closest('.singleResult').querySelector('tbody');
	Array.from(tbody.querySelectorAll('tr'))
		.sort(comparer(idx, this.asc = !this.asc))
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
	// make backup of orig data for filtering and restoring
	res.fulldata = {
		rows: Array.from(tbody.children),
		cols: Array.from(res.querySelector('.colnames').children)
	};
	// initialize menus statically on backend
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

document.querySelectorAll('.singleResult').forEach(res => postProcess(res));
document.addEventListener('click',e=>{
	if (e.target.classList.contains('dropbutton'))
		return;
	let dropdown = e.target.closest('.dropdown');
	if (dropdown == null){
		document.querySelectorAll('.dropdown').forEach(dd => dd.classList.add('hidden'));
	}
});
