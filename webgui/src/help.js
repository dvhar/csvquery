import React from 'react';

const paramlist = [
	'()',
	'(<expression>)',
	'(<expression>, <expression>)',
	'(<expression>, <2 numbers> or <regex>)',
	"(<expression>, 'big' or 'small', <password>)",
	'(<any number of expressions>)',
	'(<expression>, <optional decimal places>)',
	'(<expression> or *)',
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
	["year" ,"year number",1],
	["month" ,"0-11 month number",1],
	["monthname","month name",1],
	["week" ,"0-52 week of year number",1],
	["day" ,"day of month number (same as dayofmonth())",1],
	["dayname" ,"day name",1],
	["dayofyear","day of year number",1],
	["dayofmonth","day of month number",1],
	["dayofweek","0-6 day of week number",1],
	["hour" ,"0-23 hour number",1],
	["minute" ,"0-59 minute number",1],
	["second" ,"0-59 second number",1],
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
	["substr","sustring from indexes or pattern matching",3],
	["md5","md5 hash",1],
	["sha1","sha1 hash",1],
	["sha256","sha256 hash",1],
	["string","convert datatype to text",1],
	["int","convert datatype to integer",1],
	["float","convert datatype to floating point number",1],
	["encrypt" ,"encrypt with chacha cipher, output base64",4],
	["decrypt" ,"input base64, decrypt with chacha cipher",4],
	["base64_encode","encode as bas64 (not needed for encrypting)",1],
	["base64_decode","decode from base64 (not needed for encryption)",1],

]

const Func = ({fun,usage,params}) => {
	return (
		<>
		<tr><td>{fun}</td><td>{paramlist[params]}</td><td>{usage}</td> </tr>
		</>
	);
}
class FuncTable extends React.Component {
	render(){
		return (
		<table>
			<th>Function</th><th>parameters</th><th>description</th>
			{funclist.map((f,i)=>{return(<Func fun={f[0]} usage={f[1]} params={f[2]}/>);})}
		</table>
		);
	}
};

export class Help extends React.Component {
	render(){
		//if (!this.props.show) return <></>
		return ( 
			<div className="helpBox">
			<h3>What this software does</h3>
			<hr/>
			{"Run queries on csv files, display the results, and save to new csv files. It can handle big csv files that are too big for other programs."}
			<br/>
			{"The program will show you the first several hundred results in the browser, with 2 options for viewing certain rows or columns. You can click on a column header to sort the displayed results by that column."}
			<h3>How to save files</h3>
			<hr/>
			{"After running a query, hit the save button. Navigate to where you want to save, type in the file name, and hit the save button to the right. All the queries on the page will run again, but this time they will be saved to csv files. If there are multiple queries on the page, you still only need to specify one file and a number will be added to the filename for each one."}
			<h3>How to use the query language</h3>
			<hr/>
			{"This program uses a custom version of SQL based on TSQL. Some features like Unions, Subqueries, and @variables are not implemented yet."}
			<br/>
			<blockquote>
				<h4>Specifying a file</h4>
					{"Click 'Browse Files' to find files, and double click one to add it to the query. If a file has no header, add 'nh' or 'noheader' after the file path or alias, or before 'select'."}
					<h4>Selecting some columns</h4>
					{"Columns can be specified by name or number. Select column numbers by prefacing the number with 'c', or by using a plain unquoted number if putting a 'c' in front of the whole query. Commas between selections are optional. Aliases use the 'as' keyword."}
					<br/><br/>
					Example: selecting columns 1-3, dogs, and cats from a file<br/>
					<blockquote>
						{"select c1, c2 as 'food', c3, dogs, cats from '/home/user/pets.csv'"}
						<br/>
						{"select c1 c2 c3 dogs cats from 'C:\\users\\dave\\pets.csv' nh"}
						<br/>
						{"c select 1 2 3 dogs cats from 'C:\\users\\dave\\pets.csv'"}
					</blockquote>
				<h4>Selecting all columns</h4>
					{"'select * ' works how you'd expect. If you don't specify any columns at all, it will also select all. "}
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
					{"Begin a query with 'with <expression> as <alias>' to use that value throughout the query. Separate aliases with ',' or 'and'.'"}
					<br/><br/>
					Example:
					<br/>
					<blockquote>
						{"with age*7 as dogyears, 'Mr.'+name as dogname select dogname, dogyears from '/home/user/pets.csv' order by dogyears"}
					</blockquote>
				<h4>Functions</h4>
					<blockquote>
						<FuncTable/>
					</blockquote>
				<h4>Selecting rows with a distinct value</h4>
					{"Put the 'distinct' keyword in front of the expression that you want to be distinct. Put 'hidden' after 'distinct' if you don't want that value to show up as a result column."}
					<br/><br/>
					Examples:
					<blockquote>
						{"select distinct c3 from '/home/user/pets.csv'"}
						<br/>
						{"select distinct hidden dogtypes, fluffiness from '/home/user/pets.csv'"}
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
					 {"Use any combinatin of '<expression> <relational operator> <expression>', parentheses, 'and', 'or', 'not', and 'between'. Dates are handled nicely, so 'May 18 1955' is the same as '5/18/1955'. Empty entries can be comparied against the keyword 'null'."}
					<br/><br/>
					{"Valid relational operators are =,  !=,  <>,  >,  <,  >=,  <=, like, and between. '!' is evaluated the same as 'not', and can be put in front of a relational operator or a whole comparison."}
					<br/><br/>
					Examples:
					<blockquote>
						{"select from '/home/user/pets.csv' where dateOfBirth < 'april 10 1980' or age between (c3-19)*1.2 and 30"}
						<br/>
						{"select from '/home/user/pets.csv' where (c1 < c13 or fuzzy = very) and not (c3 = null or weight >= 50 or name not like 'a_b%')"}
						<br/>
						{"select from '/home/user/pets.csv' where c1 in (2,3,5,7,11,13,17,19,23)"}
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
							+" on pet.fluffiness = fur.fluff"
							+" left join '/home/user/codes.csv' code"
							+" on pet.species = code.species"}
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
			{"Browsers can take a while to load big results, even when limiting the number of rows. If the results of a query look similar to the results of the previous query, you can confirm that they are new by checkng the query number in the top-right corner."}
			<br/><br/>
			<hr/>
			version 1.0.0 - 10/8/2020
			<hr/>
			<br/><br/>
			</div>
		)
	}
}
