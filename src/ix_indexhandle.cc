//
// File:        ix_indexhandle.cc
// Description: IX_IndexHandle handles manipulations within the index
// Author:      Yifei Huang (yifei@stanford.edu)
//

#include <unistd.h>
#include <sys/types.h>
#include "pf.h"
#include "ix_internal.h"
#include <math.h>
#include "comparators.h"
#include <cstdio>


IX_IndexHandle::IX_IndexHandle(){
  isOpenHandle = false;
  header_modified = false;

  splittwice = false;
}

IX_IndexHandle::~IX_IndexHandle(){

}

RC IX_IndexHandle::PrintRootPage(){
  printf("PRINTING ROOT PAGE\n");
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader)))
    return (rc);

  struct Node_Entry *entries = (struct Node_Entry *)((char *)rHeader + header.entryOffset_N);
  char *keys = (char *)rHeader + header.keysOffset_N;

  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = rHeader->firstSlotIndex;
  while(curr_idx != NO_MORE_SLOTS){
    printf("Key: %d, Page: %d \n", *(int *)(keys + curr_idx * header.attr_length), entries[curr_idx].page);
    prev_idx = curr_idx;
    curr_idx = entries[prev_idx].nextSlot;
  }

  return (0);
}


RC IX_IndexHandle::PrintLeafNodesString(PageNum curr_page){
  RC rc = 0;
  PF_PageHandle ph;
  struct IX_NodeHeader_L *LHeader;
  PageNum thispage = curr_page;
  char *prevKey = (char *)malloc(header.attr_length);
  int numcompared = 0;
  while(thispage != NO_MORE_PAGES){
    printf("*****printing leafnode %d \n", thispage);
    if((rc = pfh.GetThisPage(thispage, ph)) || ph.GetData((char *&)LHeader)){
      printf("return here\n");
      return (rc);
    }
    struct Node_Entry *entries = (struct Node_Entry *)((char *)LHeader + header.entryOffset_N);
    char *keys = (char *)LHeader + header.keysOffset_N;
    int prev_idx = BEGINNING_OF_SLOTS;
    int curr_idx = LHeader->firstSlotIndex;
    bool isfirstcomp = true;
    //int prevkey = *(int *)(keys + header.attr_length*curr_idx);
    //printf("first slot index: %d \n", curr_idx);
    while(curr_idx != NO_MORE_SLOTS){
      if(entries[curr_idx].isValid == OCCUPIED_NEW){
        //printf("\nNew Entry: %d, page: %d, slot: %d , index: %d \n", *(int*)(keys + curr_idx*header.attr_length), 
        //  entries[curr_idx].page, entries[curr_idx].slot, curr_idx);
        //int curr_key = *(int *)(keys + curr_idx*header.attr_length);
        char * curr_key = keys + curr_idx * header.attr_length;
        numcompared++;
        //printf("compare");
        if(strncmp(curr_key, prevKey, header.attr_length) < 0 && isfirstcomp == false){
          printf("------------------------------------------------------ERROR \n");
          if(splittwice)
            printf("----------------------------------------------------split twice!!!\n");
          //PrintRootPage();
          return (IX_EOF);
        }
        isfirstcomp = false;
        memcpy(prevKey, curr_key, header.attr_length);
      }
      else if(entries[curr_idx].isValid == OCCUPIED_DUP){
        //printf("\nDup Entry: %d \n", *(int *)(keys + curr_idx * header.attr_length));
        //printf("bucket location: %d \n", entries[curr_idx].page);
        PrintBuckets(entries[curr_idx].page);
      }
      else{
        free(prevKey);
        printf("invalid spot\n");
        return (IX_EOF);
      }
      prev_idx = curr_idx;
      curr_idx = entries[prev_idx].nextSlot;
      //printf("next index: %d \n", curr_idx);
      //printf("curr_slot: %d \n", curr_idx);
    }
    //printf("prev page: %d \n", prevpage);
    PageNum prevpage = thispage;
    thispage = LHeader->nextPage;
    //printf("next page: %d \n", thispage);
    if((rc = pfh.UnpinPage(prevpage)))
      return (rc);
    //if(splittwice)
    //  printf("THERE is a page 4\n");
  }
  free(prevKey);
  printf("numcompared %d \n", numcompared);
  return (0);
}

RC IX_IndexHandle::PrintAllEntriesString(){
  //printf("PRINTING ALL ENTRIES\n");
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader)))
    return (rc);

  if(! rHeader->isLeafNode){
    PageNum rootNodeNum;
    rootPH.GetPageNum(rootNodeNum);
    //printf("root node %d \n", rootNodeNum);
    PageNum curr_page = rHeader->invalid1;
    PF_PageHandle ph;
    struct IX_NodeHeader *header;
    if((pfh.GetThisPage(curr_page, ph)) || (rc = ph.GetData((char *&)header)))
      return (rc);
    while(! header->isLeafNode){
      PageNum prev_page = curr_page;
      curr_page = header->invalid1;
      if((rc = pfh.UnpinPage(prev_page)))
        return (rc);
      if((pfh.GetThisPage(curr_page, ph)) || (rc = ph.GetData((char *&)header)))
        return (rc);
    }
    if(pfh.UnpinPage(curr_page))
      return (rc);
    //printf("rawr\n");
    PrintLeafNodesString(curr_page);
    //PrintLeafNodes(curr_page);

  }
  else{
    if((rc = PrintLeafNodesString(header.rootPage)))
      return (rc);
  }
  return (0);
}


RC IX_IndexHandle::PrintAllEntries(){
  //printf("PRINTING ALL ENTRIES\n");
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader)))
    return (rc);

  if(! rHeader->isLeafNode){
    PageNum rootNodeNum;
    rootPH.GetPageNum(rootNodeNum);
    //printf("root node %d \n", rootNodeNum);
    PageNum curr_page = rHeader->invalid1;
    PF_PageHandle ph;
    struct IX_NodeHeader *header;
    if((pfh.GetThisPage(curr_page, ph)) || (rc = ph.GetData((char *&)header)))
      return (rc);
    while(! header->isLeafNode){
      PageNum prev_page = curr_page;
      curr_page = header->invalid1;
      if((rc = pfh.UnpinPage(prev_page)))
        return (rc);
      if((pfh.GetThisPage(curr_page, ph)) || (rc = ph.GetData((char *&)header)))
        return (rc);
    }
    if(pfh.UnpinPage(curr_page))
      return (rc);
    //printf("rawr\n");
    PrintLeafNodes(curr_page);
  }
  else{
    if((rc = PrintLeafNodes(header.rootPage)))
      return (rc);
  }
  return (0);
}

RC IX_IndexHandle::CheckAllValuesInt(PageNum curr_page){
  printf("checking all values");
  RC rc = 0;
  PF_PageHandle ph;
  struct IX_NodeHeader_L *LHeader;
  PageNum thispage = curr_page;
  int prevkey = 0;
  while(thispage != NO_MORE_PAGES){
    //printf("*****printing leafnode %d \n", thispage);
    if((rc = pfh.GetThisPage(thispage, ph)) || ph.GetData((char *&)LHeader)){
      return (rc);
    }
    struct Node_Entry *entries = (struct Node_Entry *)((char *)LHeader + header.entryOffset_N);
    char *keys = (char *)LHeader + header.keysOffset_N;
    int prev_idx = BEGINNING_OF_SLOTS;
    int curr_idx = LHeader->firstSlotIndex;
    //int prevkey = *(int *)(keys + header.attr_length*curr_idx);
    //printf("first slot index: %d \n", curr_idx);
    while(curr_idx != NO_MORE_SLOTS){
      if(entries[curr_idx].isValid == OCCUPIED_NEW){
        printf("checking\n");
        //printf("\nNew Entry: %d, page: %d, slot: %d , index: %d \n", *(int*)(keys + curr_idx*header.attr_length), 
        //  entries[curr_idx].page, entries[curr_idx].slot, curr_idx);
        //printf("compare");
        int curr_key = *(int *)(keys + curr_idx*header.attr_length);
        printf("currkey: %d \n", curr_key);
        if(curr_key != (prevkey +1)){
          printf("------------------------------------------------------ERROR \n");
          //if(splittwice)
          //  printf("----------------------------------------------------split twice!!!\n");
          //PrintRootPage();
          return (IX_EOF);
        }
        prevkey = curr_key;
      }
      else if(entries[curr_idx].isValid == OCCUPIED_DUP){
        //printf("\nDup Entry: %d \n", *(int *)(keys + curr_idx * header.attr_length));
        //printf("bucket location: %d \n", entries[curr_idx].page);
        PrintBuckets(entries[curr_idx].page);
      }
      else{
        printf("error here\n");
        return (IX_EOF);
      }
      prev_idx = curr_idx;
      curr_idx = entries[prev_idx].nextSlot;
      //printf("next index: %d \n", curr_idx);
      //printf("curr_slot: %d \n", curr_idx);
    }
    //printf("prev page: %d \n", prevpage);
    PageNum prevpage = thispage;
    thispage = LHeader->nextPage;
    //printf("next page: %d \n", thispage);
    if((rc = pfh.UnpinPage(prevpage)))
      return (rc);
    //if(splittwice)
    //  printf("THERE is a page 4\n");
  }
  return (0);
}

RC IX_IndexHandle::PrintLeafNodes(PageNum curr_page){
  RC rc = 0;
  PF_PageHandle ph;
  struct IX_NodeHeader_L *LHeader;
  PageNum thispage = curr_page;
  int prevkey = 0;
  while(thispage != NO_MORE_PAGES){
    //printf("*****printing leafnode %d \n", thispage);
    if((rc = pfh.GetThisPage(thispage, ph)) || ph.GetData((char *&)LHeader)){
      return (rc);
    }
    struct Node_Entry *entries = (struct Node_Entry *)((char *)LHeader + header.entryOffset_N);
    char *keys = (char *)LHeader + header.keysOffset_N;
    int prev_idx = BEGINNING_OF_SLOTS;
    int curr_idx = LHeader->firstSlotIndex;
    //int prevkey = *(int *)(keys + header.attr_length*curr_idx);
    //printf("first slot index: %d \n", curr_idx);
    while(curr_idx != NO_MORE_SLOTS){
      if(entries[curr_idx].isValid == OCCUPIED_NEW){
        //printf("\nNew Entry: %d, page: %d, slot: %d , index: %d \n", *(int*)(keys + curr_idx*header.attr_length), 
        //  entries[curr_idx].page, entries[curr_idx].slot, curr_idx);
        //printf("compare");
        int curr_key = *(int *)(keys + curr_idx*header.attr_length);
        if(curr_key < (prevkey )){
          printf("------------------------------------------------------ERROR \n");
          //if(splittwice)
          //  printf("----------------------------------------------------split twice!!!\n");
          //PrintRootPage();
          return (IX_EOF);
        }
        prevkey = curr_key;
      }
      else if(entries[curr_idx].isValid == OCCUPIED_DUP){
        //printf("\nDup Entry: %d \n", *(int *)(keys + curr_idx * header.attr_length));
        //printf("bucket location: %d \n", entries[curr_idx].page);
        PrintBuckets(entries[curr_idx].page);
      }
      else
        return (IX_EOF);
      prev_idx = curr_idx;
      curr_idx = entries[prev_idx].nextSlot;
      //printf("next index: %d \n", curr_idx);
      //printf("curr_slot: %d \n", curr_idx);
    }
    //printf("prev page: %d \n", prevpage);
    PageNum prevpage = thispage;
    thispage = LHeader->nextPage;
    //printf("next page: %d \n", thispage);
    if((rc = pfh.UnpinPage(prevpage)))
      return (rc);
    //if(splittwice)
    //  printf("THERE is a page 4\n");
  }
  return (0);
}

RC IX_IndexHandle::PrintBuckets(PageNum page){
  RC rc = 0;
  PF_PageHandle ph;
  struct IX_BucketHeader *bHeader;
  PageNum thispage = page;
  while(thispage != NO_MORE_PAGES){
    printf("print bucket: %d \n", thispage);
    if((rc = pfh.GetThisPage(thispage, ph)) || (rc = ph.GetData((char *&)bHeader)))
      return (rc);
    struct Bucket_Entry *entries = (struct Bucket_Entry *)((char*)bHeader + header.entryOffset_B);
    int prev_idx = BEGINNING_OF_SLOTS;
    int curr_idx = bHeader->firstSlotIndex;
    //printf("first index: %d \n", curr_idx);
    while(curr_idx != NO_MORE_SLOTS){
      printf(" page %d slot %d | " , entries[curr_idx].page, entries[curr_idx].slot);
      prev_idx = curr_idx;
      curr_idx = entries[prev_idx].nextSlot;
    }
    PageNum prevpage = thispage;
    thispage = bHeader->nextBucket;
    if((rc = pfh.UnpinPage(prevpage)))
      return (rc);
    //printf("this page : %d \n", thispage);
  }
  return (0);

}

RC IX_IndexHandle::InsertEntry(void *pData, const RID &rid){
  if(! isValidIndexHeader() || isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader))){
    printf("failing here\n");
    return (rc);
  }

  if(rHeader->num_keys == header.maxKeys_N){
    printf("splitting the topmost node\n");
    PageNum newRootPage;
    char *newRootData;
    PF_PageHandle newRootPH;
    if((rc = CreateNewNode(newRootPH, newRootPage, newRootData, false)))
      return (rc);
    struct IX_NodeHeader_I *newRootHeader = (struct IX_NodeHeader_I *)newRootData;
    newRootHeader->isEmpty = false;
    newRootHeader->firstPage = header.rootPage;

    int unused;
    PageNum unusedPage;
    if((rc = SplitNode((struct IX_NodeHeader *&)newRootData, (struct IX_NodeHeader *&)rHeader, header.rootPage, BEGINNING_OF_SLOTS, unused, unusedPage)))
      return (rc);
    if((rc = pfh.MarkDirty(header.rootPage)) || (rc = pfh.UnpinPage(header.rootPage)))
      return (rc);
    rootPH = newRootPH; // reset root
    header.rootPage = newRootPage;
    header_modified = true;
    struct IX_NodeHeader *useMe;
    if((rc = newRootPH.GetData((char *&)useMe))){
      return (rc);
    }
    if((rc = InsertIntoNonFullNode(useMe, header.rootPage, pData, rid)))
      return (rc);
  }
  else{
    //printf("reached here\n");
    if((rc = InsertIntoNonFullNode(rHeader, header.rootPage, pData, rid)))
      return (rc);
    //printf("reached insert entry\n");
    //if((rc = InsertIntoNonFullNode((char *&)rHeader,pData, rid)))
    //  return (rc);
  }

  if((rc = pfh.MarkDirty(header.rootPage)))
    return (rc);

  //PrintAllEntriesString();
  return (rc);
}

// index refers to prev index in parent
RC IX_IndexHandle::SplitNode(struct IX_NodeHeader *pHeader, struct IX_NodeHeader *oldHeader, 
  PageNum oldPage, int index, int & newKeyIndex, PageNum &newPageNum){
  RC rc = 0;
  //printf("********* SPLIT ********* at index %d \n", index);
  bool isLeaf = false;
  if(oldHeader->isLeafNode == true){
    //printf("yes is leaf!\n");
    isLeaf = true;
  }
  PageNum newPage;
  struct IX_NodeHeader *newHeader;
  PF_PageHandle newPH;
  if((rc = CreateNewNode(newPH, newPage, (char *&)newHeader, isLeaf))){
    return (rc);
  }
  newPageNum = newPage;
  //printf("NEWPAGENUM: %d \n", newPageNum);
  struct Node_Entry *pEntries = (struct Node_Entry *) ((char *)pHeader + header.entryOffset_N);
  struct Node_Entry *oldEntries = (struct Node_Entry *) ((char *)oldHeader + header.entryOffset_N);
  struct Node_Entry *newEntries = (struct Node_Entry *) ((char *)newHeader + header.entryOffset_N);
  char *pKeys = (char *)pHeader + header.keysOffset_N;
  char *newKeys = (char *)newHeader + header.keysOffset_N;
  char *oldKeys = (char *)oldHeader + header.keysOffset_N;

  int prev_idx1 = BEGINNING_OF_SLOTS;
  int curr_idx1 = oldHeader->firstSlotIndex;
  for(int i=0; i < header.maxKeys_N/2 ; i++){
    prev_idx1 = curr_idx1;
    curr_idx1 = oldEntries[prev_idx1].nextSlot;
  }

  oldEntries[prev_idx1].nextSlot = NO_MORE_SLOTS;
  char *parentKey = oldKeys + curr_idx1*header.attr_length;
  
  char * tempchar = (char *)malloc(header.attr_length);
  memcpy(tempchar, parentKey, header.attr_length);
  //printf("split key: %s \n", tempchar);
  free(tempchar);

  if(!isLeaf){ // update the keys
    struct IX_NodeHeader_I *newIHeader = (struct IX_NodeHeader_I *)newHeader;
    newIHeader->firstPage = oldEntries[curr_idx1].page;
    newIHeader->isEmpty = false;
    prev_idx1 = curr_idx1;
    curr_idx1 = oldEntries[prev_idx1].nextSlot;
    oldEntries[prev_idx1].nextSlot = oldHeader->freeSlotIndex;
    oldHeader->freeSlotIndex = prev_idx1;
    oldHeader->num_keys--;
  }
  //prev_idx1 = curr_idx1;

  int prev_idx2 = BEGINNING_OF_SLOTS;
  int curr_idx2 = newHeader->freeSlotIndex;
  while(curr_idx1 != NO_MORE_SLOTS){
    //printf("current_index2: %d \n", curr_idx2);
    //printf("curr_index1: %d \n", curr_idx1);
    newEntries[curr_idx2].page = oldEntries[curr_idx1].page;
    newEntries[curr_idx2].slot = oldEntries[curr_idx1].slot;
    newEntries[curr_idx2].isValid = oldEntries[curr_idx1].isValid;
    //if(oldEntries[curr_idx1].isValid != OCCUPIED_NEW)
    //  printf("-----------ERRORRRRRR ------------\n");
    memcpy(newKeys + curr_idx2*header.attr_length, oldKeys + curr_idx1*header.attr_length, header.attr_length);
    if(prev_idx2 == BEGINNING_OF_SLOTS){
      newHeader->freeSlotIndex = newEntries[curr_idx2].nextSlot;
      newEntries[curr_idx2].nextSlot = newHeader->firstSlotIndex;
      newHeader->firstSlotIndex = curr_idx2;
    } 
    else{
      newHeader->freeSlotIndex = newEntries[curr_idx2].nextSlot;
      newEntries[curr_idx2].nextSlot = newEntries[prev_idx2].nextSlot;
      newEntries[prev_idx2].nextSlot = curr_idx2;
    }
    prev_idx2 = curr_idx2;  
    curr_idx2 = newHeader->freeSlotIndex; // update insert index

   // newEntries[curr_idx1].isValid = UNOCCUPIED;
    //entries1[curr_idx1].nextSlot = oldBHeader->freeSlotIndex;
    prev_idx1 = curr_idx1;
    curr_idx1 = oldEntries[prev_idx1].nextSlot;
    oldEntries[prev_idx1].nextSlot = oldHeader->freeSlotIndex;
    oldHeader->freeSlotIndex = prev_idx1;
    oldHeader->num_keys--;
    newHeader->num_keys++;
  }

  // insert parent key into parent at index i
  int loc = pHeader->freeSlotIndex;
  memcpy(pKeys + loc * header.attr_length, parentKey, header.attr_length);
  newKeyIndex = loc;
  //printf("new key location: %d \n", newKeyIndex);
  //printf("next free key: %d \n", pEntries[newKeyIndex].nextSlot);
  pEntries[loc].page = newPage;
  pEntries[loc].isValid = OCCUPIED_NEW;
  if(index == BEGINNING_OF_SLOTS){
    pHeader->freeSlotIndex = pEntries[loc].nextSlot;
    pEntries[loc].nextSlot = pHeader->firstSlotIndex;
    pHeader->firstSlotIndex = loc;
  }
  else{
    pHeader->freeSlotIndex = pEntries[loc].nextSlot;
    pEntries[loc].nextSlot = pEntries[index].nextSlot;
    pEntries[index].nextSlot = loc;
  }
  pHeader->num_keys++;

  // if is leaf node, update the page pointers
  if(isLeaf){
    struct IX_NodeHeader_L *newLHeader = (struct IX_NodeHeader_L *) newHeader;
    struct IX_NodeHeader_L *oldLHeader = (struct IX_NodeHeader_L *) oldHeader;
    newLHeader->nextPage = oldLHeader->nextPage;
    newLHeader->prevPage = oldPage;
    oldLHeader->nextPage = newPage;
    //printf("oldLHeader->nextPage : %d \n", newPage);
    if(newLHeader->nextPage != NO_MORE_PAGES){
      PF_PageHandle nextPH;
      struct IX_NodeHeader_L *nextHeader;
      if((rc = pfh.GetThisPage(newLHeader->nextPage, nextPH)) || (nextPH.GetData((char *&)nextHeader)))
        return (rc);
      nextHeader->prevPage = newPage;
      if((rc = pfh.MarkDirty(newLHeader->nextPage)) || (rc = pfh.UnpinPage(newLHeader->nextPage)))
        return (rc);
    }
  }

  if((rc = pfh.MarkDirty(newPage))||(rc = pfh.UnpinPage(newPage))){
    return (rc);
  }
  //PrintLeafNodes(newPage);
  //printf("****finishsplit***\n");
  return (rc);
}

RC IX_IndexHandle::InsertIntoBucket(PageNum page, const RID &rid){
  RC rc = 0;
  PageNum ridPage;
  SlotNum ridSlot;
  if((rc = rid.GetPageNum(ridPage)) || (rc = rid.GetSlotNum(ridSlot))){
    return (rc);
  }

  bool notEnd = true;
  PageNum currPage = page;
  PF_PageHandle bucketPH;
  struct IX_BucketHeader *bucketHeader;
  while(notEnd){
    if((rc = pfh.GetThisPage(currPage, bucketPH)) || (rc = bucketPH.GetData((char *&)bucketHeader))){
      return (rc);
    }
    // Try to find the entry in the database
    struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bucketHeader + header.entryOffset_B);
    int prev_idx = BEGINNING_OF_SLOTS;
    int curr_idx = bucketHeader->firstSlotIndex;
    while(curr_idx != NO_MORE_SLOTS){
      if(entries[curr_idx].page == ridPage && entries[curr_idx].slot == ridSlot){
        RC rc2 = 0;
        if((rc2 = pfh.UnpinPage(currPage)))
          return (rc2);
        return (IX_DUPLICATEENTRY);
      }
      prev_idx = curr_idx;
      curr_idx = entries[prev_idx].nextSlot;
    }
    // INSERT if it's the last bucket
    //printf("number of nodes in bucket: %d \n", bucketHeader->num_keys);
    if(bucketHeader->nextBucket == NO_MORE_PAGES && bucketHeader->num_keys == header.maxKeys_B){
      notEnd = false;
      PageNum newBucketPage;
      PF_PageHandle newBucketPH;
      if((rc = CreateNewBucket(newBucketPage)))
        return (rc);
      bucketHeader->nextBucket = newBucketPage;
      if((rc = pfh.MarkDirty(currPage)) || (rc = pfh.UnpinPage(currPage)))
        return (rc);
      currPage = newBucketPage;
      if((rc = pfh.GetThisPage(currPage, bucketPH)) || (rc = bucketPH.GetData((char *&)bucketHeader)))
        return (rc);
      entries = (struct Bucket_Entry *)((char *)bucketHeader + header.entryOffset_B);
    }
    if(bucketHeader->nextBucket == NO_MORE_PAGES){
      notEnd = false;
      //printf("reached bucket insertion in bucket number %d \n", currPage);
      int loc = bucketHeader->freeSlotIndex;
      entries[loc].slot = ridSlot;
      entries[loc].page = ridPage;
      bucketHeader->freeSlotIndex = entries[loc].nextSlot;
      entries[loc].nextSlot = bucketHeader->firstSlotIndex;
      bucketHeader->firstSlotIndex = loc;
      //printf("first slot: %d, next slot%d \n", bucketHeader->firstSlotIndex, 
      //  entries[bucketHeader->firstSlotIndex]);
      bucketHeader->num_keys++;
      //printf("no more slots %d \n", NO_MORE_SLOTS);
    }


    PageNum nextPage = bucketHeader->nextBucket;
    if((rc = pfh.MarkDirty(currPage)) || (rc = pfh.UnpinPage(currPage)))
      return (rc);
    currPage = nextPage;
  }
  return (0);
}

/*
RC IX_IndexHandle::SearchEntry(void *pData, const RID &rid){
  if(! isValidIndexHeader() || isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader))){
    printf("failing here\n");
    return (rc);
  }

  struct IX_NodeHeader *nHeader = rHeader;
  while(! nHeader->isLeafNode){
    int searchIndex;
    bool isDup;
    if((rc = FindNodeInsertIndex(nHeader, pData, index, isDup)))
      return (rc);

    PageNum nextNodeNum;
    if(index == BEGINNING_OF_SLOTS){

    }

  }

}
*/

RC IX_IndexHandle::InsertIntoNonFullNode(struct IX_NodeHeader *nHeader, PageNum thisNodeNum, void *pData, 
  const RID &rid){
  RC rc = 0;

  struct Node_Entry *entries = (struct Node_Entry *) ((char *)nHeader + header.entryOffset_N);
  char *keys = (char *)nHeader + header.keysOffset_N;
  //printf("entries[0].nextSlot: %d \n", entries[0].nextSlot);
  if(nHeader->isLeafNode){
    int prevInsertIndex = BEGINNING_OF_SLOTS;
    bool isDup = false;
    if((rc = FindNodeInsertIndex(nHeader, pData, prevInsertIndex, isDup)))
      return (rc);
    //printf("found index: %d \n", prevInsertIndex);
    //printf("inserting after value: %d \n", *(int *)(keys + prevInsertIndex*header.attr_length));
    /*
    char * tempchar = (char *)malloc(header.attr_length);
    memcpy(tempchar, keys + prevInsertIndex * header.attr_length, header.attr_length);
    printf("inserting after value: %s \n", tempchar);
    free(tempchar);
    */
    // if it's not a duplicate, insert
    //int thisSlot = entries[prevInsertIndex].nextSlot;
    if(!isDup){
      //printf("not duplicate\n");
      int index = nHeader->freeSlotIndex;
      //printf("index: %d, next index: %d \n", index, entries[index].nextSlot);
      memcpy(keys + header.attr_length * index, (char *)pData, header.attr_length);
      entries[index].isValid = OCCUPIED_NEW;
      if((rc = rid.GetPageNum(entries[index].page)) || (rc = rid.GetSlotNum(entries[index].slot)))
        return (rc);
      nHeader->isEmpty = false;
      nHeader->num_keys++;
      nHeader->freeSlotIndex = entries[index].nextSlot;
      if(prevInsertIndex == BEGINNING_OF_SLOTS){
        entries[index].nextSlot = nHeader->firstSlotIndex;
        nHeader->firstSlotIndex = index;
      }
      else{
        entries[index].nextSlot = entries[prevInsertIndex].nextSlot;
        entries[prevInsertIndex].nextSlot = index;
      }
    }

    // if it is a duplicate, add a new page if new, or add it to existing bucket:
    else {
      PageNum bucketPage;
      if (isDup && entries[prevInsertIndex].isValid == OCCUPIED_NEW){
        if((rc = CreateNewBucket(bucketPage)))
          return (rc);
        entries[prevInsertIndex].isValid = OCCUPIED_DUP;
        RID rid2(entries[prevInsertIndex].page, entries[prevInsertIndex].slot);
        if((rc = InsertIntoBucket(bucketPage, rid2)) || (rc = InsertIntoBucket(bucketPage, rid)))
          return (rc);
        entries[prevInsertIndex].page = bucketPage;

      }
      else{
        bucketPage = entries[prevInsertIndex].page;
        if((rc = InsertIntoBucket(bucketPage, rid)))
          return (rc);
      }
      
    }

  }
  else{
    //PrintRootPage();
    struct IX_NodeHeader_I *nIHeader = (struct IX_NodeHeader_I *)nHeader;
    PageNum nextNodePage;
    int prevInsertIndex = BEGINNING_OF_SLOTS;
    bool isDup;
    if((rc = FindNodeInsertIndex(nHeader, pData, prevInsertIndex, isDup)))
      return (rc);
    //printf("HIHIHIindex found: %d \n", prevInsertIndex);
    if(prevInsertIndex == BEGINNING_OF_SLOTS)
      nextNodePage = nIHeader->firstPage;
    else{
      nextNodePage = entries[prevInsertIndex].page;
      //int thisIndex = entries[prevInsertIndex].nextSlot;
      //nextNodePage = entries[thisIndex].page;
    }
    // read
    PF_PageHandle nextNodePH;
    struct IX_NodeHeader *nextNodeHeader;
    int newKeyIndex;
    PageNum newPageNum;
    if((rc = pfh.GetThisPage(nextNodePage, nextNodePH)) || (rc = nextNodePH.GetData((char *&)nextNodeHeader)))
      return (rc);
    if(nextNodeHeader->num_keys == header.maxKeys_N){
      splittwice = true;
      if((rc = SplitNode(nHeader, nextNodeHeader, nextNodePage, prevInsertIndex, newKeyIndex, newPageNum)))
        return (rc);
      char *value = keys + newKeyIndex*header.attr_length;
      //printf("SPLIT AT NODE: %d \n", *(int *)value);
      int compared = comparator(pData, (void *)value, header.attr_length);
      if(compared >= 0){
        PageNum nextPage = newPageNum;
        if((rc = pfh.MarkDirty(nextNodePage)) || (rc = pfh.UnpinPage(nextNodePage)))
          return (rc);
        if((rc = pfh.GetThisPage(nextPage, nextNodePH)) || (rc = nextNodePH.GetData((char *&) nextNodeHeader)))
          return (rc);
        nextNodePage = nextPage;
      }
    }
    //printf("NEW PAGE NUMBER FPR SPLIT: %d \n", newPageNum);
    // determine which one is the next node to be inserted into
    
    /*
    if(compared >= 0){
      PageNum nextPage = nextNodeHeader->invalid1;
      if((rc = pfh.MarkDirty(nextNodePage)) || (rc = pfh.UnpinPage(nextNodePage)))
        return (rc);
      if((rc = pfh.GetThisPage(nextPage, nextNodePH)) || (rc = nextNodePH.GetData((char *&) nextNodeHeader)))
        return (rc);
      nextNodePage = nextPage;
    }
    */

    if((rc = InsertIntoNonFullNode(nextNodeHeader, nextNodePage, pData, rid)))
      return (rc);
    if((rc = pfh.MarkDirty(nextNodePage)) || (rc = pfh.UnpinPage(nextNodePage)))
      return (rc);

  }
  return (rc);
}

RC IX_IndexHandle::FindNodeInsertIndex(struct IX_NodeHeader *nHeader, 
  void *pData, int& index, bool& isDup){
  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  char *keys = ((char *)nHeader + header.keysOffset_N);
  //printf("STARTING FINDNODE INSERT INDEX: \n");
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = nHeader->firstSlotIndex;
  //iterloc = -1;
  //printf("current idx: %d, %d \n", nHeader->firstSlotIndex, *(int *) (keys + header.attr_length*nHeader->firstSlotIndex));
  isDup = false;
  while(curr_idx != NO_MORE_SLOTS){
    char *value = keys + header.attr_length * curr_idx;
    int compared = comparator(pData, (void*) value, header.attr_length);
    //printf("N comparing %d, %d. result: %d \n", *(int *)pData, *(int *)value, compared);
    if(compared == 0)
      isDup = true;
    if(compared < 0)
      break;
    prev_idx = curr_idx;
    curr_idx = entries[prev_idx].nextSlot;

  } 
  index = prev_idx;
  //printf("index found: %d \n", index);
  //if(isDup)
  //  printf("is a duplicate\n");
  //printf("node found index: %d \n", index);
  return (0);
}


RC IX_IndexHandle::DeleteEntry(void *pData, const RID &rid){
  RC rc = 0;
  if(! isValidIndexHeader() || isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);

  // get root page
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader))){
    printf("failing here\n");
    return (rc);
  }

  if(rHeader->isEmpty)
    return (IX_INVALIDENTRY);

  bool toDelete = false;
  if((rc = DeleteFromNode(rHeader, pData, rid, toDelete)))
    return (rc);

  if(toDelete){
    rHeader->isLeafNode = true;
  }

  return (rc);
}


RC IX_IndexHandle::DeleteFromNode(struct IX_NodeHeader *nHeader, void *pData, const RID &rid, bool &toDelete){
  RC rc = 0;
  toDelete = false;
  if(nHeader->isLeafNode){
    if((rc = DeleteFromLeaf((struct IX_NodeHeader_L *)nHeader, pData, rid, toDelete))){
      //printf("reached here\n");
      return (rc);
    }
    if(toDelete){
      printf("toDelete is set to true\n");
    }
  }
  // else, delete from a following index
  else{
    int prevIndex, currIndex;
    bool isDup;
    if((rc = FindNodeInsertIndex(nHeader, pData, currIndex, isDup)))
      return (rc);

    struct IX_NodeHeader_I *iHeader = (struct IX_NodeHeader_I *)nHeader;
    struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
    char *keys = (char *)nHeader + header.entryOffset_N;
    
    PageNum nextNodePage;
    bool useFirstPage = false;
    if(currIndex == BEGINNING_OF_SLOTS){ // use first slot
      useFirstPage = true;
      nextNodePage = iHeader->firstPage;
      prevIndex = currIndex;
    }
    /*
    else if(isDup == true){ // key exists, go down the next path
      
      printf("****** IS IN THE INTERNAL NODE ****\n");
      currIndex = entries[prevIndex].nextSlot;
      nextNodePage = entries[currIndex].page;
    }*/
    else{ // if key does not exist, go down the prev node
      //char *prevKey = keys + prevIndex * header.attr_length;
      //if((rc = FindNodeInsertIndex(nHeader, (void *)prevKey, prevIndex, isDup)))
      //  return (rc);
      //if(prevIndex == BEGINNING_OF_SLOTS)
      //  currIndex = nHeader->firstSlotIndex;
      //else
      //currIndex = entries[prevIndex].nextSlot;
      //nextNodePage = entries[currIndex].page;
      if((rc = FindPrevIndex(nHeader, currIndex, prevIndex)))
        return (rc);
      nextNodePage = entries[currIndex].page;
    }

    PF_PageHandle nextNodePH;
    struct IX_NodeHeader *nextHeader;
    if((rc = pfh.GetThisPage(nextNodePage, nextNodePH)) || (rc = nextNodePH.GetData((char *&)nextHeader)))
      return (rc);

    bool toDeleteNext = false;
    rc = DeleteFromNode(nextHeader, pData, rid, toDeleteNext);

    RC rc2 = 0;
    if((rc2 = pfh.MarkDirty(nextNodePage)) || (rc2 = pfh.UnpinPage(nextNodePage)))
      return (rc2);

    if(rc == IX_INVALIDENTRY)
      return (rc);

    // delete this leaf/child node
    if(toDeleteNext){
      printf("todelete next is true\n");
      if((rc = pfh.DisposePage(nextNodePage))){
        printf("cannot not dispose page\n");
        return (rc);
      }
      if(useFirstPage == false){
        printf("used first page\n");
        if(prevIndex == BEGINNING_OF_SLOTS)
          nHeader->firstSlotIndex = entries[currIndex].nextSlot;
        else
          entries[prevIndex].nextSlot = entries[currIndex].nextSlot;
        entries[currIndex].nextSlot = nHeader->freeSlotIndex;
        nHeader->freeSlotIndex = currIndex;
      }
      else{
        printf("not using first page\n");
        int firstslot = nHeader->firstSlotIndex;
        nHeader->firstSlotIndex = entries[firstslot].nextSlot;
        iHeader->firstPage = entries[firstslot].page;
        entries[firstslot].nextSlot = nHeader->freeSlotIndex;
        nHeader->freeSlotIndex = firstslot;
      }

      // update counters
      if(nHeader->num_keys == 0){
        nHeader->isEmpty = true;
        toDelete = true;
      }
      else
        nHeader->num_keys--;
      
    }

  }

  return (rc);
}

RC IX_IndexHandle::FindPrevIndex(struct IX_NodeHeader *nHeader, int thisIndex, int &prevIndex){
  RC rc = 0;
  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  char *keys = ((char *)nHeader + header.keysOffset_N);
  //printf("STARTING FINDNODE INSERT INDEX: \n");
  int prev_idx = BEGINNING_OF_SLOTS;
  int curr_idx = nHeader->firstSlotIndex;
  //iterloc = -1;
  //printf("current idx: %d, %d \n", nHeader->firstSlotIndex, *(int *) (keys + header.attr_length*nHeader->firstSlotIndex));
  while(curr_idx != thisIndex){
    prev_idx = curr_idx;
    curr_idx = entries[prev_idx].nextSlot;
  } 
  prevIndex = prev_idx;
  return (0);
}


RC IX_IndexHandle::DeleteFromLeaf(struct IX_NodeHeader_L *nHeader, void *pData, const RID &rid, bool &toDelete){
  RC rc = 0;
  int prevIndex, currIndex;
  bool isDup;
  if((rc = FindNodeInsertIndex((struct IX_NodeHeader *)nHeader, pData, currIndex, isDup)))
    return (rc);
  if(isDup == false) // does not exist
    return (IX_INVALIDENTRY);

  struct Node_Entry *entries = (struct Node_Entry *)((char *)nHeader + header.entryOffset_N);
  char *key = (char *)nHeader + header.keysOffset_N;

  if(currIndex== nHeader->firstSlotIndex)
    prevIndex = currIndex;
  else{
    if((rc = FindPrevIndex((struct IX_NodeHeader *)nHeader, currIndex, prevIndex)))
      return (rc);
  }
  /*
  if(prevIndex == BEGINNING_OF_SLOTS){
    printf("correct?\n");
    currIndex = nHeader->firstSlotIndex;
  }
  else
    currIndex = entries[prevIndex].nextSlot;
    */

  // if only entry, delete it
  if(entries[currIndex].isValid == OCCUPIED_NEW){
    PageNum ridPage;
    SlotNum ridSlot;
    if((rc = rid.GetPageNum(ridPage)) || (rc = rid.GetSlotNum(ridSlot))){
      printf("error in retrieving rid\n");
      return (rc);
    }

    //printf("searching at index: %d\n", currIndex);
    //printf("searching for: %d %d \n", ridPage, ridSlot);
    //printf("got: %d %d \n", entries[currIndex].page, entries[currIndex].slot);
    int compare = comparator((void*)(key + header.attr_length*currIndex), pData, header.attr_length);
    if(ridPage != entries[currIndex].page || ridSlot != entries[currIndex].slot || compare != 0 )
      return (IX_INVALIDENTRY);

    if(currIndex == nHeader->firstSlotIndex){
      //printf("delete first node entry\n");
      nHeader->firstSlotIndex = entries[currIndex].nextSlot;
    }
    else
      entries[prevIndex].nextSlot = entries[currIndex].nextSlot;
      
    entries[currIndex].nextSlot = nHeader->freeSlotIndex;
    nHeader->freeSlotIndex = currIndex;
    entries[currIndex].isValid = UNOCCUPIED;
    nHeader->num_keys--;
    //printf("number of keys remaining: %d \n", nHeader->num_keys);
  }
  // if duplicate, delete it from bucket
  else if(entries[currIndex].isValid == OCCUPIED_DUP){
    //printf("deleting from bucket\n");
    PageNum bucketNum = entries[currIndex].page;
    PF_PageHandle bucketPH;
    struct IX_BucketHeader *bHeader;
    bool deletePage = false;
    RID lastRID;
    PageNum nextBucketNum;
    if((rc = pfh.GetThisPage(bucketNum, bucketPH)) || (rc = bucketPH.GetData((char *&)bHeader))){
      //printf("not valid page\n");
      return (rc);
    }
    /*
    if((rc = DeleteFromBucket(bHeader, rid, deletePage, lastRID, nextBucketNum))){
      RC rc2 = 0;
      if((rc2 = pfh.MarkDirty(bucketNum)) || (pfh.UnpinPage(bucketNum)))
        return (rc2);
      return (rc);
    }*/
    rc = DeleteFromBucket(bHeader, rid, deletePage, lastRID, nextBucketNum);
    RC rc2 = 0;
    if((rc2 = pfh.MarkDirty(bucketNum)) || (rc = pfh.UnpinPage(bucketNum)))
      return (rc2);

    if(rc == IX_INVALIDENTRY)
      return (IX_INVALIDENTRY);

    if(deletePage){ // delete the bucket
      printf("deletingpage\n");
      if((rc = pfh.DisposePage(bucketNum) ))
        return (rc);
      if(nextBucketNum == NO_MORE_PAGES){
        entries[currIndex].isValid = OCCUPIED_NEW;
        if((rc = lastRID.GetPageNum(entries[currIndex].page)) || 
          (rc = lastRID.GetSlotNum(entries[currIndex].slot)))
          return (rc);
        printf("---------nextrid: %d %d \n", entries[currIndex].page, entries[currIndex].slot);
      }
      else
        entries[currIndex].page = nextBucketNum;
    }
  }
  if(nHeader->num_keys == 0){
    printf("hellw??\n");
    toDelete = true;
    // reset the pointers
    PageNum prevPage = nHeader->prevPage;
    PageNum nextPage = nHeader->nextPage;
    PF_PageHandle leafPH;
    struct IX_NodeHeader_L *leafHeader;
    if(prevPage != NO_MORE_PAGES){
      if((rc = pfh.GetThisPage(prevPage, leafPH))|| (rc = leafPH.GetData((char *&)leafHeader)) )
        return (rc);
      leafHeader->nextPage = nextPage;
      if((rc = pfh.MarkDirty(prevPage)) || (rc = pfh.UnpinPage(prevPage)))
        return (rc);
    }
    if(nextPage != NO_MORE_PAGES){
      if((rc = pfh.GetThisPage(nextPage, leafPH))|| (rc = leafPH.GetData((char *&)leafHeader)) )
        return (rc);
      leafHeader->prevPage = prevPage;
      if((rc = pfh.MarkDirty(nextPage)) || (rc = pfh.UnpinPage(nextPage)))
        return (rc);
    }
    printf("finished deleting leaf node\n");

  }
  //printf("reaches the end of delete from leaf\n");
  return (0);
}


RC IX_IndexHandle::DeleteFromBucket(struct IX_BucketHeader *bHeader, const RID &rid, 
  bool &deletePage, RID &lastRID, PageNum &nextPage){
  printf("beginning of delete from bucket\n");
  RC rc = 0;
  PageNum nextPageNum = bHeader->nextBucket;
  nextPage = bHeader->nextBucket;

  struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);

  if((nextPageNum != NO_MORE_PAGES)){
    printf("getting next page\n");
    bool toDelete = false;
    PF_PageHandle nextBucketPH;
    struct IX_BucketHeader *nextHeader;
    RID last;
    PageNum nextNextPage;
    if((rc = pfh.GetThisPage(nextPageNum, nextBucketPH)) || (rc = nextBucketPH.GetData((char *&)nextHeader)))
      return (rc);
    rc = DeleteFromBucket(nextHeader, rid, toDelete, last, nextNextPage);
    int numKeysInNext = nextHeader->num_keys;
    RC rc2 = 0;
    if((rc2 = pfh.MarkDirty(nextPageNum)) || (rc2 = pfh.UnpinPage(nextPageNum)))
      return (rc2);

    if(toDelete && bHeader->num_keys < header.maxKeys_B && numKeysInNext == 1){
      int loc = bHeader->freeSlotIndex;
      if((rc2 = last.GetPageNum(entries[loc].page)) || (rc2 = last.GetSlotNum(entries[loc].slot)))
        return (rc2);

      bHeader->freeSlotIndex = entries[loc].nextSlot;
      entries[loc].nextSlot = bHeader->firstSlotIndex;
      bHeader->firstSlotIndex = loc;

      bHeader->num_keys++;
      numKeysInNext == 0;
    }
    if(toDelete && numKeysInNext == 0){
      if((rc2 = pfh.DisposePage(nextPageNum)))
        return (rc2);
      bHeader->nextBucket = nextNextPage;
    }

    if(rc == 0)
      return (0);
  }

  PageNum ridPage;
  SlotNum ridSlot;
  if((rc = rid.GetPageNum(ridPage))|| (rc = rid.GetSlotNum(ridSlot)))
    return (rc);
  
  int prevIndex = BEGINNING_OF_SLOTS;
  int currIndex = bHeader->firstSlotIndex;
  bool found = false;
  while(currIndex != NO_MORE_SLOTS){
    if(entries[currIndex].page == ridPage && entries[currIndex].slot == ridSlot){
      found = true;
      break;
    }
    prevIndex = currIndex;
    currIndex = entries[prevIndex].nextSlot;
  }

  // if found, delete
  if(found){
    printf("found please delete thnx\n");
    if (prevIndex == BEGINNING_OF_SLOTS)
      bHeader->firstSlotIndex = entries[currIndex].nextSlot;
    else
      entries[prevIndex].nextSlot = entries[currIndex].nextSlot;
    entries[currIndex].nextSlot = bHeader->freeSlotIndex;
    bHeader->freeSlotIndex = currIndex;

    printf("num left in bucket : %d \n", bHeader->num_keys);
    bHeader->num_keys--;
    if(bHeader->num_keys == 1){ // move this last one to the prev 
      int firstSlot = bHeader->firstSlotIndex;
      RID last(entries[firstSlot].page, entries[firstSlot].slot);
      lastRID = last;
      printf("set delete to true \n");
      deletePage = true;
    }

    return (0);

  }

  printf("not found\n");
  return (IX_INVALIDENTRY);

}

/// RETURN NOT FOUND IF NOT FOUND
/*
RC IX_IndexHandle::DeleteFromBucket(struct IX_BucketHeader *bHeader, const RID &rid, 
  bool &deletePage, RID &lastRID, PageNum &nextPage){
  RC rc = 0;
  nextPage = bHeader->nextBucket;

  struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);

  PageNum ridPage;
  SlotNum ridSlot;
  if((rc = rid.GetPageNum(ridPage))|| (rc = rid.GetSlotNum(ridSlot)))
    return (rc);
  
  int prevIndex = BEGINNING_OF_SLOTS;
  int currIndex = bHeader->firstSlotIndex;
  bool found = false;
  while(currIndex != NO_MORE_SLOTS){
    if(entries[currIndex].page == ridPage && entries[currIndex].slot == ridSlot){
      found = true;
      break;
    }
    prevIndex = currIndex;
    currIndex = entries[prevIndex].nextSlot;
  }

  if(found){
    if (prevIndex == BEGINNING_OF_SLOTS)
      bHeader->firstSlotIndex = entries[currIndex].nextSlot;
    else
      entries[prevIndex].nextSlot = entries[currIndex].nextSlot;
    entries[currIndex].nextSlot = bHeader->firstSlotIndex;
    bHeader->firstSlotIndex = currIndex;

    bHeader->num_keys--;
    if(bHeader->num_keys == 1){ // move this last one to the prev 
      int firstSlot = bHeader->firstSlotIndex;
      RID last(entries[firstSlot].page, entries[firstSlot].slot);
      lastRID = last;
      deletePage = true;
    }

  }
  else if(nextPage == NO_MORE_PAGES){
    return (IX_INVALIDENTRY);
  }
  else{ // search in next bucket
    bool toDelete = false;
    PF_PageHandle nextBucketPH;
    struct IX_BucketHeader *nextHeader;
    if((rc = pfh.GetThisPage(nextPage, nextBucketPH)) || (rc = nextBucketPH.GetData((char *&)nextHeader)))
      return (rc);
    PageNum nextBucketPage;
    rc = DeleteFromBucket(nextHeader, rid, toDelete, lastRID, nextBucketPage);
    RC rc2 = 0;
    if((rc2 = pfh.MarkDirty(nextPage)) || (rc2 = pfh.UnpinPage(nextPage)))
      return (rc2);
    if(rc == IX_INVALIDENTRY)
      return (IX_INVALIDENTRY);

    if(toDelete){
      if(bHeader->num_keys < header.maxKeys_B){
        // add it to the current index
      }
      else{
        bHeader->nextBucket = nextBucketPage;
      }

    }
  }


  return (0);
}
*/


// Returns the page handle with the header set up
RC IX_IndexHandle::CreateNewNode(PF_PageHandle &ph, PageNum &page, char *&nData, bool isLeaf){
  RC rc = 0;
  if((rc = pfh.AllocatePage(ph)) || (rc = ph.GetPageNum(page))){
    return (rc);
  }
  if((rc = ph.GetData(nData)))
    return (rc);
  struct IX_NodeHeader *nHeader = (struct IX_NodeHeader *)nData;

  nHeader->isLeafNode = isLeaf;
  nHeader->isEmpty = true;
  nHeader->num_keys = 0;
  nHeader->invalid1 = NO_MORE_PAGES;
  nHeader->invalid2 = NO_MORE_PAGES;
  nHeader->firstSlotIndex = NO_MORE_SLOTS;
  nHeader->freeSlotIndex = 0;

  struct Node_Entry *entries = (struct Node_Entry *)((char*)nHeader + header.entryOffset_N);

  for(int i=0; i < header.maxKeys_N; i++){
    entries[i].isValid = UNOCCUPIED;
    entries[i].page = NO_MORE_PAGES;
    if(i == (header.maxKeys_N -1))
      entries[i].nextSlot = NO_MORE_SLOTS;
    else
      entries[i].nextSlot = i+1;
  }
  //printf("NODE CREATION: %d \n", page);

  return (rc);
}

RC IX_IndexHandle::CreateNewBucket(PageNum &page){
  char *nData;
  PF_PageHandle ph;
  RC rc = 0;
  if((rc = pfh.AllocatePage(ph)) || (rc = ph.GetPageNum(page))){
    return (rc);
  }
  if((rc = ph.GetData(nData))){
    RC rc2;
    if((rc2 = pfh.UnpinPage(page)))
      return (rc2);
    return (rc);
  }
  struct IX_BucketHeader *bHeader = (struct IX_BucketHeader *) nData;

  bHeader->num_keys = 0;
  bHeader->firstSlotIndex = NO_MORE_SLOTS;
  bHeader->freeSlotIndex = 0;
  bHeader->nextBucket = NO_MORE_PAGES;

  struct Bucket_Entry *entries = (struct Bucket_Entry *)((char *)bHeader + header.entryOffset_B);

  for(int i=0; i < header.maxKeys_B; i++){
    if(i == (header.maxKeys_B -1))
      entries[i].nextSlot = NO_MORE_SLOTS;
    else
      entries[i].nextSlot = i+1;
  }
  //printf("NEW BUCKET PAGE %d \n", page);
  if( (rc =pfh.MarkDirty(page)) || (rc = pfh.UnpinPage(page)))
    return (rc);

  return (rc);
}

RC IX_IndexHandle::ForcePages(){
  RC rc = 0;
  if (isOpenHandle == false)
    return (IX_INVALIDINDEXHANDLE);
  pfh.ForcePages();
  return (rc);
}

// returns pinned first leaf page
RC IX_IndexHandle::GetFirstLeafPage(PF_PageHandle &leafPH, PageNum &leafPage){
  RC rc = 0;
  struct IX_NodeHeader *rHeader;
  if((rc = rootPH.GetData((char *&)rHeader))){
    //printf("failing here\n");
    return (rc);
  }

  // if root node is a leaf:
  if(rHeader->isLeafNode == true){
    leafPH = rootPH;
    leafPage = header.rootPage;
    return (0);
  }

  struct IX_NodeHeader_I *nHeader = (struct IX_NodeHeader_I *)rHeader;
  PageNum nextPageNum = nHeader->firstPage;
  PF_PageHandle nextPH;
  if(nextPageNum == NO_MORE_PAGES)
    return (IX_EOF);
  if((rc = pfh.GetThisPage(nextPageNum, nextPH)) || (rc = nextPH.GetData((char *&)nHeader)))
    return (rc);
  while(nHeader->isLeafNode == false){
    PageNum prevPage = nextPageNum;
    nextPageNum = nHeader->firstPage;
    if((rc = pfh.UnpinPage(prevPage)))
      return (rc);
    if((rc = pfh.GetThisPage(nextPageNum, nextPH)) || (rc = nextPH.GetData((char *&)nHeader)))
      return (rc);
  }
  leafPage = nextPageNum;
  leafPH = nextPH;

  return (rc);
}



bool IX_IndexHandle::isValidIndexHeader() const{
  //printf("maxkeys: %d, %d \n", header.maxKeys_I, header.maxKeys_L);
  if(header.maxKeys_N <= 0 || header.maxKeys_B <= 0){
    printf("fail 1\n");
    return false;
  }
  if(header.entryOffset_N != sizeof(struct IX_NodeHeader) || 
    header.entryOffset_B != sizeof(struct IX_BucketHeader)){
    //printf("keyheaders: %d , %d \n", header.keyHeadersOffset_I, header.keyHeadersOffset_L);
    //printf("part 2 not true \n");
    printf("fail 2\n");
    return false;
  }

  //printf("size of keyHeader_L : %d \n", sizeof(struct KeyHeader_L));
  //int attrLength1 = (header.keysOffset_B - header.entryOffset_B)/(header.maxKeys_B);
  int attrLength2 = (header.keysOffset_N - header.entryOffset_N)/(header.maxKeys_N);
  //printf("attribute length: %d, %d \n", attrLength1, attrLength2);
  //if(attrLength1 != sizeof(struct Bucket_Entry) || attrLength2 != sizeof(struct Node_Entry))
  if(attrLength2 != sizeof(struct Node_Entry)){
    printf("fail 3\n");
    return false;
  }
  if((header.entryOffset_B + header.maxKeys_B * sizeof(Bucket_Entry) > PF_PAGE_SIZE) ||
    header.keysOffset_N + header.maxKeys_N * header.attr_length > PF_PAGE_SIZE)
    return false;
  return true;
}

int IX_IndexHandle::CalcNumKeysNode(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_NodeHeader);
  return floor(1.0*body_size / (sizeof(struct Node_Entry) + attrLength));
}

int IX_IndexHandle::CalcNumKeysBucket(int attrLength){
  int body_size = PF_PAGE_SIZE - sizeof(struct IX_BucketHeader);
  return floor(1.0*body_size / (sizeof(Bucket_Entry)));
}
