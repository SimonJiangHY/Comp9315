// query.c ... query scan functions
// part of Multi-attribute Linear-hashed Files
// Manage creating and using Query objects
// Last modified by John Shepherd, July 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "string.h"
#include "hash.h"
#include "stdio.h"

// A suggestion ... you can change however you like

struct QueryRep {
	Reln    rel;       // need to remember Relation info
	Bits    known;     // the hash value from MAH
	Bits    unknown;   // the unknown bits from MAH
	PageID  curPage;   // current page in scan, can be overflow page
	PageID  curDataPage; // current DataPage
	int     is_ovflow; // are we in the overflow pages?
	//TODO
	Tuple   queryTuple;   // the query tuple
	PageID* vaild_page_list; // the vaild page list we need to scan
	int  vaild_page_nums; // number of valid pages 
	int list_index;    // the index of page_list
	int last_list_index;
	Offset  curTuple;
	Offset  tupleIndex;

};

int Power(int depth){
	int result = 1;
	while (depth > 0){
		result *= 2;
		depth --;
	}

	return result;
}

// the quick sort function is copied from web
void QuickSort(PageID* arr, int size){
    int temp, i, j;
    for(i = 1; i <size; i++)
    for(j=i; j>0; j--){
        if(arr[j] <arr[j-1]){
            temp = arr[j];
            arr[j]=arr[j-1];
            arr[j-1]=temp;
        }
    }
}

// get all possible PageID by known and unknown
// then store then in Array
void bitRecursion(Bits known,Bits unknown,int position,int depth,PageID* list,int page_num){
	if (position == depth){
		for(int i =0;i<page_num;i++){
			if (list[i]==9999999){
				list[i] = known;
				break;
			}
		}
	}else{
        if (bitIsSet(unknown,position)==1){
			bitRecursion(setBit(known,position),unknown,position+1,depth,list,page_num); 
	    }
		bitRecursion(known,unknown,position+1,depth,list,page_num);
	}
}

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan
Query startQuery(Reln r, char *q)
{
	Query new = malloc(sizeof(struct QueryRep));
	assert(new != NULL);
	// TODO
	// Partial algorithm:
	// form known bits from known attributes
	// form unknown bits from '?' attributes
	// compute PageID of first page
	//   using known bits and first "unknown" value
	// set all values in QueryRep object
	Count attrNum = nattrs(r);
	char **queryAttr = malloc(attrNum*sizeof(char *));
	tupleVals(q,queryAttr);

	// ---------------
	// printf("query: %s\n",q);
	// for (int i = 0 ; i < attrNum ;i++){
	// 	printf(" %s ",queryAttr[i]);
	// }
	// printf("\ncheck query Array over\n");
	// -----------------

	Bits oneBit,attrBits;
	int i,a,b,d;
	d = MAXCHVEC;
	ChVecItem *cv = chvec(r);
    for (i=0;i<d;i++){
		a = cv[i].att;
		b = cv[i].bit;
		if (strcmp(queryAttr[a],"?")!=0){
			attrBits = hash_any((unsigned char *)queryAttr[a],strlen(queryAttr[a]));
			oneBit = bitIsSet(attrBits,b);
			new->known = new->known | (oneBit << i);
            new->unknown = new->unknown | (0 << i);
		} else {
			new->known = new->known | (0 << i);
			new->unknown = new->unknown | (1 << i); 
		}		
	}

    // ----------------
	// char buf_1[MAXBITS+1];
	// char buf_2[MAXBITS+1];
	// bitsString(new->known,buf_1);
	// printf("known   %s\n",buf_1);
	// bitsString(new->unknown,buf_2);
	// printf("unknown %s\n",buf_2);
	//---------------

    Count r_depth = depth(r); 
	new->known = getLower(new->known,r_depth+1);
	new->unknown = getLower(new->unknown,r_depth+1);
	int uk_num = 0;
	for (int i = 0; i<r_depth+1;i++){
		if (bitIsSet(new->unknown,i)==1){
			uk_num++;
		}
	}

    //-----------------
	// printf("unknown bit number %d\n",uk_num);
	// char buf_3[MAXBITS+1];
	// char buf_4[MAXBITS+1];
	// bitsString(new->known,buf_3);
	// printf("getlow known   %s\n",buf_3);
	// bitsString(new->unknown,buf_4);
	// printf("getlow unknown %s\n",buf_4);
	//-------------------
	PageID* PageID_List;
	int page_num = Power(uk_num);
	if (bitIsSet(new->known,r_depth)==0){
	    PageID_List = malloc(page_num *sizeof(PageID));
	    for (int i=0;i<page_num;i++){
		    PageID_List[i]= 9999999;
	    }
	    bitRecursion(new->known,new->unknown,0,r_depth+1,PageID_List,page_num);
	}else{
		page_num = page_num *2;
		PageID_List = malloc(page_num*sizeof(PageID));
		
		for (int i=0;i<page_num;i++){
		    PageID_List[i]= 9999999;
	    }
		bitRecursion(new->known,new->unknown,0,r_depth+1,PageID_List,page_num);
		bitRecursion(unsetBit(new->known,r_depth),new->unknown,0,r_depth+1,PageID_List,page_num);
	}
	QuickSort(PageID_List,page_num);
	new->last_list_index = page_num-1;
	for (int i=0;i<page_num;i++){
		if (PageID_List[i]>(npages(r)-1)){
			new->last_list_index = i-1;
			break;
		}
	}

	new->rel = r;
    new->queryTuple = copyString(q);
	new->vaild_page_list = PageID_List;
	new->vaild_page_nums = page_num;
	new->curTuple = 0;
	new->tupleIndex = 0;
	new->list_index = 0;
	new->curDataPage = new->vaild_page_list[new->list_index];
	new->curPage = new->vaild_page_list[new->list_index];
	new->is_ovflow = 0;

    // printf("page number is %d\n",npages(r)-1);
	// printf("last list index %d and its id %d \n",new->last_list_index,new->vaild_page_list[new->last_list_index]);
    // //--------------------
    // printf("%s\n",new->queryTuple);
	// //printf("max pageid %d\n",new->max_PageID);
	// printf("first page %d\n",new->curDataPage);
	// printf("Vaild PageIDs:\n");
	// for (int i=0;i<page_num;i++){
	// 	printf("%d ",new->vaild_page_list[i]);
	// }
	// printf("\n");	
    //-------------------
	freeVals(queryAttr,attrNum);
	return new;
}



Tuple getNextTuple(Query q)
{
	// TODO
	// Partial algorithm:
	// if (more tuples in current page)
	//    get next matching tuple from current page
	// else if (current page has overflow)
	//    move to overflow page
	//    grab first matching tuple from page
	// else
	//    move to "next" bucket
	//    grab first matching tuple from data page
	// endif
	// if (current page has no matching tuples)
	//    go to next page (try again)
	// endif
	for (;q->list_index<=q->last_list_index;q->list_index++){

		//printf("-----------scaning page %d----------\n",q->vaild_page_list[q->list_index]);
        q->curDataPage = q->vaild_page_list[q->list_index];
		Page page = NULL;
		if (q->is_ovflow==0){
		    page = getPage(dataFile(q->rel),q->curPage);
		} else {
			page = getPage(ovflowFile(q->rel),q->curPage);
		}
		Count page_tuple_num = pageNTuples(page);
		if (q->tupleIndex < page_tuple_num){
            for (;q->tupleIndex <page_tuple_num; q->tupleIndex++){
	            char *t = pageData(page) + q->curTuple;
			    //printf("tuple %d totalnum %d --- %s match %s\n",q->tupleIndex,page_tuple_num,q->queryTuple,t);
	            q->curTuple += strlen(t)+1;
	            if (tupleMatch(q->rel,q->queryTuple,t)){
					q->tupleIndex++;    
			        return t;
		        }
			}
		}
		PageID ovp= NO_PAGE;
		ovp = pageOvflow(page);
		while (ovp!=NO_PAGE){
			q->tupleIndex=0;
		    q->curTuple=0;
			q->is_ovflow = 1;
			Page ovpage = getPage(ovflowFile(q->rel),ovp);
			q->curPage = ovp;
			//printf("over flow page id %d",q->curPage);
			Count ovpage_tuple_num = pageNTuples(ovpage);
			if (q->tupleIndex < ovpage_tuple_num){
				for (;q->tupleIndex<ovpage_tuple_num;q->tupleIndex++){
					char *ovt = pageData(ovpage) + q->curTuple;
					q->curTuple += strlen(ovt)+1;
					//printf("ovp tuple %d totalnum %d --- %s match %s\n",q->tupleIndex,ovpage_tuple_num,q->queryTuple,ovt);
					if (tupleMatch(q->rel,q->queryTuple,ovt)){
						q->tupleIndex++;
						return ovt;
					} 
				}
			}
			ovp = pageOvflow(ovpage);
		}
    	q->curDataPage = q->vaild_page_list[q->list_index+1];
		q->curPage = q->vaild_page_list[q->list_index+1];
		q->tupleIndex = 0;
		q->curTuple = 0;
		q->is_ovflow = 0;		
	}
    
	return NULL;
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	// TODO
	free(q->vaild_page_list);
	free(q);
}

