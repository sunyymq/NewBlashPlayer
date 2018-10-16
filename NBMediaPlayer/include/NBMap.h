//
// Created by parallels on 9/8/18.
//

#ifndef NBMAP_H
#define NBMAP_H

#include <NBRBTree.h>

template <typename KeyType, typename ValueType>
class NBMap {
public:
    struct NBMapEntry {			//Entry node
        KeyType first;              //Entry key
        ValueType second;          	//Entry value
        struct rb_node node;
    };

    class const_iterator {			//Hash_map iterator
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

        ValueType& operator*() {
            return mBucket->second;
        }

        NBMapEntry* operator->() const {
            return mBucket;
        }

    private:
        friend class NBMap;

    private:
        NBMapEntry* mBucket;
    };

public:
    NBMap();
    ~NBMap();

    void clear();
    size_t erase(KeyType key);
    size_t erase(const_iterator iter);
    
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
    ValueType& operator[] (const KeyType& key);

private:
    struct rb_root mRBTree;
    size_t mSize;

private:
    NBMapEntry* searchRBTree(struct rb_root *root, KeyType key, struct rb_node **parent = NULL, struct rb_node ***p_new_node = NULL) const;
};

template <typename KeyType, typename ValueType>
NBMap<KeyType, ValueType>::NBMap() {
    mRBTree = RB_ROOT;
    mSize = 0;
}

template <typename KeyType, typename ValueType>
NBMap<KeyType, ValueType>::~NBMap() {
    clear();
}

template <typename KeyType, typename ValueType>
typename NBMap<KeyType, ValueType>::NBMapEntry* NBMap<KeyType, ValueType>::searchRBTree(struct rb_root *root, KeyType key, struct rb_node **parent, struct rb_node ***p_new_node) const
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

template <typename KeyType, typename ValueType>
void NBMap<KeyType, ValueType>::clear() {
    while (struct rb_node* node = rb_first(&mRBTree)) {
        NBMapEntry* mapEntry = container_of(node, NBMapEntry, node);
        rb_erase(&mapEntry->node, &mRBTree);
        RB_EMPTY_NODE(&mapEntry->node);
        delete mapEntry;
    }
    
    mSize = 0;
}

template <typename KeyType, typename ValueType>
size_t NBMap<KeyType, ValueType>::erase(KeyType key) {
    NBMapEntry* mapEntry = searchRBTree(&mRBTree, key);
    if (mapEntry == NULL) {
        return 0;
    }
    rb_erase(&mapEntry->node, &mRBTree);
    delete mapEntry;
    -- mSize;
    return 1;
}

template <typename KeyType, typename ValueType>
size_t NBMap<KeyType, ValueType>::erase(const_iterator iter) {
    if (iter == end()) {
        return 0;
    }
    NBMapEntry* mapEntry = iter.mBucket;
    rb_erase(&mapEntry->node, &mRBTree);
    delete mapEntry;
    -- mSize;
    return 1;
}

template <typename KeyType, typename ValueType>
typename NBMap<KeyType, ValueType>::const_iterator NBMap<KeyType, ValueType>::begin() const {
    struct rb_node *node = rb_first(&mRBTree);
    return const_iterator(rb_entry(node, NBMapEntry, node));
}

template <typename KeyType, typename ValueType>
typename NBMap<KeyType, ValueType>::const_iterator NBMap<KeyType, ValueType>::end() const {
    return const_iterator(NULL);
}

template <typename KeyType, typename ValueType>
typename NBMap<KeyType, ValueType>::const_iterator NBMap<KeyType, ValueType>::find(const KeyType& key) const {
    NBMapEntry* mapEntry = searchRBTree((struct rb_root*)&mRBTree, key);
    if (mapEntry == NULL) {
        return const_iterator(NULL);
    }
    return const_iterator(mapEntry);
}

template <typename KeyType, typename ValueType>
ValueType& NBMap<KeyType, ValueType>::operator[] (const KeyType& key) {
    struct rb_node* parent = NULL;
    struct rb_node** new_node = NULL;
    NBMapEntry* mapEntry = searchRBTree(&mRBTree, key, &parent, &new_node);
    if (mapEntry == NULL) {
        mapEntry = new NBMapEntry;
        mapEntry->first = key;
        /* Add new node and rebalance tree. */
        rb_link_node(&mapEntry->node, parent, new_node);
        rb_insert_color(&mapEntry->node, &mRBTree);
        ++mSize;
    }
    return mapEntry->second;
}

#endif //NBMAP_H
