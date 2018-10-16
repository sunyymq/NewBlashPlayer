#ifndef DE_LRU_CACHE_H
#define DE_LRU_CACHE_H

#include <NBMap.h>
#include <list>
#include <algorithm>

template <class T, const T VALUE = T(-1)>
class DELRUCache {
public:
    const static T INVALID_VALUE = VALUE;

public:
    DELRUCache(size_t size)
        :max_entries(size) {
        mEntries = new Node[max_entries];
        mFreeEntries = new Node*[max_entries];
        mHead = new Node;
        mTail = new Node;

        reset();
    }

    ~DELRUCache(){
        delete mHead;
        delete mTail;
        delete[] mEntries;
        delete[] mFreeEntries;
    }

    void reset() {
        mAccessIndex.clear();
        mFreeEntriesIndex = 0;

        // 存储可用结点的地址
        for(int i = 0; i < max_entries; ++i)
            mFreeEntries[i] = &mEntries[i];
//        for(int i = 0; i < max_entries; ++i)
//            mFreeEntries.push_back(mEntries+i);

        mHead->prev = NULL;
        mHead->next = mTail;
        mTail->prev = mHead;
        mTail->next = NULL;
    }

    T set(T data){
        T replacedValue = INVALID_VALUE;
        Node *node = NULL;

        typename NBMap<T, Node*>::const_iterator iter = mAccessIndex.find(data);

        // node exists
        if(iter != mAccessIndex.end()) {
            node = iter->second;
            detach(node);
            node->data = data;
            attach(node);
            return replacedValue;
        }

        // 可用结点为空，即cache已满
        if(mFreeEntriesIndex >= max_entries) {
            node = mTail->prev;
            replacedValue = node->data;
            detach(node);
            mAccessIndex.erase(node->data);
        }
        else{
            node = mFreeEntries[mFreeEntriesIndex];
            ++mFreeEntriesIndex;
        }

        node->data = data;
        mAccessIndex[data] = node;
        attach(node);

        return replacedValue;
    }

    T get(T key) {
        T replacedValue = INVALID_VALUE;
        Node *node = NULL;

        typename NBMap<T, Node*>::const_iterator iter = mAccessIndex.find(key);
        if(iter == mAccessIndex.end()) {
            return replacedValue;
        }

        node = iter->second;
        return node->data;
    }

private:

    struct Node{
        T data;
        Node *prev, *next;
    };

    void detach(Node* node){
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    void attach(Node* node){
        node->prev = mHead;
        node->next = mHead->next;
        mHead->next = node;
        node->next->prev = node;
    }

private:
    NBMap<T, Node*> mAccessIndex;

    //Dual link list
    Node *mHead;
    Node *mTail;

    //Cached node
    Node **mFreeEntries;
    int mFreeEntriesIndex;
    Node *mEntries;

    const int max_entries;
};

#endif /* lru_cache_h */
