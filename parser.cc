#include "interpretor.h"
#include "scanner.h"
#include <boost/algorithm/string/replace.hpp>
#include<boost/filesystem.hpp>

/*
{} = optional
[] = one of
[[]] = at least one of

query             -> <preselect> <select> <from> <afterfrom> | <addalias>
handlealias       -> add filealias <file> | drop filealias | show tables
preselect         -> <options> <with> | ε
options           -> [ oh noh nh h ah s p t nan m ] { <options> } | ε
with              -> with <vars> | ε
vars              -> <expradd> as alias { [ , and ] vars }
select            -> select { top number } <selections> | ε
quantifier        -> top number | distinct | <quantifier> | ε
selections        -> {,} <expradd> <selections> | ε
expradd           -> <exprmult> { [ + - ] <expradd> }
exprmult          -> <exprneg> { [ * % ^ / ] <exprmult> }
exprneg           -> { - } <exprcase>
<exprCase>        -> case <caseWhenPredList> { else <expradd> } end
                   | case <expradd> <caseWhenExprList> { else <expradd> } end
                   | <value>
                   | ( <expradd> )
caseWhenExprList> -> <caseWhenExpr> <caseWhenExprList> | <caseWhenExpr>
caseWhenExpr      -> when <exprAdd> then <exprAdd>
caseWhenPredList  -> <casePredicate> <caseWhenPredList> | <casePredicate>
casePredicate     -> when <predicates> then <exprAdd>
predicates        -> { not }  <predicateCompare> { <logop> <predicates> }
value             -> column | literal | variable | <function>
predicateCompare  -> { not } <expradd> { not } <relop> { not } <expradd>
                    | { not } <expradd> { not } between <expradd> and <expradd>
                    | { not } <expradd> in ( <setlist> )
                    | { not } ( predicates )
setlist           -> <expressionlist> | <query>
function          -> func ( params )
expressionlist    -> <expradd> {,} <expressionlist> | ε
from              -> from <file> <join>
join              -> {[left inner]} join <file> on <predicates> <join> | ε
file              -> [ file view ] { <fileoptions> } { {as} alias } { <fileoptions> }
                    | ( <query> ) { {as} alias }
fileoptions       -> [ alias nh h ah s t p ] <fileoptions> | ε
afterfrom         -> {[[ <where> <group> <having> <order> <limit> ]]}
where             -> where <predicates>
having            -> having <predicates>
order             -> order by <expressionlist>
group             -> group by <expressionlist>
limit             -> limit number
*/

#define newNode(l) make_unique<node>(l)

//for debugging
static int ident = 0;
//#define e //turn off debug printer
#ifndef e
#define e(A) { \
	string spc; \
	for (int i=0; i< ident; i++) spc += "    "; \
	perr(st(spc,  A , " : '", t.val,"'")); \
	ident++; } \
	shared_ptr<void> _(nullptr, [&](...){ \
		ident--; \
		string spc; \
		t = sc.currToken(); \
		for (int i=0; i< ident; i++) spc += "    "; \
	perr(st(spc,"done ",A, " : '", t.val,"'")); });
#endif

class parser {
	void parseOptions(astnode& n);
	astnode parsePreSelect();
	astnode parseWith();
	astnode parseVars();
	astnode parseSelect();
	astnode parseSelections();
	astnode parseExprAdd();
	astnode parseExprMult();
	astnode parseExprNeg();
	astnode parseExprCase();
	astnode parseCaseWhenPredList();
	astnode parseCaseWhenExprList();
	astnode parseCaseWhenExpr();
	astnode parseCasePredicate();
	astnode parsePredicates();
	astnode parsePredCompare();
	astnode parseJoin();
	astnode parseOrder();
	astnode parseHaving();
	astnode parseGroupby();
	astnode parseFrom(bool); //true for requires selections beforehand
	astnode parseWhere();
	astnode parseValue();
	astnode parseFunction();
	astnode parseAfterFrom();
	astnode parseFile(bool join);
	astnode parseSetList(bool);
	astnode parseExpressionList(bool i, bool s);
	astnode parseQuery();
	astnode parseHandleAlias();
	void parseFileOptions(astnode&);
	void parseQuantifier(astnode&);
	void parseLimit(astnode&);

	querySpecs* q;
	stack<bool> justfiles;
	bool needcomma = globalSettings.needcomma;
	bool firstselection = true;
	token t;
	scanner sc;
	public:
	parser(querySpecs &qs): q{&qs}, sc(qs) {}
	void parse(){
		q->tree = parseQuery();
	}
};


//run recursive descent parser for query
void parseQuery(querySpecs &q) {
	if (q.isSubquery) return;
	parser pr(q);
	pr.parse();
}

//node1 is preselect
//node2 is select
//node3 is from
//node4 is afterfrom
//or
//node1 is handlealias
//tok1.id is N_HANDLEALIAS
//tok2 is add/drop/show
astnode parser::parseQuery() {
	justfiles.push(false);
	t = sc.currToken();
	if (t == EOS){
		error("Empty query");
	}
	e("parse query");
	astnode n = newNode(N_QUERY);
	if (t == "add" || t == "drop" || t == "show"){
		n->tok2 = t;
		n->node1 = parseHandleAlias();
		n->tok1.id = N_HANDLEALIAS;
	} else {
		n->node1 = parsePreSelect();
		n->node3 = parseFrom(false);
		n->node2 = parseSelect();
		if (!justfiles.top())
			n->node3 = parseFrom(true);
		n->node4 = parseAfterFrom();
	}
	t = sc.currToken();
	switch (t.id){
		case EOS:
		case SP_RPAREN:
		break;
		default:
			error("Unexpected token at and of query: '",t,"'");
	}
	justfiles.pop();
	return n;
}

//node1 is file
//tok1 is alias
//tok2 is add/drop/show
astnode parser::parseHandleAlias() {
	t = sc.currToken();
	e("parse handlealias");
	astnode n = newNode(N_HANDLEALIAS);
	n->tok2 = t;
	t = sc.nextToken();
	n->tok1 = t;
	sc.nextToken();
	if (n->tok2 == "drop" && t != WORD_TK)
			error("Expected file alias after 'drop'. Found '",t,"'");
	if (n->tok2 == "add"){
		if (t != WORD_TK)
			error("Expected file alias after 'add'. Found '",t,"'");
		n->node1 = parseFile(false);
	}
	return n;
}

//node1 is with
//tok3.id is options
astnode parser::parsePreSelect() {
	t = sc.currToken();
	e("parse preselect");
	astnode n = newNode(N_PRESELECT);
	parseOptions(n);
	n->node1 = parseWith();
	return n;
}

//anything that comes after 'from', in any order
//tok1.id is quantitylimit
astnode parser::parseAfterFrom() {
	t = sc.currToken();
	e("parse afterfrom");
	astnode n = newNode(N_AFTERFROM);
	for (;;){
		t = sc.currToken();
		switch (t.id){
		case KW_WHERE:
			n->node1 = parseWhere();
			break;
		case KW_GROUP:
			n->node2 = parseGroupby();
			break;
		case KW_HAVING:
			n->node3 = parseHaving();
			break;
		case KW_ORDER:
			n->node4 = parseOrder();
			break;
		case KW_LIMIT:
			parseLimit(n);
			break;
		default:
			goto done;
		}
	}
	done:
	return n;
}

//tok3.id of arg node is options
void parser::parseOptions(astnode& n) {
	t = sc.currToken();
	e("parse options");
	int& opts = n->optionbits();
	if (t == "c") {
		opts |= O_C;
	} else if (t == "oh") {
		if (opts & O_NOH) error("Cannot have multiple output header options");
		opts |= O_OH;
	} else if (t == "noh") {
		if (opts & O_NOH) error("Cannot have multiple output header options");
		opts |= O_NOH;
	} else if (t == "nh") {
		if (opts & (O_H|O_AH)) error("Cannot have multiple input header options");
		opts |= O_NH;
	} else if (t == "h") {
		if (opts & (O_NH|O_AH)) error("Cannot have multiple input header options");
		opts |= O_H;
	} else if (t == "ah") {
		if (opts & (O_NH|O_H)) error("Cannot have multiple input header options");
		opts |= O_AH;
	} else if (t == "s") {
		if (opts & (O_P|O_T|O_SC)) error("Cannot have multiple delimiter options");
		opts |= O_S;
	} else if (t == "p") {
		if (opts & (O_S|O_T|O_SC)) error("Cannot have multiple delimiter options");
		opts |= O_P;
	} else if (t == "t") {
		if (opts & (O_P|O_S|O_SC)) error("Cannot have multiple delimiter options");
		opts |= O_T;
	} else if (t == "sc") {
		if (opts & (O_P|O_S|O_T)) error("Cannot have multiple delimiter options");
		opts |= O_SC;
	} else if (t == "nan") {
		opts |= O_NAN;
	} else if (t == "m") {
		needcomma = false;
	} else {
		return;
	}
	sc.nextToken();
	parseOptions(n);
}

//node1 is vars
astnode parser::parseWith() {
	t = sc.currToken();
	e("parse with");
	if (t != "with") { return nullptr; }
	astnode n = newNode(N_WITH);
	sc.nextToken();
	n->node1 = parseVars();
	return n;
}

//node1 is expr
//tok1 is alias
//tok3 is midrow index-1 of phase2 non-aggs
//node2 is vars
astnode parser::parseVars() {
	t = sc.currToken();
	e("parse vars");
	astnode n = newNode(N_VARS);
	n->node1 = parseExprAdd();
	t = sc.currToken();
	if (t != "as") error("Expected 'as' after expression. Found '",t,"'");
	t = sc.nextToken();
	if (t != WORD_TK) error("Expected variable name. Found '",t,"'");
	n->tok1 = t;
	t = sc.nextToken();
	if (t == "and" || t == SP_COMMA) {
		sc.nextToken();
		n->node2 = parseVars();
	}
	return n;
}

//node2 is selections (same as N_SELECTIONS)
//tok1.id is quantityLimit
astnode parser::parseSelect() {
	t = sc.currToken();
	e("parse select");
	if (justfiles.top()) return nullptr;
	astnode n = newNode(N_SELECT);
	if (t != "select") error("Expected 'select'. Found '",t,"'");
	sc.nextToken();
	parseQuantifier(n);
	n->node2 = parseSelections();
	return n;
}

//node1 is expression
//node2 is next selection
//tok2 is alias
//later stages:
//  tok3.id will be midrow index
//  tok4.id will be destrow index
astnode parser::parseSelections() {
	t = sc.currToken();
	e("parse selections");
	if (t == "from"){
		return nullptr;
	}
	astnode n = newNode(N_SELECTIONS);
	if (t == SP_COMMA) {
		t = sc.nextToken();
	} else if (needcomma && !firstselection){
		error("Expected 'from' clause or comma before next selection. Found: '",t,"'");
	}
	switch (t.id) {
	case SP_STAR:
		n->tok1 = t;
		sc.nextToken();
		n->node2 = parseSelections();
		return n;
	//expression
	case KW_CASE:
	case WORD_TK:
	case SP_MINUS:
	case SP_LPAREN:
		//alias = expression
		if (sc.peekToken() == SP_EQ) {
			if (t.asWord() != WORD_TK) error("Alias must be a word. Found '",t,"'");
			n->tok2 = t;
			sc.nextToken();
			sc.nextToken();
			n->node1 = parseExprAdd();
		//expression
		} else {
			n->node1 = parseExprAdd();
			t = sc.currToken();
			if (t == "as") {
				t = sc.nextToken();
				n->tok2 = t.asWord();
				sc.nextToken();
			} else if (t != "from" && t.toWord() == WORD_TK && needcomma){
				n->tok2 = t.asWord();
				sc.nextToken();
			}
		}
		if (sc.currToken() == SP_LPAREN)
			error("Cannot start expression after '", sc.prevToken(),
						"' with parenthesis. Did you misspell a function or forget a comma between selections?");
		firstselection = false;
		n->node2 = parseSelections();
		return n;
	default:
		error("Expected a new selection or 'from' clause. Found '",t,"'");
	}
	return nullptr;
}

//node1 is exprMult
//node2 is exprAdd
//tok1 is add/minus operator
astnode parser::parseExprAdd() {
	t = sc.currToken();
	e("parse expradd");
	astnode n = newNode(N_EXPRADD);
	n->node1 = parseExprMult();
	t = sc.currToken();
	switch (t.id) {
	case SP_MINUS:
	case SP_PLUS:
		n->tok1 = t;
		sc.nextToken();
		n->node2 = parseExprAdd();
	}
	return n;
}

//node1 is exprNeg
//node2 is exprMult
//tok1 is mult/div operator
astnode parser::parseExprMult() {
	t = sc.currToken();
	e("parse exprmult");
	astnode n = newNode(N_EXPRMULT);
	n->node1 = parseExprNeg();
	t = sc.currToken();
	switch (t.id) {
	case SP_STAR:
		if (sc.peekToken() == "from") { break; }
	case SP_MOD:
	case SP_CARROT:
	case SP_DIV:
		n->tok1 = t;
		sc.nextToken();
		n->node2 = parseExprMult();
	}
	return n;
}

//tok1 is minus operator
//node1 is exprCase
astnode parser::parseExprNeg() {
	t = sc.currToken();
	e("parse exprneg");
	astnode n = newNode(N_EXPRNEG);
	if (t == SP_MINUS) {
		n->tok1 = t;
		sc.nextToken();
	}
	n->node1 = parseExprCase();
	return n;
}

//tok1 is [case, word, expr] token - tells if case, terminal value, or (expr)
//tok2 is [when, expr] token - tells what kind of case. predlist, or expr exprlist respectively
//node1 is (expression), when predicate list, expression for exprlist
//node2 is expression list to compare to initial expression
//node3 is else expression
//later stages:
//  tok3.id is datatype of when comparison expressions
astnode parser::parseExprCase() {
	t = sc.currToken();
	e("parse exprcase");
	astnode n = newNode(N_EXPRCASE);
	token t2;
	switch (t.id) {
	case KW_CASE:
		n->tok1 = t;

		t2 = sc.nextToken();
		switch (t2.id) {
		//when predicates are true
		case KW_WHEN:
			n->tok2 = t2;
			n->node1 = parseCaseWhenPredList();
			break;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			n->tok2 = t2;
			n->node1 = parseExprAdd();
			t = sc.currToken();
			if (t != "when") error("Expected 'when' after case expression. Found '",t,"'");
			n->node2 = parseCaseWhenExprList();
			break;
		default: error("Expected expression or 'when'. Found ",sc.currToken());
		}

		switch (sc.currToken().id) {
		case KW_END:
			sc.nextToken();
			break;
		case KW_ELSE:
			t = sc.nextToken();
			n->node3 = parseExprAdd();
			t = sc.currToken();
			if (t != "end") error("Expected 'end' after 'else' expression. Found '",t,"'");
			sc.nextToken();
			break;
		default:
			error("Expected 'end' or 'else' after case expression. Found '",sc.currToken(),"'");
		}
		break;

	case WORD_TK:
		n->tok1 = t;
		n->node1 = parseValue();
		break;

	case SP_LPAREN:
		n->tok1 = t;
		sc.nextToken();
		n->node1 = parseExprAdd();
		if (sc.currToken() != SP_RPAREN) error("Expected closing parenthesis. Found '",sc.currToken(),"'");
		sc.nextToken();
		break;
	default: error("Expected case, value, or expression. Found ",sc.currToken());
	}
	return n;
}

//tok1 is [value, column, function name, variable name]
//tok2.id is FUNCTION if function
//node1 is function expression if doing that
//later stages:
//  tok1.id is colIdx
//  tok2.id is type [literal, column, variable, function] enum
//  tok3.val is source file alias
//  tok3.id is trivial indicator
//  tok4.id is trivial alias indicator
astnode parser::parseValue() {
	t = sc.currToken();
	e("parse value");
	astnode n = newNode(N_VALUE);
	n->tok1 = t;
	//see if it's a function
	if (functionMap.count(t.lower()) && !t.quoted && sc.peekToken()==SP_LPAREN) {
		n->tok2.id = FUNCTION;
		n->node1 = parseFunction();
	//any non-function value
	} else {
		sc.nextToken();
	}
	return n;
}

//node1 is case predicate
//node2 is next case predicate list node
astnode parser::parseCaseWhenPredList() {
	t = sc.currToken();
	e("parse casewhenpredlist");
	astnode n = newNode(N_CPREDLIST);
	n->node1 = parseCasePredicate();
	t = sc.currToken();
	if (t == "when") {
		n->node2 = parseCaseWhenPredList();
	}
	return n;
}

//node1 is predicates
//node2 is expression if true
astnode parser::parseCasePredicate() {
	t = sc.currToken();
	e("parse casepredicate");
	astnode n = newNode(N_CPRED);
	sc.nextToken(); //eat when token
	n->node1 = parsePredicates();
	t = sc.currToken();
	if (t != "then") error("Expected 'then' after predicate. Found: '",sc.currToken(),"'");
	sc.nextToken(); //eat then token
	n->node2 = parseExprAdd();
	return n;
}

//tok1 is logop
//tok2 is negation
//node1 is predicate comparison
//node2 is next predicates node
//later:
//	[ANDCHAIN] is 1 if part of simple 'and' chain
//	[CHAINSIZE] is number of chained expressions to scan if first of join chain
//	[CHAINIDX] is index of andchain
//	[FILENO] is file number
astnode parser::parsePredicates() {
	t = sc.currToken();
	e("parse predicates");
	astnode n = newNode(N_PREDICATES);
	if (t == SP_NEGATE) { n->tok2 = t; t = sc.nextToken(); }
	n->node1 = parsePredCompare();
	t = sc.currToken();
	if ((t.id & LOGOP) != 0) {
		n->tok1 = t;
		sc.nextToken();
		n->node2 = parsePredicates();
	}
	return n;
}

//tok1 is [relop, lparen] for comparison or more predicates
//tok2 is negation
//tok3 is 'like' expression
//node1 is [expr, predicates]
//node2 is second expr, <setlist> if inlist
//node3 is third expr for betweens
//later:
//  [TOSCAN] will be number of node (1,2) for indexable join value
//  [ANDCHAIN] is 1 or 2 if part of simple and chain, 2 if not first
//  [VALPOSIDX] is index of valpos vector for joins
astnode parser::parsePredCompare() {
	t = sc.currToken();
	e("parse predcompare");
	astnode n = newNode(N_PREDCOMP);
	int negate = 0;
	if (t == SP_NEGATE) { negate ^= 1; t = sc.nextToken(); }
	//more predicates in parentheses
	if (t == SP_LPAREN) {
		auto pos = sc.getPos();
		//try parsing as predicate
		t = sc.nextToken();
		bool tryAgain = false;
		bool ispreds = false;
		try {
			n->node1 = parsePredicates();
			t = sc.currToken();
			ispreds = true;
		//if failed, reparse as expression
		} catch (...) {
			sc.setPos(pos);
			tryAgain = true;
		}
		if (ispreds) {
			if (t != SP_RPAREN) error("Expected closing parenthesis. Found: '",t,"'"); // should not be caught in try block
			t = sc.nextToken();
		}
		//return if (more comparisions)
		if (!tryAgain) {
			n->tok1.id = SP_LPAREN;
			return n;
		}
	}
	//comparison
	auto nullInList = sc.currToken() == "null";
	n->node1 = parseExprAdd();
	t = sc.currToken();
	if (t == SP_NEGATE) {
		negate ^= 1;
		t = sc.nextToken();
	}
	if ((t.id & RELOP)==0 && t!="between" && t!="like" && t!="in")
		error("Expected relational operator. Found: '",t,"'");
	t = sc.currToken();
	n->tok1 = t;
	t = sc.nextToken();
	if (t == SP_NEGATE) {
		negate ^= 1;
		t = sc.nextToken();
	}
	if (n->tok1 == "like") {
		n->tok1.id = KW_LIKE;
		n->tok3 = t;
		t = sc.nextToken();
	} else if (n->tok1 == "in") {
		n->tok1.id = KW_IN;
		if (t != SP_LPAREN) error("Expected opening parenthesis for expression list. Found: '",t,"'");
		sc.nextToken();
		n->node2 = parseSetList(!nullInList);
		t = sc.currToken();
		if (t != SP_RPAREN) error("Expected closing parenthesis after expression list. Found: '",t,"'");
		sc.nextToken();
	} else {
		n->node2 = parseExprAdd();
	}
	if (n->tok1 == "between") {
		n->tok1.id = KW_BETWEEN;
		sc.nextToken();
		n->node3 = parseExprAdd();
	}
	n->tok2.id = negate;
	return n;
}

//node1 is expressionlist or query
//tok1.id is 1 if subquery else 0
//tok2.id is index of subquery
astnode parser::parseSetList(bool interdependant) {
	t = sc.currToken();
	e("parse setlist");
	astnode n = newNode(N_SETLIST);
	auto pos = sc.getPos();
	try {
		n->node1 = parseQuery();
		n->tok1.id = 1;
	} catch (exception ex){
		sc.setPos(pos);
		n->node1 = parseExpressionList(interdependant, false);
	}
	return n;
}

//node1 is case expression
//node2 is next exprlist node
astnode parser::parseCaseWhenExprList() {
	t = sc.currToken();
	e("parse casewhenexprlist");
	astnode n = newNode(N_CWEXPRLIST);
	n->node1 = parseCaseWhenExpr();
	t = sc.currToken();
	if (t == "when")
		n->node2 = parseCaseWhenExprList();
	return n;
}

//node1 is comparison expression
//node2 is result expression
//tok1.id will be comparision datatype
astnode parser::parseCaseWhenExpr() {
	t = sc.currToken();
	e("parse casewhenexpr");
	astnode n = newNode(N_CWEXPR);
	sc.nextToken(); //eat when token
	n->node1 = parseExprAdd();
	sc.nextToken(); //eat then token
	n->node2 = parseExprAdd();
	return n;
}

//row limit at front of query
void parser::parseQuantifier(astnode& n) {
	t = sc.currToken();
	e("parse top");
	if (t == "top") {
		t = sc.nextToken();
		if (!is_number(t.val)) error("Expected number after 'top'. Found '",t,"'");
		n->tok1.id = atoi(t.val.c_str());
		sc.nextToken();
		parseQuantifier(n);
		return;
	}
	if (t == "distinct") {
		q->distinctFiltering = true;
		sc.nextToken();
		parseQuantifier(n);
		return;
	}
}

//row limit at end of query
void parser::parseLimit(astnode& n) {
	t = sc.currToken();
	e("parse limit");
	if (t == "limit") {
		t = sc.nextToken();
		if (!is_number(t.val)) error("Expected number after 'limit'. Found '",t,"'");
		n->tok1.id = atoi(t.val.c_str());
		sc.nextToken();
	}
}

//tok1 is file path
//tok4 is alias
//node1 is file
//node2 is joins
astnode parser::parseFrom(bool withselections) {
	t = sc.currToken();
	e("parse from");

	if (!withselections){
		//will be 'select' if using selections, otherwise filepath
		if (t != WORD_TK && t != SLASH){
			if (t != "select")
				error("Expected 'select' or file name. Found: '",t,"'");
			return nullptr;
		}
		//more helpful error messages when query assumes an explicit 'select' statement
		if (!boost::filesystem::exists(t.val) && !boost::filesystem::exists(
				st(globalSettings.configdir,SLASH,"alias-",t,".txt"))){
			return nullptr;
		}
		justfiles.top() = true;
	} else {
		if (t != "from") error("Expected 'from'. Found: '",t,"'");
		t = sc.nextToken();
	}

	astnode n = newNode(N_FROM);
	n->node1 = parseFile(false);
	n->node2 = parseJoin();
	return n;
}

//tok1 is file path or view name
//tok2 will be copy of tok1 if filealias
//tok3.id is file options
//tok4 is alias
astnode parser::parseFile(bool join) {
	t = sc.fileToken();
	e("parse file");
	astnode n = newNode(N_FILE);
	bool isfile = false;
	//subquery
	if (t.val[0] == '('){
		error("Subquery as a file is not implemented yet");
	//file or view
	} else if (t == WORD_TK){
		isfile = true;
		boost::replace_first(t.val, st("~",SLASH), gethome()+SLASH);
		n->tok1 = t;
		sc.nextToken();
		parseFileOptions(n);
	} else {
		error("Expected file. Found: '",t,"'");
	}
	//alias
	t = sc.currToken();
	if (t == "as"){
		t = sc.nextToken();
		if (t != WORD_TK) error("Expected alias after as. Found: '",t,"'");
	} else if (t == WORD_TK){
		if (!(joinMap.count(t.lower()) || (join && t == "on"))){
			n->tok4 = t;
			sc.nextToken();
		}
	}
	//fileoptions can be before or after alias
	if (isfile)
		parseFileOptions(n);
	return n;
}

//tok3.id of arg node is file options
void parser::parseFileOptions(astnode& n) {
	t = sc.currToken();
	e("parse fileoptions");
	int& opts = n->optionbits();
	if (t == "nh") {
		if (opts & (O_H|O_AH)) error("Cannot have multiple input header options");
		opts |= O_NH;
	} else if (t == "h") {
		if (opts & (O_NH|O_AH)) error("Cannot have multiple input header options");
		opts |= O_H;
	} else if (t == "ah") {
		if (opts & (O_NH|O_H)) error("Cannot have multiple input header options");
		opts |= O_AH;
	} else if (t == "s") {
		if (opts & (O_P|O_T|O_SC)) error("Cannot have multiple delimiter options");
		opts |= O_S;
	} else if (t == "p") {
		if (opts & (O_S|O_T|O_SC)) error("Cannot have multiple delimiter options");
		opts |= O_P;
	} else if (t == "t") {
		if (opts & (O_P|O_S|O_SC)) error("Cannot have multiple delimiter options");
		opts |= O_T;
	} else if (t == "sc") {
		if (opts & (O_P|O_S|O_T)) error("Cannot have multiple delimiter options");
		opts |= O_SC;
	} else {
		return;
	}
	perr ( "SET OPTION " + t.val);
	sc.nextToken();
	parseFileOptions(n);
}

//tok2 is join token (join,sjoin,bjoin)
//tok3 is join details (left/inner/cross)
//node1 is file
//node2 is join condition (predicates)
//node3 is next join
astnode parser::parseJoin() {
	t = sc.currToken();
	e("parse join");
	if (joinMap.count(t.lower()) == 0)
		return nullptr;
	astnode n = newNode(N_JOIN);
	if (t == "left" || t == "inner"){
		n->tok3 = t;
		sc.nextToken();
	}
	t = sc.currToken();
	if (t == "join"){
		n->tok2 = t;
		t = sc.nextToken();
	} else {
		error("Expected 'join'. Found:",sc.currToken());
	}
	n->node1 = parseFile(true);
	t = sc.currToken();
	if (t != "on") error("Expected 'on'. Found: '",t,"'");
	sc.nextToken();
	n->node2 = parsePredicates();
	n->node3 = parseJoin();
	return n;
}

//node1 is conditions
astnode parser::parseWhere() {
	t = sc.currToken();
	e("parse where");
	if (t != "where") { return nullptr; }
	astnode n = newNode(N_WHERE);
	sc.nextToken();
	n->node1 = parsePredicates();
	return n;
}

//node1 is conditions
astnode parser::parseHaving() {
	t = sc.currToken();
	e("parse having");
	if (t != "having") { return nullptr; }
	astnode n = newNode(N_HAVING);
	sc.nextToken();
	n->node1 = parsePredicates();
	return n;
}

//node1 is sort expr list
astnode parser::parseOrder() {
	t = sc.currToken();
	e("parse order");
	if (t == "order") {
		if (sc.nextToken() != "by") error("Expected 'by' after 'order'. Found '",sc.currToken(),"'");
		sc.nextToken();
		astnode n = newNode(N_ORDER);
		n->node1 = parseExpressionList(false, true);
		return n;
	}
	return nullptr;
}

//tok1 is function id
//tok2 is * for count(*), preconv paramtype for len(), rounding dec places, date format
//tok3 is distinct, minus sign for rounding param
//tok4 is password or (determined later) count of distinct N or S functions
//[PARAMTYPE] is original paramtype for type conversion
//[RETTYPE] is inflexible return type
//[MIDIDX] is midrow index
//node1 is expression in parens
astnode parser::parseFunction() {
	t = sc.currToken();
	e("parse function");
	astnode n = newNode(N_FUNCTION);
	n->tok1 = t;
	n->tok1.id = getfunc(t.lower());
	sc.nextToken();
	t = sc.nextToken();
	//count(*)
	if (t == SP_STAR && n->tok1 == "count") {
		n->tok2 = t;
		sc.nextToken();
	//everything else
	} else {
		if (t == "distinct") {
			n->tok3 = t;
			t = sc.nextToken();
		}
		switch (n->tok1.id) {
		case FN_COALESCE:
			n->node1 = parseExpressionList(true, false);
			break;
		case FN_ENCRYPT:
		case FN_DECRYPT:
			//first param is expression to en/decrypt
			n->node1 = parseExprAdd();
			if (sc.currToken() == SP_COMMA) {
				//second param is password
				t = sc.nextToken();
				n->tok4.val = t.val;
				sc.nextToken();
			} else if (sc.currToken() != SP_RPAREN) {
				error("Expect closing parenthesis or comma after expression in crypto function. Found: '",sc.currToken(),"'");
			}
			break;

		case FN_ROUND:
		case FN_FLOOR:
		case FN_CEIL:
			n->node1 = parseExprAdd();
			if (t = sc.currToken(); t == SP_COMMA){
				t = sc.nextToken();
				if (t == SP_MINUS){
					n->tok3 = t;
					t = sc.nextToken();
				}
				n->tok2 = t;
				sc.nextToken();
			}
			break;
		case FN_POW:
			n->node1 = parseExprAdd();
			if (t = sc.currToken(); t != SP_COMMA)
				error("POW function expected comma before second expression. Found: '",t,"'");
			sc.nextToken();
			n->node2 = parseExprAdd();
			break;
		case FN_FORMAT:
			n->node1 = parseExprAdd();
			if (t = sc.currToken(); t != SP_COMMA)
				error("FORMAT function expected comma before date format parameter. Found: '",t,"'");
			n->tok2 = sc.nextToken();
			sc.nextToken();
			break;
		case FN_SUBSTR:
			n->node1 = parseExprAdd();
			if (t = sc.currToken(); t != SP_COMMA)
				error("SUBSTR function expected comma before second parameter. Found: '",t,"'");
			n->tok2 = sc.nextToken();
			if (t = sc.nextToken(); t == SP_COMMA){
				n->tok3 = sc.nextToken();
				sc.nextToken();
			}
			break;
		case FN_INC:
		case FN_RAND:
		case FN_NOW:
		case FN_NOWGM:
			break;
		default:
			n->node1 = parseExprAdd();
		}
	}
	if (sc.currToken() != SP_RPAREN) error("Expected closing parenthesis after function. Found: '",sc.currToken().val,"'");
	sc.nextToken();
	return n;
}

//node1 is groupExpressions
astnode parser::parseGroupby() {
	t = sc.currToken();
	e("parse groupby");
	if (!(t == "group" && sc.peekToken() == "by")) { return nullptr; }
	astnode n = newNode(N_GROUPBY);
	sc.nextToken();
	sc.nextToken();
	n->node1 = parseExpressionList(false,false);
	return n;
}

//node1 is expression
//node2 is expressionlist
//tok1 id desc for sorting lists
//tok2.id will be sort list size, initially set to 1
//tok3.id will be destrow index for aggregate sort values
//	or sequence number for non-agg sort values
astnode parser::parseExpressionList(bool interdependant, bool sortlist) { //bool arg if expression types are interdependant
	t = sc.currToken();
	e("parse expressionlist");
	int label = N_EXPRESSIONS;
	if (interdependant) label = N_DEXPRESSIONS;
	astnode n = newNode(label);
	n->node1 = parseExprAdd();
	t = sc.currToken();
	if (sortlist && (t == "asc" || t == "desc")){
		n->tok1.id = t == "desc";
		t = sc.nextToken();
	}
	n->tok2.id = sortlist;
	switch (t.id){
	case SP_COMMA: sc.nextToken();
	case SP_LPAREN:
	case KW_CASE:
	case WORD_TK: break;
	default: return n;
	}
	n->node2 = parseExpressionList(interdependant, sortlist);
	return n;
}
