#include "interpretor.h"
#include "vmachine.h"

static void addop(vector<opcode> &v, byte code){
	v.push_back({code, 0});
}
static void addop(vector<opcode> &v, byte code, int p1){
	v.push_back({code, p1});
}

static void determinePath(querySpecs &q){
	if (q.sorting && !q.grouping && q.joining) {
        //order join
    } else if (q.sorting && !q.grouping) {
        //ordered plain
    } else if (q.joining) {
        //normal join and grouping
    } else {
        //normal plain and grouping
    }

}
