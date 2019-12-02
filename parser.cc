#include "interpretor.h"
#include <sys/stat.h>
#include <regex>

void parseOptions(QuerySpecs &q);
Node* parseSelect(QuerySpecs &q);
Node* parseSelections(QuerySpecs &q);
Node* parseExprAdd(QuerySpecs &q);
Node* parseExprMult(QuerySpecs &q);
Node* parseExprNeg(QuerySpecs &q);
Node* parseExprCase(QuerySpecs &q);
Node* parseCaseWhenPredList(QuerySpecs &q);
Node* parseCaseWhenExprList(QuerySpecs &q);
Node* parseCaseWhenExpr(QuerySpecs &q);
Node* parseCasePredicate(QuerySpecs &q);
Node* parsePredicates(QuerySpecs &q);
Node* parsePredCompare(QuerySpecs &q);
Node* parseJoin(QuerySpecs &q);
Node* parseJoinPredicates(QuerySpecs &q);
Node* parseJoinPredCompare(QuerySpecs &q);
Node* parseOrder(QuerySpecs &q);
Node* parseHaving(QuerySpecs &q);
Node* parseGroupby(QuerySpecs &q);
Node* parseFrom(QuerySpecs &q);
Node* parseWhere(QuerySpecs &q);
Node* parseExpressionList(QuerySpecs &q, bool independant);
void parseLimit(QuerySpecs &q);

//recursive descent parser builds parse tree and QuerySpecs
Node* parseQuery(QuerySpecs &q) {
	Node* n = newNode(N_QUERY);
	scanTokens(q);
	parseOptions(q);
	n->node1 = parseSelect(q);
	n->node2 = parseFrom(q);
	n->node3 = parseWhere(q);
	n->node4 = parseGroupby(q);
	n->node5 = parseHaving(q);
	n->node6 = parseOrder(q);
	parseLimit(q);
	return n;
}

void parseOptions(QuerySpecs &q) {
	string t = q.Tok().Lower();
	if (t == "c") {
		q.options = O_C;
	} else if (t == "nh") {
		q.options = O_NH;
	} else if (t == "h") {
	} else {
		return;
	}
	q.NextTok();
	parseOptions(q);
}

//node1 is selections
Node* parseSelect(QuerySpecs &q) {
	Node* n = newNode(N_SELECT);
	if (q.Tok().Lower() != "select") error("Expected 'select'. Found "+q.Tok().val);
	q.NextTok();
	//parseTop(q);
	n->node1 = parseSelections(q);
	return n;
}

//node1 is expression
//node2 is next selection
//tok1 is * or distinct or hidden
//tok2 is alias
//tok3 is bit array - 1 and 2 are distinct
//tok4 will be aggregate function
//tok5 will be type
Node* parseSelections(QuerySpecs &q) {
	if (q.Tok().Lower() == "from") return nullptr;
	Node* n = newNode(N_SELECTIONS);
	if (q.Tok().id == SP_COMMA) q.NextTok();
	Token tok = q.Tok();
	switch (tok.id) {
	case SP_STAR:
		n->tok1 = tok;
		q.NextTok();
		return n;
	//tok3 bit 1 means distinct, bit 2 means hidden
	case KW_DISTINCT:
		n->tok1 = tok;
		q.NextTok();
		if (q.Tok().Lower() == "hidden" && !q.Tok().quoted) { n->tok1 = tok; q.NextTok(); }
	//expression
	case KW_CASE:
	case WORD:
	case SP_MINUS:
	case SP_LPAREN:
		//alias = expression
		if (q.PeekTok().id == SP_EQ) {
			if (tok.id != WORD) error("Alias must be a word. Found "+tok.val);
			n->tok2 = tok;
			q.NextTok();
			q.NextTok();
			n->node1 = parseExprAdd(q);
		//expression
		} else {
			n->node1 = parseExprAdd(q);
			if (q.Tok().Lower() == "as") {
				n->tok2 = q.NextTok();
				q.NextTok();
			}
		}
		n->node2 = parseSelections(q);
		return n;
	default:
		delete n;
		error("Expected a new selection or 'from' clause. Found "+q.Tok().val);
	}
	delete n;
	return nullptr;
}
//node1 is exprMult
//node2 is exprAdd
//tok1 is add/minus operator
Node* parseExprAdd(QuerySpecs &q) {
	Node* n = newNode(N_EXPRADD);
	n->node1 = parseExprMult(q);
	switch (q.Tok().id) {
	case SP_MINUS:
	case SP_PLUS:
		n->tok1 = q.Tok();
		q.NextTok();
		n->node2 = parseExprAdd(q);
	}
	return n;
}

//node1 is exprNeg
//node2 is exprMult
//tok1 is mult/div operator
Node* parseExprMult(QuerySpecs &q) {
	Node* n = newNode(N_EXPRMULT);
	n->node1 = parseExprNeg(q);
	switch (q.Tok().id) {
	case SP_STAR:
		if (q.PeekTok().Lower() == "from") { break; }
	case SP_MOD:
	case SP_CARROT:
	case SP_DIV:
		n->tok1 = q.Tok();
		q.NextTok();
		n->node2 = parseExprMult(q);
	}
	return n;
}

//tok1 is minus operator
//node1 is exprCase
Node* parseExprNeg(QuerySpecs &q) {
	Node* n = newNode(N_EXPRNEG);
	if (q.Tok().id == SP_MINUS) {
		n->tok1 = q.Tok();
		q.NextTok();
	}
	//n->node1 = parseExprCase(q);
	return n;
}

//tok1 is [case, word, expr] token - tells if case, terminal value, or (expr)
//tok2 is [when, expr] token - tells what kind of case. predlist, or expr exprlist respectively
//node2.tok3 will be initial 'when' expression type
//node1 is (expression), when predicate list, expression for exprlist
//node2 is expression list to compare to initial expression
//node3 is else expression
Node* parseExprCase(QuerySpecs &q) {
	Node* n = newNode(N_EXPRCASE);
	Token tok = q.Tok();
	Token tok2;
	switch (tok.id) {
	case KW_CASE:
		n->tok1 = tok;

		tok2 = q.NextTok();
		switch (tok2.id) {
		//when predicates are true
		case KW_WHEN:
			n->tok2 = tok2;
			n->node1 = parseCaseWhenPredList(q);
			break;
		//expression matches expression list
		case WORD:
		case SP_LPAREN:
			n->tok2 = tok2;
			n->node1 = parseExprAdd(q);
			if (q.Tok().Lower() != "when") error("Expected 'when' after case expression. Found "+q.Tok().val);
			//n->node2 = parseCaseWhenExprList(q);
		default: error("Expected expression or 'when'. Found "+q.Tok().val);
		}

		switch (q.Tok().id) {
		case KW_END:
			q.NextTok();
			break;
		case KW_ELSE:
			q.NextTok();
			n->node3 = parseExprAdd(q);
			if (q.Tok().Lower() != "end") error("Expected 'end' after 'else' expression. Found "+q.Tok().val);
			q.NextTok();
			break;
		default:
			error("Expected 'end' or 'else' after case expression. Found "+q.Tok().val);
		}
		break;

	case WORD:
		n->tok1 = tok;
		//n->node1 = parseValue(q);
		break;

	case SP_LPAREN:
		n->tok1 = tok;
		q.NextTok();
		n->node1 = parseExprAdd(q);
		if (q.Tok().id != SP_RPAREN) error("Expected closing parenthesis. Found "+q.Tok().val);
		q.NextTok();
		break;
	default: error("Expected case, value, or expression. Found "+q.Tok().val);
	}
	return n;
}

//if implement dot notation, put parser here
//tok1 is [value, column index, function id]
//node1 is function expression if doing that
Node* parseValue(QuerySpecs &q) {
	Node* n = newNode(N_VALUE);
	Token tok = q.Tok();
	//see if it's a function
	if (functionMap.count(tok.Lower()) && !tok.quoted && q.PeekTok().id==SP_LPAREN) {
		//n->node1 = parseFunction(q);
		return n;
	//any non-function value
	} else {
		n->tok1 = tok;
	}
	q.NextTok();
	return n;
}

//tok1 says if more predicates
//node1 is case predicate
//node2 is next case predicate list node
Node* parseCaseWhenPredList(QuerySpecs &q) {
	Node* n = newNode(N_CPREDLIST);
	n->node1 = parseCasePredicate(q);
	if (q.Tok().Lower() == "when") {
		n->tok1 = q.Tok();
		n->node2 = parseCaseWhenPredList(q);
	}
	return n;
}

//node1 is predicates
//node2 is expression if true
Node* parseCasePredicate(QuerySpecs &q) {
	Node* n = newNode(N_CPRED);
	q.NextTok(); //eat when token
	n->node1 = parsePredicates(q);
	if (q.Tok().Lower() != "then") error("Expected 'then' after predicate. Found: "+q.Tok().val);
	q.NextTok(); //eat then token
	n->node2 = parseExprAdd(q);
	return n;
}

//tok1 is logop
//tok2 is negation
//node1 is predicate comparison
//node2 is next predicates node
Node* parsePredicates(QuerySpecs &q) {
	Node* n = newNode(N_PREDICATES);
	if (q.Tok().id == SP_NEGATE) { n->tok2 = q.Tok(); q.NextTok(); }
	n->node1 = parsePredCompare(q);
	if ((q.Tok().id & LOGOP) != 0) {
		n->tok1 = q.Tok();
		q.NextTok();
		n->node2 = parsePredicates(q);
	}
	return n;
}

//more strict version of parsePredicates for joins
//node1 is predicate comparison
Node* parseJoinPredicates(QuerySpecs &q) {
	Node* n = newNode(N_PREDICATES);
	n->node1 = parseJoinPredCompare(q);
	if ((q.Tok().id & LOGOP) != 0) error("Only one join condition per file");
	return n;
}

//more strict version of parsePredCompare for joins
Node* parseJoinPredCompare(QuerySpecs &q) {
	Node* n = newNode(N_PREDCOMP);
	n->node1 = parseExprAdd(q);
	if (q.Tok().id != SP_EQ) error("Expected = operator. Found: "+q.Tok().val);
	q.NextTok();
	n->node2 = parseExprAdd(q);
	return n;
}

//tok1 is [relop, paren] for comparison or more predicates
//tok2 is negation
//tok3 will be independant type
//tok4 will be join file key
//tok5 will be join struct
//node1 is [expr, predicates]
//node2 is second expr
//node3 is third expr for betweens
Node* parsePredCompare(QuerySpecs &q) {
	Node* n = newNode(N_PREDCOMP);
	int negate = 0;
	if (q.Tok().id == SP_NEGATE) { negate ^= 1; q.NextTok(); }
	//more predicates in parentheses
	if (q.Tok().id == SP_LPAREN) {
		int pos = q.tokIdx;
		//try parsing as predicate
		q.NextTok();
		bool tryAgain = false;
		try {
			n->node1 = parsePredicates(q);
			if (q.Tok().id != SP_RPAREN) error("Expected cosing parenthesis. Found: "+q.Tok().val);
			q.NextTok();
		//if failed, reparse as expression
		} catch (const std::invalid_argument& e) {
			q.tokIdx = pos;
			tryAgain = true;
		}
		if (!tryAgain) return n;
	}
	//comparison
	n->node1 = parseExprAdd(q);
	if (q.Tok().id == SP_NEGATE) {
		negate ^= 1;
		q.NextTok();
	}
	n->tok2.id = negate;
	if ((q.Tok().id & RELOP) == 0) error("Expected relational operator. Found: "+q.Tok().val);
	n->tok1 = q.Tok();
	q.NextTok();
	if (n->tok1.id == KW_LIKE) {
		n->node2 = newNode(N_VALUE,  q.Tok());
		q.NextTok();
	} else if (n->tok1.id == KW_IN) {
		if (q.Tok().id != SP_LPAREN) error("Expected opening parenthesis for expression list. Found: "+q.Tok().val);
		q.NextTok();
		//n->node2 = parseExpressionList(q,true);
		if (q.Tok().id != SP_RPAREN) error("Expected closing parenthesis after expression list. Found: "+q.Tok().val);
		q.NextTok();
	} else {
		n->node2 = parseExprAdd(q);
	}
	if (n->tok1.id == KW_BETWEEN) {
		q.NextTok();
		n->node3 = parseExprAdd(q);
	}
	return n;
}

//node1 is case expression
//node2 is next exprlist node
Node* parseCaseWhenExprList(QuerySpecs &q) {
	Node* n = newNode(N_CWEXPRLIST);
	n->node1 = parseCaseWhenExpr(q);
	if (q.Tok().Lower() == "when")
		n->node2 = parseCaseWhenExprList(q);
	return n;
}

//node1 is comparison expression
//node2 is result expression
Node* parseCaseWhenExpr(QuerySpecs &q) {
	Node* n = newNode(N_CWEXPR);
	q.NextTok(); //eat when token
	n->node1 = parseExprAdd(q);
	q.NextTok(); //eat then token
	n->node2 = parseExprAdd(q);
	return n;
}

//row limit at front of query
void parseTop(QuerySpecs &q) {
	if (q.Tok().Lower() == "top") {
		Token tok = q.NextTok();
		if (!is_number(tok.val)) error("Expected number after 'top'. Found "+q.PeekTok().val);
		q.quantityLimit = atoi(q.NextTok().val.c_str());
		q.NextTok();
	}
}

//row limit at end of query
void parseLimit(QuerySpecs &q) {
	if (q.Tok().Lower() == "limit") {
		Token tok = q.NextTok();
		if (!is_number(tok.val)) error("Expected number after 'limit'. Found "+q.PeekTok().val);
		q.quantityLimit = atoi(q.NextTok().val.c_str());
		q.NextTok();
	}
}

//tok1 is file path
//tok2 is alias
//tok3 is noheader
//node1 is joins
Node* parseFrom(QuerySpecs &q) {
	Node* n = newNode(N_FROM);
	if (q.Tok().Lower() != "from") error("Expected 'from'. Found: "+q.Tok().val);
	string filepath = boost::replace_all_copy(q.NextTok().val, "~/", string(getenv("HOME"))+"/");
	n->tok1.val = filepath;
	if ( access( n->tok1.val.c_str(), F_OK ) == -1 )
		error("Could not open file "+n->tok1.val);
	q.NextTok();
	Token t = q.Tok();
	if (t.Lower() == "noheader" || t.Lower() == "nh") {
		n->tok3 = t;
		t = q.NextTok();
	}
	switch (t.id) {
	case KW_AS:
		t = q.NextTok();
		if (t.id != WORD) error("Expected alias after as. Found: "+t.val);
	case WORD:
		n->tok2 = t;
		q.NextTok();
	}
	if (q.Tok().Lower() == "noheader" || q.Tok().Lower() == "nh") {
		n->tok3 = q.Tok();
		q.NextTok();
	}
	n->node1 = parseJoin(q);
	return n;
}

//tok1 is join details (left/outer or inner)
//tok2 is join token (join,sjoin,bjoin)
//tok3 is filepath. id==0 means non-override big file
//tok4 is alias
//tok5 is noheader
//node1 is join condition (predicates)
//node2 is next join
Node* parseJoin(QuerySpecs &q) {
	Node* n = newNode(N_JOIN);
	string s = q.Tok().Lower();
	if (joinMap.count(s) == 0)
		return nullptr;
	q.joining = true;
	if (s == "left" || s == "inner" || s == "outer"){
		n->tok1 = q.Tok();
		q.NextTok();
	}
	bool sizeOverride = false;
	s = q.Tok().Lower();
	if (s == "join" || s == "sjoin" || s == "bjoin"){
		n->tok2 = q.Tok();
		q.NextTok();
	} else {
		error("Expected 'join'. Found:"+q.Tok().val);
	}
	//file path
	n->tok3 = q.NextTok();
    struct stat stat_buf;
    int finfo = stat(n->tok3.val.c_str(), &stat_buf);
    finfo = finfo == 0 ? stat_buf.st_size : -1;
	if (finfo == -1)
		error("Could not open file "+n->tok3.val);
	//see if file is 100+ megabytes
	if (!sizeOverride && finfo > 100000000)
		n->tok3.id = 0;
	//alias
	Token t = q.NextTok();
	if (t.Lower() == "noheader" || t.Lower() == "nh") {
		n->tok5 = t;
		t = q.NextTok();
	}
	switch (t.id) {
	case KW_AS:
		t = q.NextTok();
		if (t.id != WORD) error("Expected alias after as. Found: "+t.val);
	case WORD:
		n->tok4 = t;
		break;
	default:
		error("Join requires an alias. Found: "+q.Tok().val);
	}
	if (q.PeekTok().Lower() == "noheader" || q.PeekTok().Lower() == "nh") {
		n->tok5 = q.Tok();
		q.NextTok();
	}
	if (q.NextTok().Lower() != "on") error("Expected 'on'. Found: "+q.Tok().val);
	q.NextTok();
	n->node1 = parseJoinPredicates(q);
	n->node2 = parseJoin(q);
	return n;
}

//node1 is conditions
Node* parseWhere(QuerySpecs &q) {
	if (q.Tok().Lower() != "where") { return nullptr; }
	Node* n = newNode(N_WHERE);
	q.NextTok();
	n->node1 = parsePredicates(q);
	return n;
}

//node1 is conditions
Node* parseHaving(QuerySpecs &q) {
	if (q.Tok().Lower() != "having") { return nullptr; }
	Node* n = newNode(N_WHERE);
	q.NextTok();
	n->node1 = parsePredicates(q);
	return n;
}

//node1 is sort expr
//tok1 is asc
Node* parseOrder(QuerySpecs &q) {
	if (q.Tok().Lower() == "order") {
		if (q.NextTok().Lower() != "by") error("Expected 'by' after 'order'. Found "+q.Tok().val);
		q.NextTok();
		Node* n = newNode(N_ORDER);
		n->node1 = parseExprAdd(q);
		if (q.Tok().Lower() == "asc") {
			n->tok1 = q.Tok();
			q.NextTok();
		}
		/*
		if _,ok := q.files["_f3"]; ok && q.joining && !q.groupby {
			return nil, ErrMsg(q.Tok(),"Non-grouping ordered queries can only join 2 files")
		}
		*/
		return n;
	}
	return nullptr;
}

//tok1 is function id
//tok2 is * for count(*)
//tok3 is distinct or cipher
//tok4 is password
//node1 is expression in parens
Node* parseFunction(QuerySpecs &q) {
	Node* n = newNode(N_FUNCTION);
	n->tok1 = q.Tok();
	q.NextTok();
	//count(*)
	if (q.NextTok().id == SP_STAR && n->tok1.id == FN_COUNT) {
		n->tok2 = q.Tok();
		q.NextTok();
	//everything else
	} else {
		if (q.Tok().Lower() == "distinct") {
			n->tok3 = q.Tok();
			q.NextTok();
		}
		bool needPrompt = true;
		Token commaOrParen;
		string cipherTok = "aes";
		string password;
		switch (n->tok1.id) {
		case FN_COALESCE:
			n->node1 = parseExpressionList(q, true);
			break;
		case FN_ENCRYPT:
		case FN_DECRYPT:
			//first param is expression to en/decrypt
			n->node1 = parseExprAdd(q);
			if (q.Tok().id == SP_COMMA) {
				//second param is cipher
				cipherTok = q.NextTok().Lower();
				commaOrParen = q.NextTok();
				if (commaOrParen.id == SP_COMMA) {
					//password is 3rd param
					password = q.NextTok().val;
					needPrompt = false;
					q.NextTok();
				} else if (commaOrParen.id != SP_RPAREN) {
					error("Expected comma or closing parenthesis after cipher in crypto function. Found: "+commaOrParen.val);
				}
			} else if (q.Tok().id != SP_RPAREN) {
				error("Expect closing parenthesis or comma after expression in crypto function. Found: "+q.Tok().val);
			}
			//if (q.password == "" && needPrompt) q.password = promptPassword();
			if (password == "") { password = q.password; }
			if (cipherTok != "rc4" && cipherTok != "aes")
				error("Second parameter to encryption function is cipher type (aes or rc4). Found: "+cipherTok+". "
						"Use aes for strong but bulky encryption, or rc4 for something a government could probably crack but takes less space.");
			n->tok3.val = cipherTok;
			n->tok4.val = password;
			break;
		case FN_INC:
			break;
		default:
			n->node1 = parseExprAdd(q);
		}
	}
	if (q.Tok().id != SP_RPAREN) error("Expected closing parenthesis after function. Found: "+q.Tok().val);
	q.NextTok();
	//groupby if aggregate function
	if ((n->tok1.id & AGG_BIT) != 0) { q.groupby = true; }
	return n;
}

//node1 is groupExpressions
Node* parseGroupby(QuerySpecs &q) {
	if (!(q.Tok().Lower() == "group" && q.PeekTok().Lower() == "by")) { return nullptr; }
	Node* n = newNode(N_GROUPBY);
	q.NextTok();
	q.NextTok();
	n->node1 = parseExpressionList(q,false);
	return n;
}

//node1 is expression
//node2 is expressionlist
Node* parseExpressionList(QuerySpecs &q, bool interdependant) { //bool afg if expression types are interdependant
	int label = N_EXPRESSIONS;
	if (interdependant) label = N_DEXPRESSIONS;
	Node* n = newNode(label);
	n->node1 = parseExprAdd(q);
	n->node2 = parseExpressionList(q, interdependant);
	return n;
}
