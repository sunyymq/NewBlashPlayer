#include "DESocketHelper.h"
#include "common/DELooper.h"
#include "common/DEInternal.h"
#include "task/DEBaseTask.h"

/* CURLOPT_OPENSOCKETFUNCTION */
curl_socket_t opensocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address) {
    DEBaseTask* task_item = (DEBaseTask*)clientp;
    curl_socket_t sockfd = CURL_SOCKET_BAD;
    /* restrict to IPv4 */
    if(purpose == CURLSOCKTYPE_IPCXN && address->family == AF_INET) {
        de_open_socket_func open_socket = de_looper_get_open_socket();
        de_socket_t sock = open_socket(task_item->getLooper(), task_item);
        if (sock != -1) {
            sockfd = sock;
        }
    }
    return sockfd;
}

/* CURLOPT_CLOSESOCKETFUNCTION */
int close_socket(void *clientp, curl_socket_t sockfd){
    DEBaseTask* task_item = (DEBaseTask*)clientp;
    de_close_socket_func close_socket = de_looper_get_close_socket();
    close_socket(task_item->getLooper(), sockfd);
    return 0;
}

