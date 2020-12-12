#include "interpretor.h"
#include <boost/algorithm/string/replace.hpp>

/*
{} = optional
[] = one of
[[]] = at least one of

query             -> <preselect> <select> <from> <afterfrom> | <addalias>
addalias          -> add filealias <file>
preselect         -> <options> <with> | ε
options           -> [ oh noh nh h ah s p t nan ] { <options> } | ε
with              -> with <vars> | ε
vars              -> <expradd> as alias { [ , and ] vars }
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
predicateCompare  -> { not } <expradd> { not } <relop> <expradd>
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
*/

#define newNode(l) make_unique<node>(l)

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
	astnode parseFile();
	astnode parseSetList(bool);
	astnode parseExpressionList(bool i, bool s);
	astnode parseQuery();
	astnode parseAddAlias();
	void parseFileOptions(astnode&);
	void parseTop(astnode&);
	void parseLimit(astnode&);

	querySpecs* q;
	bool justfile = false;
	public:
	parser(querySpecs &qs): q{&qs} {}
	void parse(){
		q->tree = parseQuery();
		auto t = q->tok();
		switch (t.id){
			case EOS:
			case SP_RPAREN:
			break;
			default:
				error("Unexpected token at and of query: ",t.val);
		}
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
//node1 is addalias
//tok1.id is N_ADDALIAS
astnode parser::parseQuery() {
	token t = q->tok();
	astnode n = newNode(N_QUERY);
	if (t.lower() == "add"){
		q->nextTok();
		n->node1 = parseAddAlias();
		n->tok1.id = N_ADDALIAS;
		return n;
	}
	n->node1 = parsePreSelect();
	n->node3 = parseFrom(false);
	n->node2 = parseSelect();
	if (!justfile)
		n->node3 = parseFrom(true);
	n->node4 = parseAfterFrom();
	return n;
}

//node1 is file
//tok1 is alias
astnode parser::parseAddAlias() {
	token t = q->tok();
	astnode n = newNode(N_ADDALIAS);
	if (t.id != WORD_TK)
		error("Expected file alias after 'add'. Found ",t.val);
	n->tok1 = t;
	q->nextTok();
	n->node1 = parseFile();
	return n;
}

//node1 is with
//tok1.id is options
astnode parser::parsePreSelect() {
	token t = q->tok();
	astnode n = newNode(N_PRESELECT);
	parseOptions(n);
	n->node1 = parseWith();
	return n;
}

//anything that comes after 'from', in any order
//tok1.id is quantitylimit
astnode parser::parseAfterFrom() {
	token t = q->tok();
	astnode n = newNode(N_AFTERFROM);
	for (;;){
		t = q->tok();
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

void parser::parseOptions(astnode& n) {
	token t = q->tok();
	string s = t.lower();
	if (s == "c") {
		n->tok1.id |= O_C;
	} else if (s == "oh") {
		if (n->tok1.id & O_NOH) error("Cannot mix output header options");
		n->tok1.id |= O_OH;
	} else if (s == "noh") {
		if (n->tok1.id & O_NOH) error("Cannot mix output header options");
		n->tok1.id |= O_NOH;
	} else if (s == "nh") {
		if (n->tok1.id & (O_H|O_AH)) error("Cannot mix input header options");
		n->tok1.id |= O_NH;
	} else if (s == "h") {
		if (n->tok1.id & (O_NH|O_AH)) error("Cannot mix input header options");
		n->tok1.id |= O_H;
	} else if (s == "ah") {
		if (n->tok1.id & (O_NH|O_H)) error("Cannot mix input header options");
		n->tok1.id |= O_AH;
	} else if (s == "s") {
		if (n->tok1.id & (O_P|O_T)) error("Cannot mix delimiter options");
		n->tok1.id |= O_S;
	} else if (s == "p") {
		if (n->tok1.id & (O_S|O_T)) error("Cannot mix delimiter options");
		n->tok1.id |= O_P;
	} else if (s == "t") {
		if (n->tok1.id & (O_P|O_S)) error("Cannot mix delimiter options");
		n->tok1.id |= O_T;
	} else if (s == "nan") {
		n->tok1.id |= O_NAN;
	} else {
		return;
	}
	q->nextTok();
	parseOptions(n);
}

//node1 is vars
astnode parser::parseWith() {
	token t = q->tok();
	if (t.lower() != "with") { return nullptr; }
	astnode n = newNode(N_WITH);
	q->nextTok();
	n->node1 = parseVars();
	return n;
}

//node1 is expr
//tok1 is alias
//tok3 is midrow index-1 of phase2 non-aggs
//node2 is vars
astnode parser::parseVars() {
	token t = q->tok();
	astnode n = newNode(N_VARS);
	n->node1 = parseExprAdd();
	t = q->tok();
	if (t.lower() != "as") error("Expected 'as' after expression. Found ",t.val);
	t = q->nextTok();
	if (t.id != WORD_TK) error("Expected variable name. Found ",t.val);
	n->tok1 = t;
	t = q->nextTok();
	if (t.lower() == "and" || t.id == SP_COMMA) {
		q->nextTok();
		n->node2 = parseVars();
	}
	return n;
}

//node2 is selections (same as N_SELECTIONS)
//tok1.id is quantityLimit
astnode parser::parseSelect() {
	if (justfile) return nullptr;
	token t = q->tok();
	astnode n = newNode(N_SELECT);
	if (t.lower() != "select") error("Expected 'select'. Found "+t.val);
	q->nextTok();
	parseTop(n);
	n->node2 = parseSelections();
	return n;
}

//node1 is expression
//node2 is next selection
//tok1 is * or distinct or hidden
//tok2 is alias
//later stages:
//  tok3.id will be midrow index
//  tok4.id will be destrow index
astnode parser::parseSelections() {
	token t = q->tok();
	if (t.lower() == "from"){
		return nullptr;
	}
	astnode n = newNode(N_SELECTIONS);
	if (t.id == SP_COMMA) t = q->nextTok();
	switch (t.id) {
	case SP_STAR:
		n->tok1 = t;
		q->nextTok();
		n->node2 = parseSelections();
		return n;
	case KW_DISTINCT:
		n->tok1 = t;
		t = q->nextTok();
		if (t.lower() == "hidden" && !t.quoted) {
			n->tok1 = t;
			n->tok1.id = KW_DISTINCT;
			t = q->nextTok();
		}
	//expression
	case KW_CASE:
	case WORD_TK:
	case SP_MINUS:
	case SP_LPAREN:
		//alias = expression
		if (q->peekTok().id == SP_EQ) {
			if (t.id != WORD_TK) error("Alias must be a word. Found ",t.val);
			n->tok2 = t;
			q->nextTok();
			q->nextTok();
			n->node1 = parseExprAdd();
		//expression
		} else {
			n->node1 = parseExprAdd();
			t = q->tok();
			if (t.lower() == "as") {
				t = q->nextTok();
				n->tok2 = t;
				q->nextTok();
			}
		}
		if (q->tok().id == SP_LPAREN)
			error("Cannot start expression after '", q->lastTok().val,
						"' with parenthesis. Did you misspell a function or forget a comma between selections?");
		n->node2 = parseSelections();
		return n;
	default:
		error("Expected a new selection or 'from' clause. Found ",t.val);
	}
	return nullptr;
}

//node1 is exprMult
//node2 is exprAdd
//tok1 is add/minus operator
astnode parser::parseExprAdd() {
	token t = q->tok();
	astnode n = newNode(N_EXPRADD);
	n->node1 = parseExprMult();
	t = q->tok();
	switch (t.id) {
	case SP_MINUS:
	case SP_PLUS:
		n->tok1 = t;
		q->nextTok();
		n->node2 = parseExprAdd();
	}
	return n;
}

//node1 is exprNeg
//node2 is exprMult
//tok1 is mult/div operator
astnode parser::parseExprMult() {
	token t = q->tok();
	astnode n = newNode(N_EXPRMULT);
	n->node1 = parseExprNeg();
	t = q->tok();
	switch (t.id) {
	case SP_STAR:
		if (q->peekTok().lower() == "from") { break; }
	case SP_MOD:
	case SP_CARROT:
	case SP_DIV:
		n->tok1 = t;
		q->nextTok();
		n->node2 = parseExprMult();
	}
	return n;
}

//tok1 is minus operator
//node1 is exprCase
astnode parser::parseExprNeg() {
	token t = q->tok();
	astnode n = newNode(N_EXPRNEG);
	if (t.id == SP_MINUS) {
		n->tok1 = t;
		q->nextTok();
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
	token t = q->tok();
	astnode n = newNode(N_EXPRCASE);
	token t2;
	switch (t.id) {
	case KW_CASE:
		n->tok1 = t;

		t2 = q->nextTok();
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
			t = q->tok();
			if (t.lower() != "when") error("Expected 'when' after case expression. Found ",t.val);
			n->node2 = parseCaseWhenExprList();
			break;
		default: error("Expected expression or 'when'. Found ",q->tok().val);
		}

		switch (q->tok().id) {
		case KW_END:
			q->nextTok();
			break;
		case KW_ELSE:
			t = q->nextTok();
			n->node3 = parseExprAdd();
			t = q->tok();
			if (t.lower() != "end") error("Expected 'end' after 'else' expression. Found ",t.val);
			q->nextTok();
			break;
		default:
			error("Expected 'end' or 'else' after case expression. Found ",q->tok().val);
		}
		break;

	case WORD_TK:
		n->tok1 = t;
		n->node1 = parseValue();
		break;

	case SP_LPAREN:
		n->tok1 = t;
		q->nextTok();
		n->node1 = parseExprAdd();
		if (q->tok().id != SP_RPAREN) error("Expected closing parenthesis. Found ",q->tok().val);
		q->nextTok();
		break;
	default: error("Expected case, value, or expression. Found ",q->tok().val);
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
	token t = q->tok();
	astnode n = newNode(N_VALUE);
	n->tok1 = t;
	//see if it's a function
	if (functionMap.count(t.lower()) && !t.quoted && q->peekTok().id==SP_LPAREN) {
		n->tok2.id = FUNCTION;
		n->node1 = parseFunction();
	//any non-function value
	} else {
		q->nextTok();
	}
	return n;
}

//node1 is case predicate
//node2 is next case predicate list node
astnode parser::parseCaseWhenPredList() {
	token t = q->tok();
	astnode n = newNode(N_CPREDLIST);
	n->node1 = parseCasePredicate();
	t = q->tok();
	if (t.lower() == "when") {
		n->node2 = parseCaseWhenPredList();
	}
	return n;
}

//node1 is predicates
//node2 is expression if true
astnode parser::parseCasePredicate() {
	token t = q->tok();
	astnode n = newNode(N_CPRED);
	q->nextTok(); //eat when token
	n->node1 = parsePredicates();
	t = q->tok();
	if (t.lower() != "then") error("Expected 'then' after predicate. Found: ",q->tok().val);
	q->nextTok(); //eat then token
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
	token t = q->tok();
	astnode n = newNode(N_PREDICATES);
	if (t.id == SP_NEGATE) { n->tok2 = t; t = q->nextTok(); }
	n->node1 = parsePredCompare();
	t = q->tok();
	if ((t.id & LOGOP) != 0) {
		n->tok1 = t;
		q->nextTok();
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
//  [VALPOSIDX] is index of valpos vector for joins
//  [ANDCHAIN] is 1 or 2 if part of simple and chain, 2 if not first
astnode parser::parsePredCompare() {
	token t = q->tok();
	astnode n = newNode(N_PREDCOMP);
	int negate = 0;
	if (t.id == SP_NEGATE) { negate ^= 1; t = q->nextTok(); }
	//more predicates in parentheses
	if (t.id == SP_LPAREN) {
		int pos = q->tokIdx;
		//try parsing as predicate
		t = q->nextTok();
		bool tryAgain = false;
		try {
			n->node1 = parsePredicates();
			t = q->tok();
			if (t.id != SP_RPAREN) error("Expected cosing parenthesis. Found: ",t.val);
		//if failed, reparse as expression
		} catch (const std::invalid_argument& e) {
			q->tokIdx = pos;
			tryAgain = true;
		}
		//return if (more comparisions)
		if (!tryAgain) {
			n->tok1.id = SP_LPAREN;
			return n;
		}
	}
	//comparison
	auto nullInList = q->tok().lower() == "null";
	n->node1 = parseExprAdd();
	t = q->tok();
	if (t.id == SP_NEGATE) {
		negate ^= 1;
		t = q->nextTok();
	}
	n->tok2.id = negate;
	if ((t.id & RELOP) == 0) error("Expected relational operator. Found: ",t.val);
	t = q->tok();
	n->tok1 = t;
	t = q->nextTok();
	if (n->tok1.id == KW_LIKE) {
		n->tok3 = t;
		t = q->nextTok();
	} else if (n->tok1.id == KW_IN) {
		if (t.id != SP_LPAREN) error("Expected opening parenthesis for expression list. Found: ",t.val);
		q->nextTok();
		n->node2 = parseSetList(!nullInList);
		t = q->tok();
		if (t.id != SP_RPAREN) error("Expected closing parenthesis after expression list. Found: ",t.val);
		q->nextTok();
	} else {
		n->node2 = parseExprAdd();
	}
	if (n->tok1.id == KW_BETWEEN) {
		q->nextTok();
		n->node3 = parseExprAdd();
	}
	return n;
}

//node1 is expressionlist or query
//tok1.id is 1 if subquery else 0
//tok2.id is index of subquery
astnode parser::parseSetList(bool interdependant) {
	astnode n = newNode(N_SETLIST);

	int pos = q->tokIdx;
	try {
		n->node1 = parseQuery();
		n->tok1.id = 1;
	} catch (exception ex){
		q->tokIdx = pos;
		n->node1 = parseExpressionList(interdependant, false);
	}
	return n;
}

//node1 is case expression
//node2 is next exprlist node
astnode parser::parseCaseWhenExprList() {
	token t = q->tok();
	astnode n = newNode(N_CWEXPRLIST);
	n->node1 = parseCaseWhenExpr();
	t = q->tok();
	if (t.lower() == "when")
		n->node2 = parseCaseWhenExprList();
	return n;
}

//node1 is comparison expression
//node2 is result expression
//tok1.id will be comparision datatype
astnode parser::parseCaseWhenExpr() {
	token t = q->tok();
	astnode n = newNode(N_CWEXPR);
	q->nextTok(); //eat when token
	n->node1 = parseExprAdd();
	q->nextTok(); //eat then token
	n->node2 = parseExprAdd();
	return n;
}

//row limit at front of query
void parser::parseTop(astnode& n) {
	token t = q->tok();
	if (t.lower() == "top") {
		t = q->nextTok();
		if (!is_number(t.val)) error("Expected number after 'top'. Found ",t.val);
		n->tok1.id = atoi(t.val.c_str());
		q->nextTok();
	}
}

//row limit at end of query
void parser::parseLimit(astnode& n) {
	token t = q->tok();
	if (t.lower() == "limit") {
		t = q->nextTok();
		if (!is_number(t.val)) error("Expected number after 'limit'. Found ",t.val);
		n->tok1.id = atoi(t.val.c_str());
		q->nextTok();
	}
}

//tok1 is file path
//tok4 is alias
//tok5 is noheader
//node1 is file
//node2 is joins
astnode parser::parseFrom(bool withselections) {
	token t = q->tok();

	if (!withselections){
		//will be unquoted 'select' if using selections, otherwise filepath
		if (!t.quoted)
			return nullptr;
		justfile = true;
	} else {
		if (t.lower() != "from") error("Expected 'from'. Found: ",t.val);
		t = q->nextTok();
	}

	astnode n = newNode(N_FROM);
	n->node1 = parseFile();
	n->node2 = parseJoin();
	return n;
}

//tok1 is file path or view name
//tok4 is alias
//tok5.id is file options
astnode parser::parseFile() {
	token t = q->tok();
	astnode n = newNode(N_FILE);
	bool isfile = false;
	//subquery
	if (t.id == SP_LPAREN){
		error("Subquery as a file is not implemented yet");
	//file or view
	} else if (t.id == WORD_TK){
		isfile = true;
		boost::replace_first(t.val, "~/", gethome()+"/");
		n->tok1 = t;
		q->nextTok();
		parseFileOptions(n);
	} else {
		error("Expected file. Found: ",t.val);
	}
	//alias
	t = q->tok();
	switch (t.id) {
	case KW_AS:
		t = q->nextTok();
		if (t.id != WORD_TK) error("Expected alias after as. Found: ",t.val);
	case WORD_TK:
		if (joinMap.count(t.lower())) break;
		n->tok4 = t;
		q->nextTok();
	}
	//fileoptions can be before or after alias
	if (isfile)
		parseFileOptions(n);
	return n;
}

//tok5.id of arg node is file options
void parser::parseFileOptions(astnode& n) {
	token t = q->tok();
	string s = t.lower();
	if (s == "nh") {
		if (n->tok5.id & (O_H|O_AH)) error("Cannot mix input header options");
		n->tok5.id |= O_NH;
	} else if (s == "h") {
		if (n->tok5.id & (O_NH|O_AH)) error("Cannot mix input header options");
		n->tok5.id |= O_H;
	} else if (s == "ah") {
		if (n->tok5.id & (O_NH|O_H)) error("Cannot mix input header options");
		n->tok5.id |= O_AH;
	} else if (s == "s") {
		if (n->tok5.id & (O_P|O_T)) error("Cannot mix delimiter options");
		n->tok5.id |= O_S;
	} else if (s == "p") {
		if (n->tok5.id & (O_S|O_T)) error("Cannot mix delimiter options");
		n->tok5.id |= O_P;
	} else if (s == "t") {
		if (n->tok5.id & (O_P|O_S)) error("Cannot mix delimiter options");
		n->tok5.id |= O_T;
	} else {
		return;
	}
	perr ( "SET OPTION " + s);
	q->nextTok();
	parseFileOptions(n);
}

//tok1 is filepath
//tok2 is join token (join,sjoin,bjoin)
//tok3 is join details (left/outer or inner)
//tok4 is alias
//tok5 is noheader
//node1 is file
//node2 is join condition (predicates)
//node3 is next join
astnode parser::parseJoin() {
	token t = q->tok();
	string s = t.lower();
	if (joinMap.count(s) == 0)
		return nullptr;
	astnode n = newNode(N_JOIN);
	if (s == "left" || s == "inner"){
		n->tok3 = t;
		q->nextTok();
	}
	t = q->tok();
	s = t.lower();
	if (s == "join"){
		n->tok2 = t;
		t = q->nextTok();
	} else {
		error("Expected 'join'. Found:",q->tok().val);
	}
	n->node1 = parseFile();
	if (n->node1->tok4.id == 0)
		error("Joined file requires an alias.");
	t = q->tok();
	if (t.lower() != "on") error("Expected 'on'. Found: ",t.val);
	q->nextTok();
	n->node2 = parsePredicates();
	n->node3 = parseJoin();
	return n;
}

//node1 is conditions
astnode parser::parseWhere() {
	token t = q->tok();
	if (t.lower() != "where") { return nullptr; }
	astnode n = newNode(N_WHERE);
	q->nextTok();
	n->node1 = parsePredicates();
	return n;
}

//node1 is conditions
astnode parser::parseHaving() {
	token t = q->tok();
	if (t.lower() != "having") { return nullptr; }
	astnode n = newNode(N_HAVING);
	q->nextTok();
	n->node1 = parsePredicates();
	return n;
}

//old: node1 is sort expr
//new: node1 is sort expr list
//tok1 is asc
astnode parser::parseOrder() {
	token t = q->tok();
	if (t.lower() == "order") {
		if (q->nextTok().lower() != "by") error("Expected 'by' after 'order'. Found ",q->tok().val);
		q->nextTok();
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
//[PARAMTYPE] is paramtype for type conversion
//[RETTYPE] is inflexible return type
//[MIDIDX] is midrow index
//node1 is expression in parens
astnode parser::parseFunction() {
	token t = q->tok();
	astnode n = newNode(N_FUNCTION);
	n->tok1 = t;
	n->tok1.id = getfunc(t.lower());
	q->nextTok(); // (
	t = q->nextTok();
	//count(*)
	if (t.id == SP_STAR && n->tok1.lower() == "count") {
		n->tok2 = t;
		q->nextTok();
	//everything else
	} else {
		if (t.lower() == "distinct") {
			n->tok3 = t;
			t = q->nextTok();
		}
		switch (n->tok1.id) {
		case FN_COALESCE:
			n->node1 = parseExpressionList(true, false);
			break;
		case FN_ENCRYPT:
		case FN_DECRYPT:
			//first param is expression to en/decrypt
			n->node1 = parseExprAdd();
			if (q->tok().id == SP_COMMA) {
				//second param is password
				t = q->nextTok();
				n->tok4.val = t.val;
				q->nextTok();
			} else if (q->tok().id != SP_RPAREN) {
				error("Expect closing parenthesis or comma after expression in crypto function. Found: ",q->tok().val);
			}
			break;

		case FN_ROUND:
		case FN_FLOOR:
		case FN_CEIL:
			n->node1 = parseExprAdd();
			if (t = q->tok(); t.id == SP_COMMA){
				t = q->nextTok();
				if (t.id == SP_MINUS){
					n->tok3 = t;
					t = q->nextTok();
				}
				n->tok2 = t;
				q->nextTok();
			}
			break;
		case FN_POW:
			n->node1 = parseExprAdd();
			if (t = q->tok(); t.id != SP_COMMA)
				error("POW function expected comma before second expression. Found: ", t.val);
			q->nextTok();
			n->node2 = parseExprAdd();
			break;
		case FN_FORMAT:
			n->node1 = parseExprAdd();
			if (t = q->tok(); t.id != SP_COMMA)
				error("FORMAT function expected comma before date format parameter. Found: ", t.val);
			n->tok2 = q->nextTok();
			q->nextTok();
			break;
		case FN_SUBSTR:
			n->node1 = parseExprAdd();
			if (t = q->tok(); t.id != SP_COMMA)
				error("SUBSTR function expected comma before second parameter. Found: ", t.val);
			n->tok2 = q->nextTok();
			if (t = q->nextTok(); t.id == SP_COMMA){
				n->tok3 = q->nextTok();
				q->nextTok();
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
	if (q->tok().id != SP_RPAREN) error("Expected closing parenthesis after function. Found: ",q->tok().val);
	q->nextTok();
	return n;
}

//node1 is groupExpressions
astnode parser::parseGroupby() {
	token t = q->tok();
	if (!(t.lower() == "group" && q->peekTok().lower() == "by")) { return nullptr; }
	astnode n = newNode(N_GROUPBY);
	q->nextTok();
	q->nextTok();
	n->node1 = parseExpressionList(false,false);
	return n;
}

//node1 is expression
//node2 is expressionlist
//tok1 id asc for sorting lists
//tok2.id will be sort list size, initially set to 1
//tok3.id will be destrow index for aggregate sort values
//	or sequence number for non-agg sort values
astnode parser::parseExpressionList(bool interdependant, bool sortlist) { //bool arg if expression types are interdependant
	token t = q->tok();
	int label = N_EXPRESSIONS;
	if (interdependant) label = N_DEXPRESSIONS;
	astnode n = newNode(label);
	n->node1 = parseExprAdd();
	t = q->tok();
	if (sortlist && (t.lower() == "asc" || t.lower() == "desc")){
		n->tok1.id = t.lower() == "asc";
		t = q->nextTok();
	}
	n->tok2.id = sortlist;
	switch (t.id){
	case SP_COMMA: q->nextTok();
	case SP_LPAREN:
	case KW_CASE:
	case WORD_TK: break;
	default: return n;
	}
	n->node2 = parseExpressionList(interdependant, sortlist);
	return n;
}
