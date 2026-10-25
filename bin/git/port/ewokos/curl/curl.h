#ifndef EWOKOS_GIT_CURL_H
#define EWOKOS_GIT_CURL_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/select.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBCURL_VERSION "7.66.0-ewokos-tinyhttpsc"
#define LIBCURL_VERSION_NUM 0x074200
#define CURL_ERROR_SIZE 256
#define CURL_GLOBAL_ALL 3

typedef struct CURL CURL;
typedef struct CURLM CURLM;
typedef int64_t curl_off_t;

typedef enum {
    CURLE_OK = 0,
    CURLE_UNSUPPORTED_PROTOCOL = 1,
    CURLE_FAILED_INIT = 2,
    CURLE_URL_MALFORMAT = 3,
    CURLE_NOT_BUILT_IN = 4,
    CURLE_COULDNT_RESOLVE_PROXY = 5,
    CURLE_COULDNT_RESOLVE_HOST = 6,
    CURLE_COULDNT_CONNECT = 7,
    CURLE_FTP_COULDNT_RETR_FILE = 19,
    CURLE_HTTP_RETURNED_ERROR = 22,
    CURLE_WRITE_ERROR = 23,
    CURLE_READ_ERROR = 26,
    CURLE_OUT_OF_MEMORY = 27,
    CURLE_OPERATION_TIMEDOUT = 28,
    CURLE_SSL_CONNECT_ERROR = 35,
    CURLE_FILE_COULDNT_READ_FILE = 37,
    CURLE_TOO_MANY_REDIRECTS = 47,
    CURLE_SSL_CERTPROBLEM = 58,
    CURLE_SSL_PINNEDPUBKEYNOTMATCH = 90
} CURLcode;

typedef enum {
    CURLM_OK = 0,
    CURLM_CALL_MULTI_PERFORM = -1,
    CURLM_BAD_HANDLE = 1,
    CURLM_BAD_EASY_HANDLE = 2,
    CURLM_OUT_OF_MEMORY = 3
} CURLMcode;

typedef enum {
    CURLOPT_URL = 1,
    CURLOPT_WRITEDATA,
    CURLOPT_WRITEFUNCTION,
    CURLOPT_HEADERDATA,
    CURLOPT_HEADERFUNCTION,
    CURLOPT_READDATA,
    CURLOPT_READFUNCTION,
    CURLOPT_INFILE,
    CURLOPT_INFILESIZE,
    CURLOPT_INFILESIZE_LARGE,
    CURLOPT_SEEKFUNCTION,
    CURLOPT_SEEKDATA,
    CURLOPT_POST,
    CURLOPT_POSTFIELDS,
    CURLOPT_POSTFIELDSIZE,
    CURLOPT_POSTFIELDSIZE_LARGE,
    CURLOPT_NOBODY,
    CURLOPT_UPLOAD,
    CURLOPT_HTTPGET,
    CURLOPT_CUSTOMREQUEST,
    CURLOPT_HTTPHEADER,
    CURLOPT_USERAGENT,
    CURLOPT_RANGE,
    CURLOPT_ENCODING,
    CURLOPT_FAILONERROR,
    CURLOPT_FOLLOWLOCATION,
    CURLOPT_MAXREDIRS,
    CURLOPT_POSTREDIR,
    CURLOPT_ERRORBUFFER,
    CURLOPT_USERPWD,
    CURLOPT_USERNAME,
    CURLOPT_PASSWORD,
    CURLOPT_HTTPAUTH,
    CURLOPT_NETRC,
    CURLOPT_COOKIEFILE,
    CURLOPT_COOKIEJAR,
    CURLOPT_RESOLVE,
    CURLOPT_SSL_VERIFYPEER,
    CURLOPT_SSL_VERIFYHOST,
    CURLOPT_SSLVERSION,
    CURLOPT_SSL_CIPHER_LIST,
    CURLOPT_SSLCERT,
    CURLOPT_SSLCERTTYPE,
    CURLOPT_SSLKEY,
    CURLOPT_SSLKEYTYPE,
    CURLOPT_KEYPASSWD,
    CURLOPT_CAPATH,
    CURLOPT_CAINFO,
    CURLOPT_PINNEDPUBLICKEY,
    CURLOPT_SSL_OPTIONS,
    CURLOPT_LOW_SPEED_LIMIT,
    CURLOPT_LOW_SPEED_TIME,
    CURLOPT_HTTP_VERSION,
    CURLOPT_GSSAPI_DELEGATION,
    CURLOPT_TCP_KEEPALIVE,
    CURLOPT_SOCKOPTFUNCTION,
    CURLOPT_VERBOSE,
    CURLOPT_DEBUGFUNCTION,
    CURLOPT_DEBUGDATA,
    CURLOPT_USE_SSL,
    CURLOPT_FTP_USE_EPSV,
    CURLOPT_PROXY,
    CURLOPT_PROXYTYPE,
    CURLOPT_PROXYUSERNAME,
    CURLOPT_PROXYPASSWORD,
    CURLOPT_PROXYHEADER,
    CURLOPT_PROXYAUTH,
    CURLOPT_PROXY_SSLCERT,
    CURLOPT_PROXY_SSLKEY,
    CURLOPT_PROXY_KEYPASSWD,
    CURLOPT_PROXY_CAINFO,
    CURLOPT_NOPROXY,
    CURLOPT_PROTOCOLS,
    CURLOPT_REDIR_PROTOCOLS,
    CURLOPT_IPRESOLVE
} CURLoption;

typedef enum {
    CURLINFO_TEXT = 0,
    CURLINFO_HEADER_IN,
    CURLINFO_HEADER_OUT,
    CURLINFO_DATA_IN,
    CURLINFO_DATA_OUT,
    CURLINFO_SSL_DATA_IN,
    CURLINFO_SSL_DATA_OUT,
    CURLINFO_RESPONSE_CODE = 100,
    CURLINFO_HTTP_CODE = CURLINFO_RESPONSE_CODE,
    CURLINFO_HTTPAUTH_AVAIL,
    CURLINFO_HTTP_CONNECTCODE,
    CURLINFO_CONTENT_TYPE,
    CURLINFO_EFFECTIVE_URL
} CURLINFO;

#define CURLAUTH_BASIC (1UL << 0)
#define CURLAUTH_DIGEST (1UL << 1)
#define CURLAUTH_GSSNEGOTIATE (1UL << 2)
#define CURLAUTH_NTLM (1UL << 3)
#define CURLAUTH_DIGEST_IE (1UL << 4)
#define CURLAUTH_ANY (~0UL)
#define CURLAUTH_ANYSAFE (~(CURLAUTH_BASIC | CURLAUTH_DIGEST_IE))

#define CURLPROTO_HTTP (1UL << 0)
#define CURLPROTO_HTTPS (1UL << 1)
#define CURLPROTO_FTP (1UL << 2)
#define CURLPROTO_FTPS (1UL << 3)
#define CURLPROTO_ALL (~0UL)

#define CURL_IPRESOLVE_WHATEVER 0
#define CURL_IPRESOLVE_V4 1
#define CURL_IPRESOLVE_V6 2
#define CURL_NETRC_OPTIONAL 1
#define CURL_REDIR_POST_301 1
#define CURL_REDIR_POST_302 2
#define CURL_REDIR_POST_303 4
#define CURL_REDIR_POST_ALL 7
#define CURLUSESSL_TRY 2
#define CURL_HTTP_VERSION_1_1 2
#define CURL_HTTP_VERSION_2 3
#define CURL_SSLVERSION_SSLv2 2
#define CURL_SSLVERSION_SSLv3 3
#define CURL_SSLVERSION_TLSv1 1
#define CURL_SSLVERSION_TLSv1_0 4
#define CURL_SSLVERSION_TLSv1_1 5
#define CURL_SSLVERSION_TLSv1_2 6
#define CURL_SSLVERSION_TLSv1_3 7
#define CURLSSLOPT_NO_REVOKE 2
#define CURLPROXY_HTTP 0
#define CURLPROXY_SOCKS4 4
#define CURLPROXY_SOCKS5 5
#define CURLPROXY_SOCKS4A 6
#define CURLPROXY_SOCKS5_HOSTNAME 7
#define CURLPROXY_HTTPS 2
#define CURL_SEEKFUNC_OK 0
#define CURL_SEEKFUNC_FAIL 2
#define CURL_SOCKOPT_OK 0

struct curl_slist {
    char *data;
    struct curl_slist *next;
};

typedef int curl_infotype;
typedef size_t (*curl_write_callback)(char *, size_t, size_t, void *);
typedef int (*curl_debug_callback)(CURL *, curl_infotype, char *, size_t, void *);
typedef int (*curl_seek_callback)(void *, curl_off_t, int);

typedef enum { CURLMSG_NONE = 0, CURLMSG_DONE = 1 } CURLMSG;
typedef struct {
    CURLMSG msg;
    CURL *easy_handle;
    union { CURLcode result; } data;
} CURLMsg;

typedef enum { CURLVERSION_NOW = 0 } CURLversion;
typedef struct {
    CURLversion age;
    const char *version;
    unsigned int version_num;
    const char *host;
    int features;
    const char *ssl_version;
} curl_version_info_data;

typedef int curl_ssl_backend_id;
typedef struct {
    curl_ssl_backend_id id;
    const char *name;
} curl_ssl_backend;
typedef enum {
    CURLSSLSET_OK = 0,
    CURLSSLSET_UNKNOWN_BACKEND,
    CURLSSLSET_TOO_LATE,
    CURLSSLSET_NO_BACKENDS
} CURLsslset;

CURLsslset curl_global_sslset(curl_ssl_backend_id id, const char *name,
        const curl_ssl_backend ***available_backends);
CURLcode curl_global_init(long flags);
void curl_global_cleanup(void);
const char *curl_version(void);
curl_version_info_data *curl_version_info(CURLversion version);

CURL *curl_easy_init(void);
CURL *curl_easy_duphandle(CURL *handle);
void curl_easy_cleanup(CURL *handle);
CURLcode curl_easy_setopt(CURL *handle, CURLoption option, ...);
CURLcode curl_easy_getinfo(CURL *handle, CURLINFO info, ...);
CURLcode curl_easy_perform(CURL *handle);
const char *curl_easy_strerror(CURLcode code);
char *curl_easy_escape(CURL *handle, const char *string, int length);
void curl_free(void *ptr);

CURLM *curl_multi_init(void);
CURLMcode curl_multi_cleanup(CURLM *multi);
CURLMcode curl_multi_add_handle(CURLM *multi, CURL *handle);
CURLMcode curl_multi_remove_handle(CURLM *multi, CURL *handle);
CURLMcode curl_multi_perform(CURLM *multi, int *running_handles);
CURLMsg *curl_multi_info_read(CURLM *multi, int *messages_in_queue);
CURLMcode curl_multi_timeout(CURLM *multi, long *milliseconds);
CURLMcode curl_multi_fdset(CURLM *multi, fd_set *read_fd_set,
        fd_set *write_fd_set, fd_set *exc_fd_set, int *max_fd);
const char *curl_multi_strerror(CURLMcode code);

struct curl_slist *curl_slist_append(struct curl_slist *list, const char *data);
void curl_slist_free_all(struct curl_slist *list);

#ifdef __cplusplus
}
#endif
#endif
