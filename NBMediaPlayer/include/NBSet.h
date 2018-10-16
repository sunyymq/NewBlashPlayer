//
//  NBSet.h
//  iOSNBMediaPlayer
//
//  Created by liu enbao on 28/09/2018.
//  Copyright © 2018 liu enbao. All rights reserved.
//

#ifndef NSSET_H
#define NSSET_H

#include <NBRBTree.h>

template <typename KeyType>
class NBSet {
public:
    struct NBMapEntry {            //Entry node
        KeyType first;              //Entry key
        struct rb_node node;
    };
    
    class const_iterator {            //Hash_map iterator
    public:
        const_iterator()
        : mBucket(NULL) {
            
        }
        
        const_iterator(NBMapEntry* bucket)
        : mBucket(bucket) {
        }
        
        const_iterator& operator++() {
            struct rb_node *node = rb_next(&mBucket->node);
            if (node != NULL) {
                mBucket = rb_entry(node, NBMapEntry, node);
            } else {
                mBucket = NULL;
            }
            return *this;
        }
        
        const_iterator operator++(int) {
            const_iterator tmp = const_iterator(mBucket);
            struct rb_node *node = rb_next(&mBucket->node);
            if (node != NULL) {
                mBucket = rb_entry(node, NBMapEntry, node);
            } else {
                mBucket = NULL;
            }
            return tmp;
        }
        
        const_iterator& operator--() {
            struct rb_node *node = rb_prev(&mBucket->node);
            if (node != NULL) {
                mBucket = rb_entry(node, NBMapEntry, node);
            } else {
                mBucket = NULL;
            }
            return *this;
        }
        
        const_iterator operator--(int) {
            const_iterator tmp = const_iterator(mBucket);
            struct rb_node *node = rb_prev(&mBucket->node);
            if (node != NULL) {
                mBucket = rb_entry(node, NBMapEntry, node);
            } else {
                mBucket = NULL;
            }
            return tmp;
        }
        
        bool operator==(const const_iterator& p) const {
            if (mBucket == p.mBucket) {
                return true;
            }
            if (mBucket == NULL || p.mBucket == NULL) {
                return false;
            }
            return mBucket->first == p->first;
        }
        
        bool operator!=(const const_iterator& p) const {
            if (mBucket == p.mBucket) {
                return false;
            }
            if (mBucket == NULL || p.mBucket == NULL) {
                return true;
            }
            return mBucket->first != p->first;
        }
        
        NBMapEntry* operator->() const {
            return mBucket;
        }
        
        KeyType operator*() {
            return mBucket->first;
        }
        
    private:
        friend class NBSet;
        
    private:
        NBMapEntry* mBucket;
    };
    
public:
    NBSet();
    ~NBSet();
    
    void clear();
    size_t erase(KeyType key);
    size_t erase(const_iterator iter);
    bool insert(const KeyType& key);
    
    size_t size() {
        //        int realSize = 0;
        //        for (struct rb_node *node = rb_first(&mRBTree); node; node = rb_next(node))
        //            ++realSize;
        //        printf("The real size is : %d : %u\n", realSize, mSize);
        return mSize;
    }
    
    const_iterator begin() const;
    const_iterator end() const;
    
    const_iterator find(const KeyType& key) const;
    
private:
    struct rb_root mRBTree;
    size_t mSize;
    
private:
    NBMapEntry* searchRBTree(struct rb_root *root, KeyType key, struct rb_node **parent = NULL, struct rb_node ***p_new_node = NULL) const;
};

template <typename KeyType>
NBSet<KeyType>::NBSet() {
    mRBTree = RB_ROOT;
    mSize = 0;
}

template <typename KeyType>
NBSet<KeyType>::~NBSet() {
    clear();
}

template <typename KeyType>
typename NBSet<KeyType>::NBMapEntry* NBSet<KeyType>::searchRBTree(struct rb_root *root, KeyType key, struct rb_node **parent, struct rb_node ***p_new_node) const
{
    struct rb_node **new_node = &(root->rb_node);
    if (p_new_node != NULL)
        *p_new_node = new_node;
    
    while (*new_node) {
        NBMapEntry* data = container_of(*new_node, NBMapEntry, node);
        
        if (parent != NULL)
            *parent = *new_node;
        
        if (key < data->first)
            new_node = &((*new_node)->rb_left);
        else if (key > data->first)
            new_node = &((*new_node)->rb_right);
        else
            return data;
        
        if (p_new_node != NULL)
            *p_new_node = new_node;
    }
    
    return NULL;
}

template <typename KeyType>
void NBSet<KeyType>::clear() {
    while (struct rb_node* node = rb_first(&mRBTree)) {
        NBMapEntry* mapEntry = container_of(node, NBMapEntry, node);
        rb_erase(&mapEntry->node, &mRBTree);
        RB_EMPTY_NODE(&mapEntry->node);
        delete mapEntry;
    }
    
    mSize = 0;
}

template <typename KeyType>
size_t NBSet<KeyType>::erase(KeyType key) {
    NBMapEntry* mapEntry = searchRBTree(&mRBTree, key);
    if (mapEntry == NULL) {
        return 0;
    }
    rb_erase(&mapEntry->node, &mRBTree);
    delete mapEntry;
    -- mSize;
    return 1;
}

template <typename KeyType>
size_t NBSet<KeyType>::erase(const_iterator iter) {
    if (iter == end()) {
        return 0;
    }
    NBMapEntry* mapEntry = iter.mBucket;
    rb_erase(&mapEntry->node, &mRBTree);
    delete mapEntry;
    -- mSize;
    return 1;
}

template <typename KeyType>
typename NBSet<KeyType>::const_iterator NBSet<KeyType>::begin() const {
    struct rb_node *node = rb_first(&mRBTree);
    return const_iterator(rb_entry(node, NBMapEntry, node));
}

template <typename KeyType>
typename NBSet<KeyType>::const_iterator NBSet<KeyType>::end() const {
    return const_iterator(NULL);
}

template <typename KeyType>
typename NBSet<KeyType>::const_iterator NBSet<KeyType>::find(const KeyType& key) const {
    NBMapEntry* mapEntry = searchRBTree((struct rb_root*)&mRBTree, key);
    if (mapEntry == NULL) {
        return const_iterator(NULL);
    }
    return const_iterator(mapEntry);
}

template <typename KeyType>
bool NBSet<KeyType>::insert(const KeyType& key) {
    struct rb_node* parent = NULL;
    struct rb_node** new_node = NULL;
    NBMapEntry* mapEntry = searchRBTree(&mRBTree, key, &parent, &new_node);
    if (mapEntry != NULL) {
        return false;
    }
    mapEntry = new NBMapEntry;
    mapEntry->first = key;
    /* Add new node and rebalance tree. */
    rb_link_node(&mapEntry->node, parent, new_node);
    rb_insert_color(&mapEntry->node, &mRBTree);
    ++mSize;
    return true;
}

#endif /* NSSET_H */
