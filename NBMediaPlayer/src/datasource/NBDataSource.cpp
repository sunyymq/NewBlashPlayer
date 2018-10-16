//
// Created by parallels on 9/7/18.
//

#include "NBDataSource.h"
#include "NBFFmpegSource.h"
#include "NBBeastSource.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavutil/avstring.h>
#include <libavutil/common.h>
#ifdef __cplusplus
}
#endif

NBDataSource::NBDataSource()
    :mUserInterrupted(false) {

}

NBDataSource::~NBDataSource() {

}

void NBDataSource::RegisterSource() {
    NBFFmpegSource::RegisterSource();
}

void NBDataSource::UnRegisterSource() {
    NBFFmpegSource::UnRegisterSource();
}

NBDataSource* NBDataSource::CreateFromURI(const NBString& uri) {
    NBDataSource* source = NULL;
    if (uri.size() < 7) {
        return NULL;
    }
    
    const char* filename = uri.string();

    if (!strncmp(filename, "http://", 7)) {
        source = new NBBeastSource(uri);
    } else {
        source = new NBFFmpegSource(uri);
    }

    return source;
}

void NBDataSource::Destroy(NBDataSource* dataSource) {
    if (dataSource == NULL) {
        return ;
    }
    delete dataSource;
}
