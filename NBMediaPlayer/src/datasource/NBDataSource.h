//
// Created by parallels on 9/7/18.
//

#ifndef NBDATASOURCE_H
#define NBDATASOURCE_H

#include <NBString.h>
#include <NBMap.h>

class NBDataSource {
public:
    NBDataSource();
    virtual ~NBDataSource();

public:
    static void RegisterSource();
    static void UnRegisterSource();

    static NBDataSource* CreateFromURI(const NBString& uri);
    static void Destroy(NBDataSource* dataSource);

public:
    virtual nb_status_t initCheck(const NBMap<NBString, NBString>* params) = 0;
    virtual void* getContext() {
        return NULL;
    }
    virtual NBString getUri() {
        return NBString();
    }

public:
    bool isUserInterrupted() {
        return mUserInterrupted;
    }

    void setUserInterrupted(bool userInterrupted) {
        mUserInterrupted = userInterrupted;
    }

protected:
    bool mUserInterrupted;
};

#endif //NBDATASOURCE_H
