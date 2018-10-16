//
//  de_internal.h
//  DownloadEngine
//
//  Created by liu enbao on 12/2/16.
//  Copyright © 2016 xb. All rights reserved.
//

#ifndef de_internal_h
#define de_internal_h

#include "common_def.h"

#include <NBMap.h>
#include <NBString.h>

#include <pthread.h>

class DEBaseTask;

//typedef std::vector<DEAny> ParamsType;

typedef NBMap<int, DEBaseTask*> TaskMapType;
typedef TaskMapType::const_iterator TaskMapConstIterator;

//typedef std::list<int> TaskListType;
//typedef TaskListType::const_iterator TaskListConstIterator;

//typedef std::vector<DEBaseTask*> TaskVectorType;
//typedef TaskVectorType::const_iterator TaskVectorConstIterator;
//typedef TaskVectorType::iterator TaskVectorIterator;

#define DEFAULT_BUFFER_SIZE 0x8000  //32K

#define MAX_FILESYTEM_PROCESSING 128
#define MAX_FILESYTEM_PROCESSING_COUNT 256
#define MAX_FILESYTEM_PROCESSING_MASK 255

// #define MAX_PARALLEL_TASK_SIZE 2097152 // 2M for per task
// #define MAX_PARALLEL_TASK_NUM 2

// #define MAX_SEGMENT_NUM MAX_PARALLEL_TASK_NUM * 2

#define DEFAULT_SEGMENT_BUFFER_SIZE 0x1000000 //16M, should >= MAX_PARALLEL_TASK_SIZE*MAX_SEGMENT_NUM

#define MAX_PARALLEL_TASK_SIZE 0x40000 // 256K for per task
#define MAX_PARALLEL_TASK_NUM 50

#define MAX_SEGMENT_NUM 64

#define MAX_CACHE_SEGMENT_NUM 16

#define MAX_NETWORK_CACHE_SIZE 0x40000

#define MAX_ERROR_RETRY 60

#endif /* de_internal_h */
