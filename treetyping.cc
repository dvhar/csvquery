#include "interpretor.h"

//give value nodes their initial type and look out for variables
static void typeInitialValue(querySpecs &q, unique_ptr<node> &n){
	if (n == nullptr)
		return;

	string thisVar;
	if (n->label == N_VARS){
		thisVar = n->tok1.val;
	}

	//only do leaf nodes
	if (n->label == N_VALUE && n->tok2.id != N_FUNCTION){
		string val = n->tok1.val;
		int period;
		//see if variable
		for (auto &v : q.vars)
			if (val == v.name){
				n->tok2.id = VARIABLE;
				goto donetyping;
			}
		//see if file alias
		period = val.find_first_of(".");
		int i;
		if (period != -1){
			string alias = val.substr(0, period);
			if (q.files.count(alias)){
				string column = val.substr(period+1, val.length()-period);
				shared_ptr<fileReader> f = q.files[alias];
				i = f->getColIdx(column);
				if (i != -1){
					//found column name with file alias
					n->tok1.id = i;
					n->tok2.id = COLUMN;
					n->tok3.id = f->types[i];
					n->tok4.val = alias;
					goto donetyping;
				} else if (!n->tok1.quoted && regex_match(column, cInt)){
					i = stoi(column.substr(1));
					if (i < f->numFields){
						//found column idx with file alias
						n->tok1.id = i;
						n->tok2.id = COLUMN;
						n->tok3.id = f->types[i];
						n->tok4.val = alias;
						goto donetyping;
					}
				} else if (!n->tok1.quoted && q.numIsCol() && regex_match(column, posInt)){
					i = stoi(column);
					if (i < f->numFields){
						//found column idx with file alias
						n->tok1.id = i;
						n->tok2.id = COLUMN;
						n->tok3.id = f->types[i];
						n->tok4.val = alias;
						goto donetyping;
					}
				}
			}
		}
		//no file alias
		for (auto &f : q.files){
			i = f.second->getColIdx(val);
			if (i != -1){
				//found column name without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.id = f.second->types[i];
				n->tok4.val = f.second->id;
				goto donetyping;
			}
		}
		if (!n->tok1.quoted && regex_match(val, cInt)){
			i = stoi(val.substr(1));
			auto f = q.files["_f1"];
			if (i < f->numFields){
				//found column idx without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.id = f->types[i];
				n->tok4.val = "_f1";
				goto donetyping;
			}
		} else if (!n->tok1.quoted && q.numIsCol() && regex_match(val, posInt)){
			i = stoi(val);
			auto f = q.files["_f1"];
			if (i < f->numFields){
				//found column idx without file alias
				n->tok1.id = i;
				n->tok2.id = COLUMN;
				n->tok3.id = f->types[i];
				n->tok4.val = "_f1";
				goto donetyping;
			}
		}
		//is not a var, function, or column, must be literal
		n->tok2.id = LITERAL;
		n->tok3.id = getNarrowestType((char*)val.c_str(), 0);
	//put label after } if not printing
	donetyping:
	cerr << "typed " << n->tok1.val << " as " << n->tok3.id << endl;
	}

	typeInitialValue(q, n->node1);
	//record variable after typing var leafnodes to avoid recursive definition
	if (n->label == N_VARS)
		q.addVar(thisVar);
	typeInitialValue(q, n->node2);
	typeInitialValue(q, n->node3);
	typeInitialValue(q, n->node4);

}

//do all typing
void applyTypes(querySpecs &q, unique_ptr<node> &n){
	typeInitialValue(q,n);
}

