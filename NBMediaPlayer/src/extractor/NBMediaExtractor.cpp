//
// Created by parallels on 9/7/18.
//

#include "NBMediaExtractor.h"
#include "NBFFmpegExtractor.h"
#include "foundation/NBMetaData.h"

NBMediaExtractor* NBMediaExtractor::Create(INBMediaSeekListener* seekListener, NBDataSource* dataSource, const char *mime) {
    NBMediaExtractor* ret = NULL;
    if (true) {
        ret = new NBFFmpegExtractor(seekListener, dataSource);
    }
    return ret;
}

void NBMediaExtractor::Destroy(NBMediaExtractor* extractor) {
    if (extractor == NULL) {
        return ;
    }
    delete extractor;
}

NBMetaData* NBMediaExtractor::getMetaData() {
    return new NBMetaData;
}

void NBMediaExtractor::notifyContinueRead() {

}
