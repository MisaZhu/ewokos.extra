#ifndef TINYHTTPSC_H
#define TINYHTTPSC_H

#include <tinyhttpsc/BearHttpsClientOne.h>

typedef BearHttpsRequest TinyHttpsRequest;
typedef BearHttpsResponse TinyHttpsResponse;

TinyHttpsRequest* NewHttpsRequest(const char* url);
void HttpsRequestFree(TinyHttpsRequest* request);
void HttpsResponseFree(TinyHttpsResponse* response);
void HttpsRequestSetTimeout(TinyHttpsRequest* request, int timeout_ms);

void HttpsRequestSetMaxRedirections(TinyHttpsRequest* request, int max_redirections);
void HttpsRequestAddHeader(TinyHttpsRequest* request, const char* key, const char* value);
TinyHttpsResponse* HttpsRequestFetch(TinyHttpsRequest* request);
bool HttpsResponse_error(TinyHttpsResponse* response);
const char* HttpsResponseGetErrorMsG(TinyHttpsResponse* response);
int HttpsResponseGetErrorCode(TinyHttpsResponse* response);
int HttpsResponseGetStatusCode(TinyHttpsResponse* response);
int HttpsResponseGetBodySize(TinyHttpsResponse* response);
const char* HttpsResponseGetHeaderValueByKey(TinyHttpsResponse* response, const char* key);
const char* HttpsResponseReadBody(TinyHttpsResponse* response, int* size);
const char* HttpsResponseReadBodyStr(TinyHttpsResponse* response, int* size);  

#endif
