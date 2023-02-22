function topMessage(msg){
	messageDiv.textContent = msg;
}

function fileclick(clicked){
	let querybox = document.querySelector('#queryTextEntry');
	let browser = clicked.closest('.fileBrowser');
	let start = querybox.value.substring(0,querybox.selectionStart);
	let end = querybox.value.substring(querybox.selectionEnd, 10000000);
	querybox.value = start +" '"+ clicked.innerText +"' "+ end;
	browser.classList.add('hidden');
}

function populateDirs(clicked, ret){
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

function dirclick(clicked){
	let payload = {
		path: clicked.clickData,
		mode: 'open',
	};
	var req = new Request("/info?info=fileClick", {
		method: 'POST',
		cache: "no-cache",
		headers: new Headers({ "Content-Type": "application/json", }),
		body: JSON.stringify(payload),
	});
	fetch(req).then(res=>
		res.status >= 400 ? false : res.json()
	).then(ret=>{
		if (!ret)
			return;
		populateDirs(clicked, ret);
	});
}

function getState(){
	var req = new Request("/info?info=getState", {
		method: 'GET',
		cache: "no-cache",
		headers: new Headers({ "Content-Type": "application/json", }),
	});
	fetch(req).then(res=>
		res.status >= 400 ? false : res.json()
	).then(ret=>{
		if (!ret) return;
		document.querySelectorAll('.startdir').forEach(fl=>populateDirs(fl, ret.startDirlist));
	});
}

function restoretable(opt){
	let res = opt.closest('.singleResult');
	let tbody = res.querySelector('tbody');
	tbody.innerText = null;
	res.fulldata.rows.forEach(row => tbody.appendChild(row));
	resizeX(res);
}

function filtertable(opt, idx){
	let res = opt.closest('.singleResult');
	let tbody = res.querySelector('tbody');
	tbody.innerText = null;
	res.fulldata.rows.forEach(row => {
		if (row.children[idx].textContent == opt.textContent)
			tbody.appendChild(row);
	});
	resizeX(res);
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
	resizeX(res);
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
		.sort(comparer(col.asc = !col.asc))
		.forEach(tr => tbody.appendChild(tr) );
}

function resizeX(res){ //TODO: bug when only 1 row
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
function resizeY(res){
	let resultpos = document.getElementById('results').offsetTop;
	let headerheight = res.querySelector('.resultHeadDiv').offsetHeight;
	let controlsheight = res.querySelector('.tableControls').offsetHeight;
	let tbodydiv = res.querySelector('.resultBodyDiv');
	tbodydiv.style.maxHeight = window.innerHeight - (resultpos + headerheight + controlsheight);
}

function postProcess(res){
	resizeY(res);
	resizeX(res);
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
			dd.classList.toggle('hidden');
		} else {
			dd.classList.add('hidden');
		}
	});
}

var historyList = [];
var currentHist = historyList.length;
function historyClick(direction){
	let queryTextEntry = document.getElementById('queryTextEntry');
	let histNumber = document.getElementById('histNumber');
	if (direction === 1 && currentHist < historyList.length){
		currentHist++;
		queryTextEntry.value = historyList[currentHist-1];
		histNumber.textContent = `${currentHist}/${historyList.length}`;
	}
	if (direction === -1 && currentHist > 1){
		if (currentHist === historyList.length)
			historyList[currentHist-1] = queryTextEntry.value;
		currentHist--;
		queryTextEntry.value = historyList[currentHist-1];
		histNumber.textContent = `${currentHist}/${historyList.length}`;
	}
}
function addHistory(query){
	if (query && !historyList.includes(query))
		historyList.push(query);
	let histNumber = document.getElementById('histNumber');
	let histLen = historyList.length;
	currentHist = histLen;
	histNumber.textContent = `${histLen}/${histLen}`;
}

function submitQuery(savefile = ''){
	let query = document.querySelector('#queryTextEntry').value;
	if (query === ''){
		topMessage("no query!");
		return
	}
	let payload = {
		type: bit.SK_QUERY,
		sessionId: sock.sessionId,
		query,
		savePath: savefile, 
	};
	changeRunButton(1);
	sock.send(payload);
}
function getResult(){
	addHistory(document.querySelector('#queryTextEntry').value);
	var req = new Request("/result/", {
		method: 'GET',
		mode: 'cors',
		cache: "no-cache",
		redirect: 'follow',
		referrer: "no-referrer",
	});
	fetch(req).then(
		res=>res.status >= 400 ? false : res.text()
	).then(dat=>{
		if (dat) {
			document.querySelector('#results').innerHTML = dat;
			document.querySelectorAll('.singleResult').forEach((res,i) => {
				res.idx = i;
				postProcess(res);
			});
		}
	});
}


function showPassprompt(show = true){
	let prompt = document.getElementById('passDropdown');
	if (show){
		prompt.classList.remove('hidden');
	} else {
		prompt.classList.add('hidden');
	}
}

function sendPassword(elem){
	let prompt = elem.closest('div');
	let pass = prompt.querySelector('input').value;
	sock.send({type: bit.SK_PASS, text: pass});
	prompt.classList.add('hidden');
}

function changeRunButton(state){
	let button = document.getElementById('runBut');
	if (state === 0){
		var text = 'Run Query';
		button.classList.add('runbut');
		button.classList.remove('stopbut');
		running = false;
	} else {
		var text = 'End Query Early';
		button.classList.add('stopbut');
		button.classList.remove('runbut');
		running = true;
	}
	button.textContent = text;
}

function pressRunButton(){
	if (running){
		sock.send({type:bit.SK_STOP});
	} else {
		submitQuery();
	}
}

function saveHandler(e) {
	if (e.key !== 'Enter')
		return;
	let path = e.target.value;
	if (path.substr(path.length-4) !== ".csv")
		return;
	console.log('saving',path);
	submitQuery(path);
	e.target.closest('.fileBrowser').classList.add('hidden');
}

function SocketHandler(){
	this.bugtimer = window.performance.now();
	this.sessionId = null;
	let ip = window.location.host;
	this.ws = new WebSocket(`ws://${ip.substring(0,ip.indexOf(':'))}:8061/socket/`);
	this.ws.onopen = e=>console.log("Websocket opened");
	this.ws.onclose = e=>{console.log("Websocket closed"); this.ws = null; };
	this.ws.onerror = e=>console.log("Websocket error: " + e.data);
	this.ws.onmessage = e=>{ 
		let dat = JSON.parse(e.data);
		switch (dat.type) {
			case bit.SK_PING:
				this.bugtimer = window.performance.now();
				break;
			case bit.SK_MSG:
				if (dat.text.includes('Error:'))
					changeRunButton(0);
				topMessage(dat.text);
				break;
			case bit.SK_PASS:
				if (this.lastpass){
					this.send(this.lastpass);
				} else {
					topMessage('Enter encryption password');
					showPassprompt();
				}
				break;
			case bit.SK_ID:
				this.sessionId = dat.id;
				console.log("Websocket session id: ",this.sessionId);
				break;
			case bit.SK_DONE:
				changeRunButton(0);
				getResult();
				break;
		}
	}
	window.setInterval(()=>{
		if (window.performance.now() > this.bugtimer+20000){
			topMessage("Query Engine Disconnected!");
			changeRunButton(0);
		}
	},2000);
	this.send = (data)=>{
		if (data.type === bit.SK_PASS)
			this.lastpass = data;
		this.ws.send(JSON.stringify(data));
	};
}

function init(){
	getState();
	document.addEventListener('click',e=>{
		if (e.target.classList.contains('dropbutton'))
			return;
		let dropdown = e.target.closest('.dropdown');
		if (dropdown == null){
			document.querySelectorAll('.dropdown').forEach(dd => dd.classList.add('hidden'));
		}
	});
	let passDropdown = document.getElementById("passDropdown");
	passDropdown.onkeydown = (e)=>{
		if (e.keyCode === 13) {
			sendPassword(passDropdown);
		}
	};
	document.getElementById('textboxContainer').onkeydown = (e) => {
		if (e.keyCode === 13 && e.shiftKey) {
			e.preventDefault();
			submitQuery();
		}
	};
	document.getElementById('saveinput').addEventListener('keyup', saveHandler);
	addHistory('');
}

var running = false;
var messageDiv = document.getElementById('topMessage');
var sock = new SocketHandler();
const bit = { //TODO: find unused things
	SK_MSG       : 0,
	SK_PING      : 1,
	SK_STOP      : 3,
	SK_PASS      : 6,
	SK_ID        : 7,
	SK_QUERY     : 8,
	SK_DONE      : 9
};
init();
