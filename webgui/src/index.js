import React from 'react';
import ReactDOM from 'react-dom';
import './index.css';
import './style.css';
import {postRequest,getRequest,bit} from './utils.js';
import * as command from './command.js';
import * as display from './display.js';
import * as help from './help.js';
import * as topbar from './topbar.js';
import * as serviceWorker from './serviceWorker';
//import testdata from './testdata.json';


class Main extends React.Component {
	constructor(props) {
		super(props);

		this.state = {
			topMessage : "",
			topDropdown : "nothing",
			openDirList : {},
			saveDirList : {},
			queryHistory: ['',],
			historyPosition : 0,
			showQuery : <></>,
			showHelp : 0,
			notifyUpdate : true,
			configpath : "",
			version : "",
			sessionId : "",
			lastpass : "",
		}
		this.topDropReset = this.topDropReset.bind(this);

		//restore previous session or initialize paths
		getRequest({info:"getState"})
			.then(dat=>{
				if (dat.history) this.setState({ queryHistory : dat.history, historyPosition : dat.history.length-1 });
				if (dat.openDirList) this.setState({ openDirList : dat.openDirList });
				if (dat.saveDirList) this.setState({ saveDirList : dat.saveDirList });
				if (dat.version) this.setState({ version : dat.version });
				if ('notifyUpdate' in dat) this.setState({ notifyUpdate : dat.notifyUpdate });
				if (dat.configpath) this.setState({ configpath : dat.configpath });
				var textbox = document.getElementById("textBoxId");
				if (textbox != null)
					textbox.value = this.state.queryHistory[this.state.historyPosition].query || "";
			});
	}
	showLoadedQuery(results){
		if (results.status & bit.DAT_ERROR){
			if (results.message === undefined || results.message === ""){
				alert("Could not make query or get error message from query engine");
				console.log(results);
			}else
				alert(results.message);
		}
		else if (results.status & bit.DAT_GOOD){
			this.setState({
				topDropdown : "nothing",
				showQuery : results.entries.map(
					tab => <display.QueryRender 
							   table = {tab} 
							   hideColumns = {new Int8Array(tab.numcols)}
							   rows = {new Object({col:"",val:"*"})}
						   />) });
			postRequest({path:"/info?info=setState",body:{
				haveInfo : true,
				currentQuery : document.getElementById("textBoxId").value,
				history : this.state.queryHistory,
				openDirList : this.state.openDirList,
				saveDirList : this.state.saveDirList,
			}});
		}
	}

	submitQuery(querySpecs){
		var fullQuery = {
			sessionId: this.state.sessionId || "",
			query: querySpecs.query || "", 
			fileIO: querySpecs.fileIO || 0, 
			savePath: querySpecs.savePath || "", 
			};
		postRequest({path:"/query/",body:fullQuery}).then(dat=>{
			if ((dat.status & bit.DAT_GOOD) && (!querySpecs.backtrack)){
				this.setState({ historyPosition : this.state.queryHistory.length,
								queryHistory : this.state.queryHistory.concat({query : dat.originalQuery}) });
			}
			this.showLoadedQuery(dat);
		});
	}
	sendSocket(request){
		this.ws.send(JSON.stringify(request));
	}

	viewHistory(position){
		this.setState({ historyPosition : position });
		var textbox = document.getElementById("textBoxId");
		if (textbox != null) { textbox.value = this.state.queryHistory[position].query; }
	}
	topDropReset(e){ 
		setTimeout(()=>{
			if (!e.target.matches('.dropContent'))
				this.setState({ topDropdown : "nothing" }); 
			if (this.state.showHelp && !e.target.matches('.helpDrop'))
				this.setState({ showHelp : false }); 
		},50);
	}
	changeFilePath(whichPath){
		if (whichPath.type === "open")
			this.state.openDirList.path = whichPath.path;
		if (whichPath.type === "save")
			this.state.saveDirList.path = whichPath.path;
		this.forceUpdate();
	}

	fileClick(request){
		postRequest({path:"/info?info=fileClick",body:{
			path : request.path,
			mode : request.mode,
			sessionId: this.state.sessionId,
		}}).then(dat=>{if (dat.mode) {
			switch (dat.mode){
			case "open":
				this.setState({openDirList: dat});
				break;
			case "save":
				this.setState({saveDirList: dat});
				break;
			default:
				break;
			}
		} else {
			this.setState({topMessage: (dat.error ? dat.error : "error")});
		}});
	}

	render(){
	
		document.addEventListener('click',this.topDropReset);

		return (
		<>
		<topbar.TopMenuBar
			s = {this.state}
			updateTopMessage = {(message)=>this.setState({ topMessage : message })}
			submitQuery = {(query)=>this.submitQuery(query)}
			viewHistory = {(position)=>this.viewHistory(position)}
			changeTopDrop = {(section)=>this.setState({ topDropdown : section })}
			toggleHelp = {()=>{this.setState({showHelp:this.state.showHelp^1})}}
			showHelp = {this.state.showHelp}
			openDirList = {this.state.openDirList}
			saveDirList = {this.state.saveDirList}
			changeFilePath = {(path)=>this.changeFilePath(path)}
			sendSocket = {(request)=>this.sendSocket(request)}
			fileClick = {(request)=>this.fileClick(request)}
		/>
		<help.Help
			version = {this.state.version}
			show = {this.state.showHelp}
			notifyUpdate = {this.state.notifyUpdate}
			configpath = {this.state.configpath}
			toggleHelp = {()=>{this.setState({showHelp:this.state.showHelp^1})}}
		/>

		<div className="querySelect"> 
		<command.QueryBox
			s = {this.state}
			showLoadedQuery = {(results)=>this.showLoadedQuery(results)}
			submitQuery = {(query)=>this.submitQuery(query)}
			sendSocket = {(request)=>this.sendSocket(request)}
		/>
		{this.state.showQuery}
		</div>
		</>
		)
	}

	componentDidMount(){
		//websocket
		var bugtimer = window.performance.now() + 30000
		var that = this;
		this.ws = new WebSocket("ws://localhost:8061/socket/");
		console.log(this.ws);
		this.ws.onopen = function(e) { console.log("OPEN"); }
		this.ws.onclose = function(e) { console.log("CLOSE"); that.ws = null; } 
		this.ws.onmessage = function(e) { 
			console.log(e.data);
			var dat = JSON.parse(e.data);
			switch (dat.type) {
			case bit.SK_PING:
				bugtimer = window.performance.now() + 20000
				break;
			case bit.SK_MSG:
				that.setState({ topMessage : dat.text }); 
				break;
			case bit.SK_PASS:
				if (that.state.lastpass){
					that.sendSocket({type: bit.SK_PASS, text: that.state.lastpass});
					that.state.lastpass = "";
				} else {
					that.setState({ topDropdown : "passShow" });
				}
				break;
			case bit.SK_ID:
				that.setState({ sessionId : dat.id });
				console.log("WS session id: "+that.state.sessionId);
				break;
			default:
				break;
			}
		}
		this.ws.onerror = function(e) { console.log("ERROR: " + e.data); } 
		window.setInterval(()=>{
			if (window.performance.now() > bugtimer+20000)
				that.setState({ topMessage : "Query Engine Disconnected!"})
		},2000);
	}
	componentWillMount() { document.title = 'CSV Query Tool' }
}

ReactDOM.render(
	<Main/>, 
	document.getElementById('root'));



// If you want your app to work offline and load faster, you can change
// unregister() to register() below. Note this comes with some pitfalls.
// Learn more about service workers: http://bit.ly/CRA-PWA
serviceWorker.unregister();
