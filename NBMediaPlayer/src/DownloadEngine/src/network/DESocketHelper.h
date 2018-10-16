#ifndef DE_SOCKET_HELPER_H
#define DE_SOCKET_HELPER_H

#include <curl/curl.h>

curl_socket_t opensocket(void *clientp, curlsocktype purpose, struct curl_sockaddr *address);
int close_socket(void *clientp, curl_socket_t sockfd);

#endif
