#include "interpretor.h"

static void addint(vector<byte> &v, int i){
	v.push_back(0);
	void* p = &v.back();
	v.resize(v.size()+3);
	*((int*)p) = i;
}
static void addop(vector<byte> &v, byte code){
	v.push_back(code);
}
static void addop(vector<byte> &v, byte code, int p1){
	v.push_back(code);
	addint(v,p1);
}
static void addop(vector<byte> &v, byte code, int p1, int p2){
	v.push_back(code);
	addint(v,p1);
	addint(v,p2);
}
static void addop(vector<byte> &v, byte code, int p1, int p2, int p3){
	v.push_back(code);
	addint(v,p1);
	addint(v,p2);
	addint(v,p3);
}
static void addop(vector<byte> &v, byte code, int p1, int p2, int p3, int p4){
	v.push_back(code);
	addint(v,p1);
	addint(v,p2);
	addint(v,p3);
	addint(v,p4);
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
