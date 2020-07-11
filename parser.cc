#include "interpretor.h"
#include <boost/algorithm/string/replace.hpp>

static void parseOptions(querySpecs &q);
static unique_ptr<node> parsePreSelect(querySpecs &q);
static unique_ptr<node> parseWith(querySpecs &q);
static unique_ptr<node> parseVars(querySpecs &q);
static unique_ptr<node> parseSelect(querySpecs &q);
static unique_ptr<node> parseSelections(querySpecs &q);
static unique_ptr<node> parseExprAdd(querySpecs &q);
static unique_ptr<node> parseExprMult(querySpecs &q);
static unique_ptr<node> parseExprNeg(querySpecs &q);
static unique_ptr<node> parseExprCase(querySpecs &q);
static unique_ptr<node> parseCaseWhenPredList(querySpecs &q);
static unique_ptr<node> parseCaseWhenExprList(querySpecs &q);
static unique_ptr<node> parseCaseWhenExpr(querySpecs &q);
static unique_ptr<node> parseCasePredicate(querySpecs &q);
static unique_ptr<node> parsePredicates(querySpecs &q);
static unique_ptr<node> parsePredCompare(querySpecs &q);
static unique_ptr<node> parseJoin(querySpecs &q);
static unique_ptr<node> parseOrder(querySpecs &q);
static unique_ptr<node> parseHaving(querySpecs &q);
static unique_ptr<node> parseGroupby(querySpecs &q);
static unique_ptr<node> parseFrom(querySpecs &q);
static unique_ptr<node> parseWhere(querySpecs &q);
static unique_ptr<node> parseValue(querySpecs &q);
static unique_ptr<node> parseFunction(querySpecs &q);
static unique_ptr<node> parseAfterFrom(querySpecs &q);
static unique_ptr<node> parseExpressionList(querySpecs &q, bool i, bool s);
static void parseTop(querySpecs &q);
static void parseLimit(querySpecs &q);

unique_ptr<node> newNode(int l){
	unique_ptr<node> n(new node);
	n->tok1 = n->tok2 = n->tok3 = n->tok4 = n->tok5 = token{};
	n->label = l;
	n->phase = n->datatype = 0;
	n->keep = false;
	return n;
}
void node::print(){
	cerr << treeMap[label] << endl;
	tok1.print();
	tok2.print();
	tok3.print();
	tok4.print();
	tok5.print();
}



//recursive descent parser for query
void parseQuery(querySpecs &q) {
	q.tree = newNode(N_QUERY);
	q.tree->node1 = parsePreSelect(q);
	q.tree->node2 = parseSelect(q);
	q.tree->node3 = parseFrom(q);
	q.tree->node4 = parseAfterFrom(q);
	parseLimit(q);
}

//could include other commands like describe
//node1 is with
static unique_ptr<node> parsePreSelect(querySpecs &q) {
	token t = q.tok();
	parseOptions(q);
	unique_ptr<node> n = newNode(N_PRESELECT);
	n->node1 = parseWith(q);
	return n;
}

//anything that comes after 'from', in any order
static unique_ptr<node> parseAfterFrom(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_AFTERFROM);
	for (;;){
		t = q.tok();
		switch (t.id){
		case KW_WHERE:
			n->node1 = parseWhere(q);
			break;
		case KW_GROUP:
			n->node2 = parseGroupby(q);
			break;
		case KW_HAVING:
			n->node3 = parseHaving(q);
			break;
		case KW_ORDER:
			n->node4 = parseOrder(q);
			break;
		default:
			goto done;
		}
	}
	done:
	return n;
}

static void parseOptions(querySpecs &q) {
	token t = q.tok();
	string s = t.lower();
	if (s == "c") {
		q.options |= O_C;
	} else if (s == "nh" || s == "noheader") {
		q.options |= O_NH;
	} else if (s == "h" || s == "header") {
		q.options |= O_H;
	} else {
		return;
	}
	q.nextTok();
	parseOptions(q);
}

//node1 is vars
static unique_ptr<node> parseWith(querySpecs &q) {
	token t = q.tok();
	if (t.lower() != "with") { return nullptr; }
	unique_ptr<node> n = newNode(N_WITH);
	q.nextTok();
	n->node1 = parseVars(q);
	return n;
}

//node1 is expr
//tok1 is id
//tok3 is midrow index-1 of phase2 non-aggs
//node2 is vars
static unique_ptr<node> parseVars(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_VARS);
	n->node1 = parseExprAdd(q);
	t = q.tok();
	if (t.lower() != "as") error("Expected 'as' after expression. Found "+t.val);
	t = q.nextTok();
	if (t.id != WORD_TK) error("Expected variable name. Found "+t.val);
	n->tok1 = t;
	t = q.nextTok();
	if (t.lower() == "and" || t.id == SP_COMMA) {
		q.nextTok();
		n->node2 = parseVars(q);
	}
	return n;
}

//node1 is selections
static unique_ptr<node> parseSelect(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_SELECT);
	if (t.lower() != "select") error("Expected 'select'. Found "+t.val);
	q.nextTok();
	parseTop(q);
	n->node1 = parseSelections(q);
	return n;
}

//node1 is expression
//node2 is next selection
//tok1 is * or distinct or hidden
//tok2 is alias
//later stages:
//  tok3.id will be midrow index
//  tok4.id will be destrow index
static unique_ptr<node> parseSelections(querySpecs &q) {
	token t = q.tok();
	if (t.lower() == "from"){
		return nullptr;
	}
	unique_ptr<node> n = newNode(N_SELECTIONS);
	if (t.id == SP_COMMA) t = q.nextTok();
	switch (t.id) {
	case SP_STAR:
		n->tok1 = t;
		q.nextTok();
		return n;
	case KW_DISTINCT:
		n->tok1 = t;
		t = q.nextTok();
		if (t.lower() == "hidden" && !t.quoted) {
			n->tok1 = t;
			n->tok1.id = KW_DISTINCT;
			t = q.nextTok();
		}
	//expression
	case KW_CASE:
	case WORD_TK:
	case SP_MINUS:
	case SP_LPAREN:
		//alias = expression
		if (q.peekTok().id == SP_EQ) {
			if (t.id != WORD_TK) error("Alias must be a word. Found "+t.val);
			n->tok2 = t;
			q.nextTok();
			q.nextTok();
			n->node1 = parseExprAdd(q);
		//expression
		} else {
			n->node1 = parseExprAdd(q);
			t = q.tok();
			if (t.lower() == "as") {
				t = q.nextTok();
				n->tok2 = t;
				q.nextTok();
			}
		}
		n->node2 = parseSelections(q);
		return n;
	default:
		error("Expected a new selection or 'from' clause. Found "+t.val);
	}
	return nullptr;
}

//node1 is exprMult
//node2 is exprAdd
//tok1 is add/minus operator
static unique_ptr<node> parseExprAdd(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_EXPRADD);
	n->node1 = parseExprMult(q);
	t = q.tok();
	switch (t.id) {
	case SP_MINUS:
	case SP_PLUS:
		n->tok1 = t;
		q.nextTok();
		n->node2 = parseExprAdd(q);
	}
	return n;
}

//node1 is exprNeg
//node2 is exprMult
//tok1 is mult/div operator
static unique_ptr<node> parseExprMult(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_EXPRMULT);
	n->node1 = parseExprNeg(q);
	t = q.tok();
	switch (t.id) {
	case SP_STAR:
		if (q.peekTok().lower() == "from") { break; }
	case SP_MOD:
	case SP_CARROT:
	case SP_DIV:
		n->tok1 = t;
		q.nextTok();
		n->node2 = parseExprMult(q);
	}
	return n;
}

//tok1 is minus operator
//node1 is exprCase
static unique_ptr<node> parseExprNeg(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_EXPRNEG);
	if (t.id == SP_MINUS) {
		n->tok1 = t;
		q.nextTok();
	}
	n->node1 = parseExprCase(q);
	return n;
}

//tok1 is [case, word, expr] token - tells if case, terminal value, or (expr)
//tok2 is [when, expr] token - tells what kind of case. predlist, or expr exprlist respectively
//node1 is (expression), when predicate list, expression for exprlist
//node2 is expression list to compare to initial expression
//node3 is else expression
//later stages:
//  tok3.id is datatype of when comparison expressions
static unique_ptr<node> parseExprCase(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_EXPRCASE);
	token t2;
	switch (t.id) {
	case KW_CASE:
		n->tok1 = t;

		t2 = q.nextTok();
		switch (t2.id) {
		//when predicates are true
		case KW_WHEN:
			n->tok2 = t2;
			n->node1 = parseCaseWhenPredList(q);
			break;
		//expression matches expression list
		case WORD_TK:
		case SP_LPAREN:
			n->tok2 = t2;
			n->node1 = parseExprAdd(q);
			t = q.tok();
			if (t.lower() != "when") error("Expected 'when' after case expression. Found "+t.val);
			n->node2 = parseCaseWhenExprList(q);
			break;
		default: error("Expected expression or 'when'. Found "+q.tok().val);
		}

		switch (q.tok().id) {
		case KW_END:
			q.nextTok();
			break;
		case KW_ELSE:
			t = q.nextTok();
			n->node3 = parseExprAdd(q);
			t = q.tok();
			if (t.lower() != "end") error("Expected 'end' after 'else' expression. Found "+t.val);
			q.nextTok();
			break;
		default:
			error("Expected 'end' or 'else' after case expression. Found "+q.tok().val);
		}
		break;

	case WORD_TK:
		n->tok1 = t;
		n->node1 = parseValue(q);
		break;

	case SP_LPAREN:
		n->tok1 = t;
		q.nextTok();
		n->node1 = parseExprAdd(q);
		if (q.tok().id != SP_RPAREN) error("Expected closing parenthesis. Found "+q.tok().val);
		q.nextTok();
		break;
	default: error("Expected case, value, or expression. Found "+q.tok().val);
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
static unique_ptr<node> parseValue(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_VALUE);
	n->tok1 = t;
	//see if it's a function
	if (functionMap.count(t.lower()) && !t.quoted && q.peekTok().id==SP_LPAREN) {
		n->tok2.id = FUNCTION;
		n->node1 = parseFunction(q);
	//any non-function value
	} else {
		q.nextTok();
	}
	return n;
}

//node1 is case predicate
//node2 is next case predicate list node
static unique_ptr<node> parseCaseWhenPredList(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_CPREDLIST);
	n->node1 = parseCasePredicate(q);
	t = q.tok();
	if (t.lower() == "when") {
		n->node2 = parseCaseWhenPredList(q);
	}
	return n;
}

//node1 is predicates
//node2 is expression if true
static unique_ptr<node> parseCasePredicate(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_CPRED);
	q.nextTok(); //eat when token
	n->node1 = parsePredicates(q);
	t = q.tok();
	if (t.lower() != "then") error("Expected 'then' after predicate. Found: "+q.tok().val);
	q.nextTok(); //eat then token
	n->node2 = parseExprAdd(q);
	return n;
}

//tok1 is logop
//tok2 is negation
//node1 is predicate comparison
//node2 is next predicates node
static unique_ptr<node> parsePredicates(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_PREDICATES);
	if (t.id == SP_NEGATE) { n->tok2 = t; t = q.nextTok(); }
	n->node1 = parsePredCompare(q);
	t = q.tok();
	if ((t.id & LOGOP) != 0) {
		n->tok1 = t;
		q.nextTok();
		n->node2 = parsePredicates(q);
	}
	return n;
}

//tok1 is [relop, paren] for comparison or more predicates
//tok2 is negation
//tok3 is 'like' expression
//tok4 will be number of node (1,2) for indexable join value
//tok5 will be index of valpos vector for joins
//node1 is [expr, predicates]
//node2 is second expr
//node3 is third expr for betweens
static unique_ptr<node> parsePredCompare(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_PREDCOMP);
	int negate = 0;
	if (t.id == SP_NEGATE) { negate ^= 1; t = q.nextTok(); }
	//more predicates in parentheses
	if (t.id == SP_LPAREN) {
		int pos = q.tokIdx;
		//try parsing as predicate
		t = q.nextTok();
		bool tryAgain = false;
		try {
			n->node1 = parsePredicates(q);
			t = q.tok();
			if (t.id != SP_RPAREN) error("Expected cosing parenthesis. Found: "+t.val);
		//if failed, reparse as expression
		} catch (const std::invalid_argument& e) {
			q.tokIdx = pos;
			tryAgain = true;
		}
		//return if (more comparisions)
		if (!tryAgain) {
			n->tok1.id = SP_LPAREN;
			return n;
		}
	}
	//comparison
	n->node1 = parseExprAdd(q);
	t = q.tok();
	if (t.id == SP_NEGATE) {
		negate ^= 1;
		t = q.nextTok();
	}
	n->tok2.id = negate;
	if ((t.id & RELOP) == 0) error("Expected relational operator. Found: "+t.val);
	t = q.tok();
	n->tok1 = t;
	t = q.nextTok();
	if (n->tok1.id == KW_LIKE) {
		n->tok3 = t;
		t = q.nextTok();
	} else if (n->tok1.id == KW_IN) {
		if (t.id != SP_LPAREN) error("Expected opening parenthesis for expression list. Found: "+t.val);
		q.nextTok();
		n->node2 = parseExpressionList(q,true,false);
		t = q.tok();
		if (t.id != SP_RPAREN) error("Expected closing parenthesis after expression list. Found: "+t.val);
		q.nextTok();
	} else {
		n->node2 = parseExprAdd(q);
	}
	if (n->tok1.id == KW_BETWEEN) {
		q.nextTok();
		n->node3 = parseExprAdd(q);
	}
	return n;
}

//node1 is case expression
//node2 is next exprlist node
static unique_ptr<node> parseCaseWhenExprList(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_CWEXPRLIST);
	n->node1 = parseCaseWhenExpr(q);
	t = q.tok();
	if (t.lower() == "when")
		n->node2 = parseCaseWhenExprList(q);
	return n;
}

//node1 is comparison expression
//node2 is result expression
//tok1.id will be comparision datatype
static unique_ptr<node> parseCaseWhenExpr(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_CWEXPR);
	q.nextTok(); //eat when token
	n->node1 = parseExprAdd(q);
	q.nextTok(); //eat then token
	n->node2 = parseExprAdd(q);
	return n;
}

//row limit at front of query
static void parseTop(querySpecs &q) {
	token t = q.tok();
	if (t.lower() == "top") {
		t = q.nextTok();
		if (!is_number(t.val)) error("Expected number after 'top'. Found "+t.val);
		q.quantityLimit = atoi(t.val.c_str());
		q.nextTok();
	}
}

//row limit at end of query
static void parseLimit(querySpecs &q) {
	token t = q.tok();
	if (t.lower() == "limit") {
		t = q.nextTok();
		if (!is_number(t.val)) error("Expected number after 'limit'. Found "+t.val);
		q.quantityLimit = atoi(t.val.c_str());
		q.nextTok();
	}
}

//tok1 is file path
//tok4 is alias
//tok5 is noheader
//node1 is joins
static unique_ptr<node> parseFrom(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_FROM);
	if (t.lower() != "from") error("Expected 'from'. Found: "+t.val);
	t = q.nextTok();
	n->tok1.val = boost::replace_all_copy(t.val, "~/", string(getenv("HOME"))+"/");
	t = q.nextTok();
	string s = t.lower();
	if (s == "noheader" || s == "nh" || s == "header" || s == "h") {
		n->tok5 = t;
		t = q.nextTok();
	}
	switch (t.id) {
	case KW_AS:
		t = q.nextTok();
		if (t.id != WORD_TK) error("Expected alias after as. Found: "+t.val);
	case WORD_TK:
		if (joinMap.count(t.lower())) break;
		n->tok4 = t;
		q.nextTok();
	}
	t = q.tok();
	s = t.lower();
	if (s == "noheader" || s == "nh" || s == "header" || s == "h") {
		n->tok5 = t;
		q.nextTok();
	}
	n->node1 = parseJoin(q);
	return n;
}

//tok1 is filepath
//tok2 is join token (join,sjoin,bjoin)
//tok3 is join details (left/outer or inner)
//tok4 is alias
//tok5 is noheader
//node1 is join condition (predicates)
//node2 is next join
static unique_ptr<node> parseJoin(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_JOIN);
	string s = t.lower();
	if (joinMap.count(s) == 0)
		return nullptr;
	q.joining = true;
	if (s == "left" || s == "inner" || s == "outer"){
		n->tok3 = t;
		q.nextTok();
	}
	bool sizeOverride = false;
	t = q.tok();
	s = t.lower();
	if (s == "join" || s == "sjoin" || s == "bjoin"){
		n->tok2 = t;
		t = q.nextTok();
	} else {
		error("Expected 'join'. Found:"+q.tok().val);
	}
	//file path
	boost::replace_first(t.val, "~/", string(getenv("HOME"))+"/");
	n->tok1 = t;
	//1st after filepath
	t = q.nextTok();
	s = t.lower();
	if (s == "noheader" || s == "nh" || s == "header" || s == "h") {
		n->tok5 = t;
		t = q.nextTok();
	}
	//alias
	switch (t.id) {
	case KW_AS:
		t = q.nextTok();
		if (t.id != WORD_TK || t.lower() == "on") error("Expected alias after as. Found: "+t.val);
	case WORD_TK:
		if (t.lower() == "on") break;
		n->tok4 = t;
		t = q.nextTok();
		break;
	}
	s = t.lower();
	if (s == "noheader" || s == "nh" || s == "header" || s == "h") {
		n->tok5 = t;
		t = q.nextTok();
	}
	if (t.lower() != "on") error("Expected 'on'. Found: "+t.val);
	q.nextTok();
	n->node1 = parsePredicates(q);
	n->node2 = parseJoin(q);
	return n;
}

//node1 is conditions
static unique_ptr<node> parseWhere(querySpecs &q) {
	token t = q.tok();
	if (t.lower() != "where") { return nullptr; }
	q.whereFiltering = true;
	unique_ptr<node> n = newNode(N_WHERE);
	q.nextTok();
	n->node1 = parsePredicates(q);
	return n;
}

//node1 is conditions
static unique_ptr<node> parseHaving(querySpecs &q) {
	token t = q.tok();
	if (t.lower() != "having") { return nullptr; }
	q.havingFiltering = true;
	unique_ptr<node> n = newNode(N_HAVING);
	q.nextTok();
	n->node1 = parsePredicates(q);
	return n;
}

//old: node1 is sort expr
//new: node1 is sort expr list
//tok1 is asc
static unique_ptr<node> parseOrder(querySpecs &q) {
	token t = q.tok();
	if (t.lower() == "order") {
		if (q.nextTok().lower() != "by") error("Expected 'by' after 'order'. Found "+q.tok().val);
		q.sorting = 1;
		q.nextTok();
		unique_ptr<node> n = newNode(N_ORDER);
		n->node1 = parseExpressionList(q, false, true);
		return n;
	}
	return nullptr;
}

//tok1 is function id
//tok2 is * for count(*)
//tok3 is cipher or distinct
//tok4 is password or (determined later) count of distinct N or S functions
//tok5 is paramtype for type conversion
//tok6 is midrow index +1
//node1 is expression in parens
static unique_ptr<node> parseFunction(querySpecs &q) {
	token t = q.tok();
	unique_ptr<node> n = newNode(N_FUNCTION);
	n->tok1 = t;
	n->tok1.id = functionMap[t.lower()];
	q.nextTok(); // (
	t = q.nextTok();
	//count(*)
	if (t.id == SP_STAR && n->tok1.lower() == "count") {
		n->tok2 = t;
		q.nextTok();
	//everything else
	} else {
		if (t.lower() == "distinct") {
			n->tok3 = t;
			t = q.nextTok();
		}
		bool needPrompt = true;
		token commaOrParen;
		string cipherTok = "aes";
		string password;
		switch (n->tok1.id) {
		case FN_COALESCE:
			n->node1 = parseExpressionList(q, true, false);
			break;
		case FN_ENCRYPT:
		case FN_DECRYPT:
			//first param is expression to en/decrypt
			n->node1 = parseExprAdd(q);
			if (q.tok().id == SP_COMMA) {
				//second param is cipher
				t = q.nextTok();
				cipherTok = t.lower();
				commaOrParen = q.nextTok();
				if (commaOrParen.id == SP_COMMA) {
					//password is 3rd param
					password = q.nextTok().val;
					needPrompt = false;
					q.nextTok();
				} else if (commaOrParen.id != SP_RPAREN) {
					error("Expected comma or closing parenthesis after cipher in crypto function. Found: "+commaOrParen.val);
				}
			} else if (q.tok().id != SP_RPAREN) {
				error("Expect closing parenthesis or comma after expression in crypto function. Found: "+q.tok().val);
			}
			//if (q.password == "" && needPrompt) q.password = promptPassword();
			if (password == "") { password = q.password; }
			if (cipherTok != "chacha" && cipherTok != "aes")
				error("Second parameter to encryption function is cipher type (aes or chacha). Found: "+cipherTok+". "
						"Use aes for strong but bulky encryption, or chacha for something a government could probably crack but takes less space.");
			n->tok3.val = cipherTok;
			n->tok4.val = password;
			break;
		case FN_INC:
			break;
		default:
			n->node1 = parseExprAdd(q);
		}
	}
	if (q.tok().id != SP_RPAREN) error("Expected closing parenthesis after function. Found: "+q.tok().val);
	q.nextTok();
	//groupby if aggregate function
	if ((n->tok1.id & AGG_BIT) != 0) {
		q.grouping = max(q.grouping,1);
	}
	return n;
}

//node1 is groupExpressions
static unique_ptr<node> parseGroupby(querySpecs &q) {
	token t = q.tok();
	if (!(t.lower() == "group" && q.peekTok().lower() == "by")) { return nullptr; }
	q.grouping = max(q.grouping,2);
	unique_ptr<node> n = newNode(N_GROUPBY);
	q.nextTok();
	q.nextTok();
	n->node1 = parseExpressionList(q,false,false);
	return n;
}

//node1 is expression
//node2 is expressionlist
//tok1 id asc for sorting lists
//tok2.id will be sort list size, initially set to 1
//tok3.id will be destrow index for aggregate sort values
//	or sequence number for non-agg sort values
static unique_ptr<node> parseExpressionList(querySpecs &q, bool interdependant, bool sortlist) { //bool arg if expression types are interdependant
	token t = q.tok();
	int label = N_EXPRESSIONS;
	if (interdependant) label = N_DEXPRESSIONS;
	unique_ptr<node> n = newNode(label);
	n->node1 = parseExprAdd(q);
	t = q.tok();
	if (sortlist && (t.lower() == "asc" || t.lower() == "desc")){
		n->tok1.id = t.lower() == "asc";
		t = q.nextTok();
	}
	n->tok2.id = sortlist;
	switch (t.id){
	case SP_COMMA: q.nextTok();
	case SP_LPAREN:
	case KW_CASE:
	case WORD_TK: break;
	default: return n;
	}
	n->node2 = parseExpressionList(q, interdependant, sortlist);
	return n;
}
