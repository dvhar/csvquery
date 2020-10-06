#include "interpretor.h"
#include <boost/algorithm/string/replace.hpp>

#define newNode(l) make_unique<node>(l)

class parser {
	void parseOptions();
	unique_ptr<node> parsePreSelect();
	unique_ptr<node> parseWith();
	unique_ptr<node> parseVars();
	unique_ptr<node> parseSelect();
	unique_ptr<node> parseSelections();
	unique_ptr<node> parseExprAdd();
	unique_ptr<node> parseExprMult();
	unique_ptr<node> parseExprNeg();
	unique_ptr<node> parseExprCase();
	unique_ptr<node> parseCaseWhenPredList();
	unique_ptr<node> parseCaseWhenExprList();
	unique_ptr<node> parseCaseWhenExpr();
	unique_ptr<node> parseCasePredicate();
	unique_ptr<node> parsePredicates();
	unique_ptr<node> parsePredCompare();
	unique_ptr<node> parseJoin();
	unique_ptr<node> parseOrder();
	unique_ptr<node> parseHaving();
	unique_ptr<node> parseGroupby();
	unique_ptr<node> parseFrom();
	unique_ptr<node> parseWhere();
	unique_ptr<node> parseValue();
	unique_ptr<node> parseFunction();
	unique_ptr<node> parseAfterFrom();
	unique_ptr<node> parseExpressionList(bool i, bool s);
	void parseTop();
	void parseLimit();

	querySpecs* q;
	public:
	parser(querySpecs &qs): q{&qs} {}
	void parse(){
		q->tree = newNode(N_QUERY);
		q->tree->node1 = parsePreSelect();
		q->tree->node2 = parseSelect();
		q->tree->node3 = parseFrom();
		q->tree->node4 = parseAfterFrom();
		parseLimit();
	}
};

void node::print(){
	perr(st(treeMap.at(label) , '\n'));
	tok1.print();
	tok2.print();
	tok3.print();
	tok4.print();
	tok5.print();
}

//run recursive descent parser for query
void parseQuery(querySpecs &q) {
	parser pr(q);
	pr.parse();
}

//could include other commands like describe
//node1 is with
unique_ptr<node> parser::parsePreSelect() {
	token t = q->tok();
	parseOptions();
	unique_ptr<node> n = newNode(N_PRESELECT);
	n->node1 = parseWith();
	return n;
}

//anything that comes after 'from', in any order
unique_ptr<node> parser::parseAfterFrom() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_AFTERFROM);
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
		default:
			goto done;
		}
	}
	done:
	return n;
}

void parser::parseOptions() {
	token t = q->tok();
	string s = t.lower();
	if (s == "c"sv) {
		q->options |= O_C;
	} else if (s == "nh"sv || s == "noheader"sv) {
		q->options |= O_NH;
	} else if (s == "h"sv || s == "header"sv) {
		q->options |= O_H;
	} else if (s == "s"sv) {
		q->options |= O_S;
	} else {
		return;
	}
	q->nextTok();
	parseOptions();
}

//node1 is vars
unique_ptr<node> parser::parseWith() {
	token t = q->tok();
	if (t.lower() != "with"sv) { return nullptr; }
	unique_ptr<node> n = newNode(N_WITH);
	q->nextTok();
	n->node1 = parseVars();
	return n;
}

//node1 is expr
//tok1 is alias
//tok3 is midrow index-1 of phase2 non-aggs
//node2 is vars
unique_ptr<node> parser::parseVars() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_VARS);
	n->node1 = parseExprAdd();
	t = q->tok();
	if (t.lower() != "as"sv) error("Expected 'as' after expression. Found "+t.val);
	t = q->nextTok();
	if (t.id != WORD_TK) error("Expected variable name. Found "+t.val);
	n->tok1 = t;
	t = q->nextTok();
	if (t.lower() == "and"sv || t.id == SP_COMMA) {
		q->nextTok();
		n->node2 = parseVars();
	}
	return n;
}

//node1 is selections
unique_ptr<node> parser::parseSelect() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_SELECT);
	if (t.lower() != "select"sv) error("Expected 'select'. Found "+t.val);
	q->nextTok();
	parseTop();
	n->node1 = parseSelections();
	return n;
}

//node1 is expression
//node2 is next selection
//tok1 is * or distinct or hidden
//tok2 is alias
//later stages:
//  tok3.id will be midrow index
//  tok4.id will be destrow index
unique_ptr<node> parser::parseSelections() {
	token t = q->tok();
	if (t.lower() == "from"sv){
		return nullptr;
	}
	unique_ptr<node> n = newNode(N_SELECTIONS);
	if (t.id == SP_COMMA) t = q->nextTok();
	switch (t.id) {
	case SP_STAR:
		n->tok1 = t;
		q->nextTok();
		n->node2 = parseSelections();
		return n;
	case KW_DISTINCT:
		n->tok1 = t;
		q->distinctFiltering = true;
		t = q->nextTok();
		if (t.lower() == "hidden"sv && !t.quoted) {
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
			if (t.id != WORD_TK) error("Alias must be a word. Found "+t.val);
			n->tok2 = t;
			q->nextTok();
			q->nextTok();
			n->node1 = parseExprAdd();
		//expression
		} else {
			n->node1 = parseExprAdd();
			t = q->tok();
			if (t.lower() == "as"sv) {
				t = q->nextTok();
				n->tok2 = t;
				q->nextTok();
			}
		}
		n->node2 = parseSelections();
		return n;
	default:
		error("Expected a new selection or 'from' clause. Found "+t.val);
	}
	return nullptr;
}

//node1 is exprMult
//node2 is exprAdd
//tok1 is add/minus operator
unique_ptr<node> parser::parseExprAdd() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_EXPRADD);
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
unique_ptr<node> parser::parseExprMult() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_EXPRMULT);
	n->node1 = parseExprNeg();
	t = q->tok();
	switch (t.id) {
	case SP_STAR:
		if (q->peekTok().lower() == "from"sv) { break; }
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
unique_ptr<node> parser::parseExprNeg() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_EXPRNEG);
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
unique_ptr<node> parser::parseExprCase() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_EXPRCASE);
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
			if (t.lower() != "when"sv) error("Expected 'when' after case expression. Found "+t.val);
			n->node2 = parseCaseWhenExprList();
			break;
		default: error("Expected expression or 'when'. Found "+q->tok().val);
		}

		switch (q->tok().id) {
		case KW_END:
			q->nextTok();
			break;
		case KW_ELSE:
			t = q->nextTok();
			n->node3 = parseExprAdd();
			t = q->tok();
			if (t.lower() != "end"sv) error("Expected 'end' after 'else' expression. Found "+t.val);
			q->nextTok();
			break;
		default:
			error("Expected 'end' or 'else' after case expression. Found "+q->tok().val);
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
		if (q->tok().id != SP_RPAREN) error("Expected closing parenthesis. Found "+q->tok().val);
		q->nextTok();
		break;
	default: error("Expected case, value, or expression. Found "+q->tok().val);
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
unique_ptr<node> parser::parseValue() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_VALUE);
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
unique_ptr<node> parser::parseCaseWhenPredList() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_CPREDLIST);
	n->node1 = parseCasePredicate();
	t = q->tok();
	if (t.lower() == "when"sv) {
		n->node2 = parseCaseWhenPredList();
	}
	return n;
}

//node1 is predicates
//node2 is expression if true
unique_ptr<node> parser::parseCasePredicate() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_CPRED);
	q->nextTok(); //eat when token
	n->node1 = parsePredicates();
	t = q->tok();
	if (t.lower() != "then"sv) error("Expected 'then' after predicate. Found: "+q->tok().val);
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
unique_ptr<node> parser::parsePredicates() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_PREDICATES);
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
//node2 is second expr
//node3 is third expr for betweens
//later:
//  [TOSCAN] will be number of node (1,2) for indexable join value
//  [VALPOSIDX] is index of valpos vector for joins
//  [ANDCHAIN] is 1 or 2 if part of simple and chain, 2 if not first
unique_ptr<node> parser::parsePredCompare() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_PREDCOMP);
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
			if (t.id != SP_RPAREN) error("Expected cosing parenthesis. Found: "+t.val);
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
	n->node1 = parseExprAdd();
	t = q->tok();
	if (t.id == SP_NEGATE) {
		negate ^= 1;
		t = q->nextTok();
	}
	n->tok2.id = negate;
	if ((t.id & RELOP) == 0) error("Expected relational operator. Found: "+t.val);
	t = q->tok();
	n->tok1 = t;
	t = q->nextTok();
	if (n->tok1.id == KW_LIKE) {
		n->tok3 = t;
		t = q->nextTok();
	} else if (n->tok1.id == KW_IN) {
		if (t.id != SP_LPAREN) error("Expected opening parenthesis for expression list. Found: "+t.val);
		q->nextTok();
		n->node2 = parseExpressionList(true,false);
		t = q->tok();
		if (t.id != SP_RPAREN) error("Expected closing parenthesis after expression list. Found: "+t.val);
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

//node1 is case expression
//node2 is next exprlist node
unique_ptr<node> parser::parseCaseWhenExprList() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_CWEXPRLIST);
	n->node1 = parseCaseWhenExpr();
	t = q->tok();
	if (t.lower() == "when"sv)
		n->node2 = parseCaseWhenExprList();
	return n;
}

//node1 is comparison expression
//node2 is result expression
//tok1.id will be comparision datatype
unique_ptr<node> parser::parseCaseWhenExpr() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_CWEXPR);
	q->nextTok(); //eat when token
	n->node1 = parseExprAdd();
	q->nextTok(); //eat then token
	n->node2 = parseExprAdd();
	return n;
}

//row limit at front of query
void parser::parseTop() {
	token t = q->tok();
	if (t.lower() == "top"sv) {
		t = q->nextTok();
		if (!is_number(t.val)) error("Expected number after 'top'. Found "+t.val);
		q->quantityLimit = atoi(t.val.c_str());
		q->nextTok();
	}
}

//row limit at end of query
void parser::parseLimit() {
	token t = q->tok();
	if (t.lower() == "limit"sv) {
		t = q->nextTok();
		if (!is_number(t.val)) error("Expected number after 'limit'. Found "+t.val);
		q->quantityLimit = atoi(t.val.c_str());
		q->nextTok();
	}
}

//tok1 is file path
//tok4 is alias
//tok5 is noheader
//node1 is joins
unique_ptr<node> parser::parseFrom() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_FROM);
	if (t.lower() != "from"sv) error("Expected 'from'. Found: "+t.val);
	t = q->nextTok();
	n->tok1.val = boost::replace_first_copy(t.val, "~/", string(getenv("HOME"))+"/");
	t = q->nextTok();
	string s = t.lower();
	if (s == "noheader"sv || s == "nh"sv || s == "header"sv || s == "h") {
		n->tok5 = t;
		t = q->nextTok();
	}
	switch (t.id) {
	case KW_AS:
		t = q->nextTok();
		if (t.id != WORD_TK) error("Expected alias after as. Found: "+t.val);
	case WORD_TK:
		if (joinMap.count(t.lower())) break;
		n->tok4 = t;
		q->nextTok();
	}
	t = q->tok();
	s = t.lower();
	if (s == "noheader"sv || s == "nh"sv || s == "header"sv || s == "h"sv) {
		n->tok5 = t;
		q->nextTok();
	}
	n->node1 = parseJoin();
	return n;
}

//tok1 is filepath
//tok2 is join token (join,sjoin,bjoin)
//tok3 is join details (left/outer or inner)
//tok4 is alias
//tok5 is noheader
//node1 is join condition (predicates)
//node2 is next join
unique_ptr<node> parser::parseJoin() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_JOIN);
	string s = t.lower();
	if (joinMap.count(s) == 0)
		return nullptr;
	q->joining = true;
	if (s == "left"sv || s == "inner"sv){
		n->tok3 = t;
		q->nextTok();
	}
	t = q->tok();
	s = t.lower();
	if (s == "join"sv ){
		n->tok2 = t;
		t = q->nextTok();
	} else {
		error("Expected 'join'. Found:"+q->tok().val);
	}
	//file path
	boost::replace_first(t.val, "~/", st(getenv("HOME"),"/"));
	n->tok1 = t;
	//1st after filepath
	t = q->nextTok();
	s = t.lower();
	if (s == "noheader"sv || s == "nh"sv || s == "header"sv || s == "h"sv) {
		n->tok5 = t;
		t = q->nextTok();
	}
	//alias
	switch (t.id) {
	case KW_AS:
		t = q->nextTok();
		if (t.id != WORD_TK || t.lower() == "on") error("Expected alias after as. Found: "+t.val);
	case WORD_TK:
		if (t.lower() == "on"sv) break;
		n->tok4 = t;
		t = q->nextTok();
		break;
	}
	s = t.lower();
	if (s == "noheader"sv || s == "nh"sv || s == "header"sv || s == "h"sv) {
		n->tok5 = t;
		t = q->nextTok();
	}
	if (n->tok4.id == 0)
		error("Joined file requires an alias.");
	if (t.lower() != "on"sv) error("Expected 'on'. Found: "+t.val);
	q->nextTok();
	n->node1 = parsePredicates();
	n->node2 = parseJoin();
	return n;
}

//node1 is conditions
unique_ptr<node> parser::parseWhere() {
	token t = q->tok();
	if (t.lower() != "where"sv) { return nullptr; }
	q->whereFiltering = true;
	unique_ptr<node> n = newNode(N_WHERE);
	q->nextTok();
	n->node1 = parsePredicates();
	return n;
}

//node1 is conditions
unique_ptr<node> parser::parseHaving() {
	token t = q->tok();
	if (t.lower() != "having"sv) { return nullptr; }
	q->havingFiltering = true;
	unique_ptr<node> n = newNode(N_HAVING);
	q->nextTok();
	n->node1 = parsePredicates();
	return n;
}

//old: node1 is sort expr
//new: node1 is sort expr list
//tok1 is asc
unique_ptr<node> parser::parseOrder() {
	token t = q->tok();
	if (t.lower() == "order"sv) {
		if (q->nextTok().lower() != "by"sv) error("Expected 'by' after 'order'. Found "+q->tok().val);
		q->sorting = 1;
		q->nextTok();
		unique_ptr<node> n = newNode(N_ORDER);
		n->node1 = parseExpressionList(false, true);
		return n;
	}
	return nullptr;
}

//tok1 is function id
//tok2 is * for count(*), preconv paramtype for len()
//tok3 is cipher or distinct
//tok4 is password or (determined later) count of distinct N or S functions
//[PARAMTYPE] is paramtype for type conversion
//[RETTYPE] is inflexible return type
//[MIDIDX] is midrow index
//node1 is expression in parens
unique_ptr<node> parser::parseFunction() {
	token t = q->tok();
	unique_ptr<node> n = newNode(N_FUNCTION);
	n->tok1 = t;
	n->tok1.id = functionMap.at(t.lower());
	q->nextTok(); // (
	t = q->nextTok();
	//count(*)
	if (t.id == SP_STAR && n->tok1.lower() == "count"sv) {
		n->tok2 = t;
		q->nextTok();
	//everything else
	} else {
		if (t.lower() == "distinct"sv) {
			n->tok3 = t;
			t = q->nextTok();
		}
		bool needPrompt = true;
		string cipherTok = "chacha";
		string password;
		switch (n->tok1.id) {
		case FN_COALESCE:
			n->node1 = parseExpressionList(true, false);
			break;
		case FN_ENCRYPT:
		case FN_DECRYPT:
			//first param is expression to en/decrypt
			n->node1 = parseExprAdd();
			if (q->tok().id == SP_COMMA) {
				//second param is cipher
				t = q->nextTok();
				cipherTok = t.lower();
				auto commaOrParen = q->nextTok();
				if (commaOrParen.id == SP_COMMA) {
					//password is 3rd param
					password = q->nextTok().val;
					needPrompt = false;
					q->nextTok();
				} else if (commaOrParen.id != SP_RPAREN) {
					error("Expected comma or closing parenthesis after cipher in crypto function. Found: "+commaOrParen.val);
				}
			} else if (q->tok().id != SP_RPAREN) {
				error("Expect closing parenthesis or comma after expression in crypto function. Found: "+q->tok().val);
			}
			if (q->password == "" && needPrompt) q->password = promptPassword();
			if (password == "") { password = q->password; }
			n->tok3.val = cipherTok;
			n->tok4.val = password;
			break;

		case FN_INC:
		case FN_RAND:
			break;
		case FN_POW:
			n->node1 = parseExprAdd();
			if (t = q->tok(); t.id != SP_COMMA)
				error("POW function expected comma before second expression. Found: " +t.val);
			q->nextTok();
			n->node2 = parseExprAdd();
			break;
		case FN_SUBSTR:
			n->node1 = parseExprAdd();
			if (t = q->tok(); t.id != SP_COMMA)
				error("SUBSTR function expected comma before second parameter. Found: " +t.val);
			n->tok2 = q->nextTok();
			if (t = q->nextTok(); t.id == SP_COMMA){
				n->tok3 = q->nextTok();
				q->nextTok();
			}
			break;
		default:
			n->node1 = parseExprAdd();
		}
	}
	if (q->tok().id != SP_RPAREN) error("Expected closing parenthesis after function. Found: "+q->tok().val);
	q->nextTok();
	//groupby if aggregate function
	if ((n->tok1.id & AGG_BIT) != 0) {
		q->grouping = max(q->grouping,1);
	}
	return n;
}

//node1 is groupExpressions
unique_ptr<node> parser::parseGroupby() {
	token t = q->tok();
	if (!(t.lower() == "group"sv && q->peekTok().lower() == "by"sv)) { return nullptr; }
	q->grouping = max(q->grouping,2);
	unique_ptr<node> n = newNode(N_GROUPBY);
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
unique_ptr<node> parser::parseExpressionList(bool interdependant, bool sortlist) { //bool arg if expression types are interdependant
	token t = q->tok();
	int label = N_EXPRESSIONS;
	if (interdependant) label = N_DEXPRESSIONS;
	unique_ptr<node> n = newNode(label);
	n->node1 = parseExprAdd();
	t = q->tok();
	if (sortlist && (t.lower() == "asc"sv || t.lower() == "desc"sv)){
		n->tok1.id = t.lower() == "asc"sv;
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
