import React from 'react';

const paramlist = [
	'()',
	'(<expression>)',
	'(<expression>, <expression>)',
	'(<expression>, <2 numbers> or <regex>)',
	"(<expression>, <optional password>)",
	'(<any number of expressions>)',
	'(<expression>, <optional decimal places>)',
	'(<expression> or *)',
	'(<expression>, <format string> or <format code>)',
];

const funclist = [

	["sum" ,"aggregate sum",1],
	["avg" ,"aggregate average",1],
	["min" ,"aggregate minimum",1],
	["max" ,"aggregate maximum",1],
	["count" ,"aggregate count",7],
	["stdev" ,"aggregate sample standard deviation",1],
	["stdevp" ,"aggregate population standard deviation",1],
	["inc" ,"number that increments each line",0],
	["rand","random number",0],
	["abs" ,"absolute value",1],
	["coalesce" ,"first non-null expression",5],
	["now","current local datetime",0],
	["nowlocal","current local datetime",0],
	["nowgm","current GMT datetime",0],
	["year" ,"year",1],
	["month" ,"month (1-12)",1],
	["monthname","month name",1],
	["week" ,"week of year (1-52)",1],
	["dayname" ,"day name",1],
	["dayofyear","day of year (1-366)",1],
	["dayofmonth","day of month (1-31)",1],
	["day" ,"day of month (same as dayofmonth)",1],
	["dayofweek","day of week (1-7, Sunday=1)",1],
	["hour" ,"hour (0-23)",1],
	["minute" ,"minute (0-59)",1],
	["second" ,"second (0-59)",1],
	["format" ,"format datetime text",8],
	["ceil","round up",6],
	["floor","round down",6],
	["round","round up or down to nearest",6],
	["cos","trig cosine",1],
	["sin","trig sine",1],
	["tan","trig tangent",1],
	["acos","trig inverse cosine",1],
	["asin","trig inverse sine",1],
	["atan","trig inverse tangent",1],
	["exp","e to the power of number",1],
	["pow","1st expression to the power of second expression",2],
	["log","log base e",1],
	["log2","log base 2",1],
	["log10","log base 10",1],
	["sqrt","square root",1],
	["cbrt","cube root",1],
	["upper","upper case",1],
	["lower","lower case",1],
	["len","lenth of text",1],
	["substr","substring from indexes or pattern matching",3],
	["md5","md5 hash in base64",1],
	["sha1","sha1 hash in base64",1],
	["sha256","sha256 hash in base64",1],
	["string","convert datatype to text",1],
	["int","convert datatype to integer",1],
	["float","convert datatype to floating point number",1],
	["date","convert datatype to datetime",1],
	["duration","convert datatype to duration",1],
	["encrypt" ,"encrypt",4],
	["decrypt" ,"decrypt",4],
	["base64_encode","encode to bas64",1],
	["base64_decode","decode from base64",1],

]

const fmtlist = [
	[1,  "mm/dd/yy","%m/%d/%y"],
	[101,"mm/dd/yyyy","%m/%d/%Y"],
	[2,  "yy.mm.dd","%y.%m.%d"],
	[102,"yyyy.mm.dd","%Y.%m.%d"],
	[3,  "dd/mm/yy","%d/%m/%y"],
	[103,"dd/mm/yyyy","%d/%m/%Y"],
	[4,  "dd.mm.yy","%d.%m.%y"],
	[104,"dd.mm.yyyy","%d.%m.%Y"],
	[5,  "dd-mm-yy","%d-%m-%y"],
	[105,"dd-mm-yyyy", "%d-%m-%Y"],
	[6,  "dd Mon yy","%d %b %y"],
	[106,"dd Mon yyyy","%d %b %Y"],
	[7,  "Mon dd, yy","%b %d, %y"],
	[107,"Mon dd, yyyy","%b %d, %Y"],
	[8,  "hh:mi:ss","%I:%M:%S"],
	[108,"hh:mi:ss","%I:%M:%S"],
	[9,  "Mon dd yy hh:mi:ss:mmmmAM","%b %d %y %I:%M:%S:mmmm%p"],
	[109,"Mon dd yyyy hh:mi:ss:mmmmAM","%b %d %Y %I:%M:%S:mmmm%p"],
	[10, "mm-dd-yy","%m-%d-%y"],
	[110,"mm-dd-yyyy","%m-%d-%Y"],
	[11, "yy/mm/dd","%y/%m/%d"],
	[111,"yyyy/mm/dd","%Y/%m/%d"],
	[12, "yymmdd","%y%m%d"],
	[112,"yyyymmdd","%Y%m%d"],
	[13, "dd Mon yy hh:mi:ss:mmm(24h)","%d %b %y %H:%M:%S:mmm"],
	[113,"dd Mon yyyy hh:mi:ss:mmm(24h)","%d %b %Y %H:%M:%S:mmm"],
	[14, "hh:mi:ss:mmm(24h)","%H:%M:%S:mmm"],
	[114,"hh:mi:ss:mmm(24h)","%H:%M:%S:mmm"],
	[20, "yy-mm-dd hh:mi:ss(24h)","%y-%m-%d %H:%M:%S"],
	[120,"yyyy-mm-dd hh:mi:ss(24h)","%Y-%m-%d %H:%M:%S"],
	[21, "yy-mm-dd hh:mi:ss.mmm(24h)","%y-%m-%d %H:%M:%S.mmm"],
	[121,"yyyy-mm-dd hh:mi:ss.mmm(24h)","%Y-%m-%d %H:%M:%S.mmm"],
	[126,"yyyy-mm-ddThh:mi:ss.mmm (no Spaces)","%Y-%m-%dT%H:%M:%S.mmm"],
	[127,"yyyy-mm-ddThh:mi:ss.mmmZ (no Spaces)","%Y-%m-%dT%H:%M:%S.mmmZ"],
	[130,"dd Mon yyyy hh:mi:ss:mmmAM","%d %b %Y %I:%M:%S:mmm%p"],
	[131,"dd/mm/yyyy hh:mi:ss:mmmAM","%d/%b/%Y %I:%M:%S:mmm%p"],
		//non standardized additions
	[119,"Mon dd yyyy hh:mi:ssAM","%b %d %Y %I:%M:%S%p"],
	[140,"dd Mon yyyy hh:mi:ssAM","%d %b %Y %I:%M:%S%p"],
	[141,"dd/mm/yyyy hh:mi:ssAM","%d/%b/%Y %I:%M:%S%p"],
	[136,"yyyy-mm-ddThh:mi:ss (no Spaces)","%Y-%m-%dT%H:%M:%S"],
	[137,"yyyy-mm-ddThh:mi:ssZ (no Spaces)","%Y-%m-%dT%H:%M:%SZ"],
];

class FmtTable extends React.Component {
	constructor(){
		super();
		this.table = (
			<table className='helpTable'>
				<th className='helpTable'>Format code</th><th className='helpTable'>Resulting format</th><th className='helpTable'>Corresonding format string</th>
				{fmtlist.map((tr)=>{ return(
					<tr>{tr.map( td => <td className='helpTable'>{td}</td> )}</tr>
					);})}
			</table>
		);
		this.showbut = ( <button onClick={()=>{this.show=true;}} >Show list of preset date formats</button> );
		this.show = false;
	}
	render(){
		return this.show === true ? this.table : this.showbut;
	}
};

const Func = ({fun,usage,params}) => {
	return (<><tr><td className='helpTable'>{fun}</td><td className='helpTable'>{paramlist[params]}</td><td className='helpTable'>{usage}</td></tr></>);
}
class FuncTable extends React.Component {
	constructor(){
		super();
		this.table =  (
			<table className='helpTable'>
				<th className='helpTable'>Function</th><th className='helpTable'>parameters</th><th className='helpTable'>description</th>
				{funclist.map((f)=>{return(<Func fun={f[0]} usage={f[1]} params={f[2]}/>);})}
			</table>
		);
		this.showbut = ( <button onClick={()=>{this.show=true;}} >Show list of functions</button> );
		this.show = false;
	}
	render(){
		return this.show === true ? this.table : this.showbut;
	}
};

export class Help extends React.Component {
	constructor(props) {
		super(props);
		this.state = {
			updateMsg : "",
			updated : false,
		};
	}
	checkupdate(){
		setTimeout(()=>{
			if (this.state.updated || !this.props.notifyUpdate) return;
			let url = new URL(`https://davosaur.com/csv/index.php?d=version2&c=${this.props.version}`);
			let resp = fetch(url);
			console.log(resp);
			resp.then(dat => { if (dat.ok){
				dat.json().then(v => {
					if (this.props.version !== '' && v.version > this.props.version){
						let update = [<a href='https://davosaur.com/csv' target='_blank'>New version available</a> ,':\t', v.version];
						if (v.notes){
							if (Array.isArray(v.notes)){
								update.push(<br/>);
								update.push(<br/>);
								v.notes.forEach(note => { update.push(<li>{note}</li>); });
							} else {
								update = update.concat([<br/>,<br/>,v.notes]);
							}
						}
						this.setState({ updateMsg : update });
					}
				});
			}})
			this.setState({ updated : true });	
		},1000)
	}
	render(){
		if (!this.props.show) return <></>
		this.checkupdate();
		return ( 
			<div className="helpBox">
			<h3>What this software does</h3>
			<hr/>
			{"Run 'select' queries on csv files, display the results, and save to new csv files. It can handle big csv files that are too big for other programs."}
			<br/>
			{"It will show you the first several hundred results in the browser, with 2 options for viewing certain rows or columns. You can click on a column header to sort the displayed results by that column."}
			<h3>How to save files</h3>
			<hr/>
			{"After running a query, hit the save button. Navigate to where you want to save, type in the file name, and hit the save button to the right. All the queries on the page will run again, but this time they will be saved to csv files. If there are multiple queries on the page, you still only need to specify one file and a number will be added to the filename for each one. The program will infer whether or not to output a header unless you override it with an option. To always output header, add 'oh' to the beginning of the query. To never output header, add 'noh'."}
			<h3>How to use the query language</h3>
			<hr/>
			{"This program uses a custom version of SQL loosely based on TSQL. Some features like Unions, Subqueries, and @variables are not implemented yet."}
			<br/>
			<blockquote>
				<h4>Specifying a file</h4>
					{"Click 'Browse Files' to find files, and double click one to add it to the query. If a file has no header, add 'nh' after the file path or alias, or at the very beginning of a query. Us 'ah' (for auto-header) instead to guess if the file has a header based on if the first row contains any numbers."}
				<h4>Using non-comma delimiters</h4>
					{"If the delimiter is not a comma, add the letter for that delimiter to the very beginning of the query. Use 's' for spaces, 't' for tabs, or 'p' for pipes, which are this character: |. Future versions will be able to specify different delimiters for each file in a joining query."}
				<h4>Selecting some columns</h4>
					{"Columns can be specified by name or number. Select column numbers by prefacing the number with 'c', or by using a plain unquoted number if putting a 'c' in front of the whole query. Commas between selections are optional. Result columns can be renamed with the 'as' keyword."}
					<br/><br/>
					Example: selecting columns 1-3, dogs, and cats from a file with no header<br/>
					<blockquote>
						{"nh select c1, c2 as 'food', c3, dogs, cats from '/home/user/pets.csv'"}
						<br/>
						{"select c1 c2 c3 dogs cats from 'C:\\users\\dave\\pets.csv' nh"}
						<br/>
						{"c nh select 1 2 3 dogs cats from 'C:\\users\\dave\\pets.csv'"}
					</blockquote>
				<h4>Selecting all columns</h4>
					{"'select * ' works how you'd expect. If you don't specify any values at all, it will also select all. "}
					<br/><br/>
					Examples:
					<br/>
					<blockquote>
						{"select * from '/home/user/pets.csv'"}
						<br/>
						{"select from '/home/user/pets.csv'"}
					</blockquote>
				<h4>Selecting more complex expressions</h4>
					{"Use + - * / % operators to add, subtract, multiply, divide, and modulus expressions. You can combine them with parentheses. You can also use case expressions."}
					<br/><br/>
					Examples:
					<blockquote>
						{"select birthdate+'3 weeks', c1*c2, c1/c2, c1-c2, c1%2, (c1-23)*(c2+34) from '/home/user/pets.csv'"}
						<br/>
						{"select case when c1*2<10 then dog when c1*10>=10.2 then cat else monkey end from '/home/user/pets.csv'"}
						<br/>
						{"select case c1 / c4 when (c3+c3)*2 then dog when c1*10 then cat end from '/home/user/pets.csv'"}
					</blockquote>
				<h4>Expression Aliases</h4>
					{"Begin a query with 'with <expression> as <alias>' to use that value throughout the query. Separate aliases with ',' or 'and'."}
					<br/><br/>
					Example:
					<br/>
					<blockquote>
						{"with age*7 as dogyears, 'Mr.'+name as dogname select dogname, dogyears from '/home/user/pets.csv' order by dogyears"}
					</blockquote>
				<h4>Functions</h4>
					<blockquote>
						<FuncTable/>
						<br/>
					{"Aggregate functions can be used with a 'group by' clause and group by as many values as you want. Without a 'group by' clause, they will return a single row with the aggregate calculations of the whole file."}
						<br/>
						<br/>
					{"Encryption function only guarantees a unique nonce for every value encrypted while the program is running. Uniqueness cannot be guaranteed between restarts, so avoid using the same password in different sessions. Encryption function uses the chacha20 stream cipher and a 32 bit MtE authenticator. It returns an empty value if ciphertext has been tampered with. It does not hide the length of a value, so it may be best to use other security solutions when possible. If no password is supplied, the program will prompt you for one."}
						<br/>
						<br/>
					{"Math functions like asin and log return an empty value when not a real number, such as log(0) or asin(100). To return nan, inf, or -inf instead, add the option 'nan' to the beginning of the query. Return value will still be empty if function is called on a null value."}
						<br/>
						<br/>
					{"Format function uses the type of date format string described by the "}<a href='https://linux.die.net/man/3/strftime' target='_blank'>Linux Manual</a>{", with the addition of 'mmm' for milliseconds. It can also take the format code for a preset format from the following table:"}
						<br/>
						<FmtTable/>
						<br/>
					</blockquote>
				<h4>Selecting rows or aggregates with a distinct value</h4>
					{"To only return rows with a distinct value, put the 'distinct' keyword in front of the expression that you want to be distinct. Put 'hidden' after 'distinct' if you don't want that value to show up as a result column."}
					<br/><br/>
					{"To calculate aggregate function of distinct values, put the 'distinct' keyword in the function call."}
					<br/><br/>
					Examples:
					<blockquote>
						{"select distinct c3 from '/home/user/pets.csv'"}
						<br/>
						{"select distinct hidden dogtypes, fluffiness from '/home/user/pets.csv'"}
						<br/>
						{"select count(distinct species) from '/home/user/pets.csv'"}
					</blockquote>
				<h4>Selecting a number of rows</h4>
					{"Use the 'top' keyword after 'select', or 'limit' at the end of the query. Be careful not to confuse the number after 'top' for part of an expression."}
					<br/><br/>
					Examples:
					<blockquote>
						{"select top 100 c1 c2 c3 dogs cats from '/home/user/pets.csv'"}
						<br/>
						{"select c1 c2 c3 dogs cats from '/home/user/pets.csv' limit 100"}
					</blockquote>
				<h4>Selecting rows that match a certain condition</h4>
					 {"Use any combinatin of '<expression> <relational operator> <expression>', parentheses, 'and', 'or', 'xor', 'not', and 'between'. Dates are handled nicely, so 'May 18 1955' is the same as '5/18/1955'. Empty entries can be compared against the keyword 'null'."}
					<br/><br/>
					{"Valid relational operators are =,  !=,  <>,  >,  <,  >=,  <=, like, in, and between. '!' is evaluated the same as 'not', and can be put in front of a relational operator or a whole comparison."}
					<br/><br/>
					Examples:
					<blockquote>
						{"select from '/home/user/pets.csv' where dateOfBirth < 'april 10 1980' or age between (c3-19)*1.2 and 30"}
						<br/>
						{"select from '/home/user/pets.csv' where (c1 < c13 or fuzzy = very) and not (c3 = null or weight >= 50 or name not like 'a_b%')"}
						<br/>
						{"select from '/home/user/pets.csv' where c1 in (2,3,5,7,11,13,17,19,23)"}
						<br/>
						{"select from '/home/user/pets.csv' where null not in (c1,c2,c3)"}
					</blockquote>
				<h4>Sorting results</h4>
					{"Use 'order by' at the end of the query, followed by any number of expressions, each followed optionally by 'asc'. Sorts by descending values unless 'asc' is specified."}
					<br/><br/>
					Examples:
					<blockquote>
						{"select from '/home/user/pets.csv' where dog = husky order by age, fluffiness asc"}
						<br/>
						{"select from '/home/user/pets.csv' order by c2*c3"}
					</blockquote>
				<h4>Joining Files</h4>
					{"Any number of files can be joined together. Left and inner joins are allowed, default is inner. Each file needs an alias. Join conditions can have as many comparisions as you want, and is most efficient when the first comparision las the lowest cardinality."}
					<br/><br/>
					Examples:
					<blockquote>
						{"select pet.name, pet.species, code.c1 from '/home/user/pets.csv' pet"
							+" left join '/home/user/codes.csv' code"
							+" on pet.species = code.species order by pet.age"}
						<br/>
						{"select pet.name, code.c1, fur.flufftype from '/home/user/pets.csv' pet"
							+" inner join '/home/user/fur.csv' fur"
							+" on pet.fluffiness = fur.fluff and fur.flufftype <> hairless"
							+" left join '/home/user/codes.csv' code"
							+" on pet.species = code.species or pet.genus = species.genus"}
					</blockquote>
			</blockquote>
			<h3>Ending queries early, viewing older queries, and exiting</h3>
			<hr/>
			{"If a query is taking too long, hit the button next to submit and the query will end and display the results that it found."}
			<br/>
			{"The browser remembers previous queries. In the top-right corner, it will show you which query you are on. You can revisit other queries by hitting the forward and back arrows around the numbers."}
			<br/>
			{"To exit the program, just leave the browser page. The program exits if it goes 3 minutes without being viewed in a browser."}
			<h3>Waiting for results to load</h3>
			<hr/>
			{"Browsers can take a while to load big results, even when limiting the number of rows. If the results of a query look similar to the results of the previous query, you can confirm that they are new by checkng the query number in the top-right corner, or by reading the query text in the yellow area above the table."}
			<br/><br/>
			{"Another thing that can take a while is files that have tons of columns. The program samples the first 10000 rows to figure out datatypes and this can take a while with very wide files. That process is not yet interuptable by the 'end query early' button."}
			<h3>Configuration</h3>
			<hr/>
			{"Some settings can be configured by editing the config file "}{this.props.configpath}
			<br/> <br/>
			<hr/>
			version {this.props.version}
			<hr/>
			{this.state.updateMsg}
			<br/><br/>
			</div>
		)
	}
}
