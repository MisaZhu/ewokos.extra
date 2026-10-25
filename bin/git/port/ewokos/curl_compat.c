#include <curl/curl.h>
#include <tinyhttpsc/tinyhttpsc.h>

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

struct CURL {
    char *url;
    char *custom_request;
    char *user_agent;
    char *range;
    char *username;
    char *password;
    char *userpwd;
    char *proxy;
    char *cookie_file;
    char *content_type;
    char *effective_url;
    struct curl_slist *headers;
    struct curl_slist *resolve;
    const void *post_fields;
    curl_off_t post_size;
    curl_off_t infile_size;
    void *write_data;
    void *header_data;
    void *read_data;
    void *seek_data;
    void *error_buffer;
    void *debug_data;
    curl_write_callback write_callback;
    curl_write_callback header_callback;
    curl_write_callback read_callback;
    curl_seek_callback seek_callback;
    curl_debug_callback debug_callback;
    long httpauth;
    long protocols;
    long redir_protocols;
    long maxredirs;
    long postredir;
    long response_code;
    long auth_avail;
    long connect_code;
    int post;
    int upload;
    int nobody;
    int failonerror;
    int followlocation;
    int ssl_verifypeer;
    int ssl_verifyhost;
    int verbose;
    int unsupported_option;
    int added;
    int completed;
    CURLcode result;
    CURL *next;
};

typedef struct CurlMessageNode {
    CURLMsg message;
    struct CurlMessageNode *next;
} CurlMessageNode;

struct CURLM {
    CURL *handles;
    CurlMessageNode *messages_head;
    CurlMessageNode *messages_tail;
};

typedef struct {
    unsigned char *data;
    size_t size;
    size_t capacity;
} CurlBuffer;

static char *curl_strdup(const char *value)
{
    if (!value)
        return NULL;
    size_t len = strlen(value) + 1;
    char *copy = malloc(len);
    if (copy)
        memcpy(copy, value, len);
    return copy;
}

static int curl_replace_string(char **target, const char *value)
{
    char *copy = curl_strdup(value);
    if (value && !copy)
        return -1;
    free(*target);
    *target = copy;
    return 0;
}

static void curl_set_error(CURL *handle, CURLcode code, const char *message)
{
    handle->result = code;
    if (handle->error_buffer && message) {
        char *error_buffer = handle->error_buffer;
        strncpy(error_buffer, message, CURL_ERROR_SIZE - 1);
        error_buffer[CURL_ERROR_SIZE - 1] = 0;
    }
}

static CURLcode curl_memory_error(CURL *handle)
{
    curl_set_error(handle, CURLE_OUT_OF_MEMORY, "out of memory");
    return CURLE_OUT_OF_MEMORY;
}

static int curl_buffer_append(CurlBuffer *buffer, const void *data, size_t size)
{
    if (!size)
        return 0;
    if (size > SIZE_MAX - buffer->size)
        return -1;
    size_t needed = buffer->size + size;
    if (needed > buffer->capacity) {
        size_t capacity = buffer->capacity ? buffer->capacity : 16384;
        while (capacity < needed) {
            if (capacity > SIZE_MAX / 2) {
                capacity = needed;
                break;
            }
            capacity *= 2;
        }
        unsigned char *new_data = realloc(buffer->data, capacity);
        if (!new_data)
            return -1;
        buffer->data = new_data;
        buffer->capacity = capacity;
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size = needed;
    return 0;
}

CURLsslset curl_global_sslset(curl_ssl_backend_id id, const char *name,
        const curl_ssl_backend ***available_backends)
{
    static const curl_ssl_backend backend = { 0, "bearssl" };
    static const curl_ssl_backend *backends[] = { &backend, NULL };
    if (available_backends)
        *available_backends = backends;
    (void)id;
    if (name && strcasecmp(name, backend.name))
        return CURLSSLSET_UNKNOWN_BACKEND;
    return CURLSSLSET_OK;
}

CURLcode curl_global_init(long flags)
{
    (void)flags;
    return CURLE_OK;
}

void curl_global_cleanup(void)
{
}

const char *curl_version(void)
{
    return LIBCURL_VERSION;
}

curl_version_info_data *curl_version_info(CURLversion version)
{
    (void)version;
    static curl_version_info_data info = {
        CURLVERSION_NOW, LIBCURL_VERSION, LIBCURL_VERSION_NUM,
        "ewokos-aarch64", 0, "BearSSL/tinyhttpsc"
    };
    return &info;
}

CURL *curl_easy_init(void)
{
    CURL *handle = calloc(1, sizeof(*handle));
    if (!handle)
        return NULL;
    handle->post_size = -1;
    handle->infile_size = -1;
    handle->maxredirs = 20;
    handle->protocols = CURLPROTO_HTTP | CURLPROTO_HTTPS;
    handle->redir_protocols = CURLPROTO_HTTP | CURLPROTO_HTTPS;
    handle->ssl_verifypeer = 1;
    handle->ssl_verifyhost = 2;
    return handle;
}

static void curl_clear_transfer_results(CURL *handle)
{
    free(handle->content_type);
    handle->content_type = NULL;
    free(handle->effective_url);
    handle->effective_url = NULL;
    handle->response_code = 0;
    handle->auth_avail = 0;
    handle->connect_code = 0;
    handle->result = CURLE_OK;
    handle->completed = 0;
    if (handle->error_buffer)
        ((char *)handle->error_buffer)[0] = 0;
}

CURL *curl_easy_duphandle(CURL *handle)
{
    if (!handle)
        return NULL;
    CURL *copy = curl_easy_init();
    if (!copy)
        return NULL;
    *copy = *handle;
    copy->next = NULL;
    copy->added = 0;
    copy->completed = 0;
    copy->url = NULL;
    copy->custom_request = NULL;
    copy->user_agent = NULL;
    copy->range = NULL;
    copy->username = NULL;
    copy->password = NULL;
    copy->userpwd = NULL;
    copy->proxy = NULL;
    copy->cookie_file = NULL;
    copy->content_type = NULL;
    copy->effective_url = NULL;

#define DUP_STRING(member) do { \
    if (handle->member) { \
        copy->member = curl_strdup(handle->member); \
        if (!copy->member) { \
            curl_easy_cleanup(copy); \
            return NULL; \
        } \
    } \
} while (0)
    DUP_STRING(url);
    DUP_STRING(custom_request);
    DUP_STRING(user_agent);
    DUP_STRING(range);
    DUP_STRING(username);
    DUP_STRING(password);
    DUP_STRING(userpwd);
    DUP_STRING(proxy);
    DUP_STRING(cookie_file);
    DUP_STRING(content_type);
    DUP_STRING(effective_url);
#undef DUP_STRING
    return copy;
}

void curl_easy_cleanup(CURL *handle)
{
    if (!handle)
        return;
    free(handle->url);
    free(handle->custom_request);
    free(handle->user_agent);
    free(handle->range);
    free(handle->username);
    free(handle->password);
    free(handle->userpwd);
    free(handle->proxy);
    free(handle->cookie_file);
    free(handle->content_type);
    free(handle->effective_url);
    free(handle);
}

static CURLcode curl_set_long_option(CURL *handle, CURLoption option, long value)
{
    switch (option) {
    case CURLOPT_INFILESIZE: handle->infile_size = value; break;
    case CURLOPT_POSTFIELDSIZE: handle->post_size = value; break;
    case CURLOPT_POST:
        handle->post = !!value;
        if (value) {
            handle->upload = 0;
            handle->nobody = 0;
        }
        break;
    case CURLOPT_NOBODY: handle->nobody = !!value; break;
    case CURLOPT_UPLOAD:
        handle->upload = !!value;
        if (value) {
            handle->post = 0;
            handle->nobody = 0;
        }
        break;
    case CURLOPT_HTTPGET:
        if (value) {
            handle->post = 0;
            handle->upload = 0;
            handle->nobody = 0;
        }
        break;
    case CURLOPT_FAILONERROR: handle->failonerror = !!value; break;
    case CURLOPT_FOLLOWLOCATION: handle->followlocation = !!value; break;
    case CURLOPT_MAXREDIRS: handle->maxredirs = value; break;
    case CURLOPT_POSTREDIR: handle->postredir = value; break;
    case CURLOPT_HTTPAUTH: handle->httpauth = value; break;
    case CURLOPT_SSL_VERIFYPEER: handle->ssl_verifypeer = !!value; break;
    case CURLOPT_SSL_VERIFYHOST: handle->ssl_verifyhost = (int)value; break;
    case CURLOPT_PROTOCOLS: handle->protocols = value; break;
    case CURLOPT_REDIR_PROTOCOLS: handle->redir_protocols = value; break;
    case CURLOPT_VERBOSE: handle->verbose = !!value; break;
    case CURLOPT_NETRC:
    case CURLOPT_TCP_KEEPALIVE:
    case CURLOPT_PROXYTYPE:
    case CURLOPT_PROXYAUTH:
        break;
    case CURLOPT_IPRESOLVE:
        if (value == CURL_IPRESOLVE_V6) {
            handle->unsupported_option = option;
            return CURLE_NOT_BUILT_IN;
        }
        break;
    case CURLOPT_HTTP_VERSION:
        if (value && value != CURL_HTTP_VERSION_1_1) {
            handle->unsupported_option = option;
            return CURLE_NOT_BUILT_IN;
        }
        break;
    case CURLOPT_SSLVERSION:
    case CURLOPT_SSL_OPTIONS:
    case CURLOPT_LOW_SPEED_LIMIT:
    case CURLOPT_LOW_SPEED_TIME:
    case CURLOPT_GSSAPI_DELEGATION:
    case CURLOPT_USE_SSL:
    case CURLOPT_FTP_USE_EPSV:
        if (value) {
            handle->unsupported_option = option;
            return CURLE_NOT_BUILT_IN;
        }
        break;
    default:
        handle->unsupported_option = option;
        return CURLE_NOT_BUILT_IN;
    }
    return CURLE_OK;
}

CURLcode curl_easy_setopt(CURL *handle, CURLoption option, ...)
{
    if (!handle)
        return CURLE_FAILED_INIT;
    va_list args;
    va_start(args, option);
    CURLcode result = CURLE_OK;
    const char *string_value;

    switch (option) {
    case CURLOPT_URL:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->url, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_CUSTOMREQUEST:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->custom_request, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_USERAGENT:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->user_agent, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_RANGE:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->range, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_USERNAME:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->username, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_PASSWORD:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->password, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_USERPWD:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->userpwd, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_PROXY:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->proxy, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_COOKIEFILE:
        string_value = va_arg(args, const char *);
        if (curl_replace_string(&handle->cookie_file, string_value)) result = curl_memory_error(handle);
        break;
    case CURLOPT_POSTFIELDS:
        handle->post_fields = va_arg(args, const void *);
        break;
    case CURLOPT_POSTFIELDSIZE_LARGE:
        handle->post_size = va_arg(args, curl_off_t);
        break;
    case CURLOPT_INFILESIZE_LARGE:
        handle->infile_size = va_arg(args, curl_off_t);
        break;
    case CURLOPT_HTTPHEADER: handle->headers = va_arg(args, struct curl_slist *); break;
    case CURLOPT_RESOLVE: handle->resolve = va_arg(args, struct curl_slist *); break;
    case CURLOPT_WRITEDATA: handle->write_data = va_arg(args, void *); break;
    case CURLOPT_HEADERDATA: handle->header_data = va_arg(args, void *); break;
    case CURLOPT_INFILE:
    case CURLOPT_READDATA: handle->read_data = va_arg(args, void *); break;
    case CURLOPT_SEEKDATA: handle->seek_data = va_arg(args, void *); break;
    case CURLOPT_ERRORBUFFER: handle->error_buffer = va_arg(args, void *); break;
    case CURLOPT_DEBUGDATA: handle->debug_data = va_arg(args, void *); break;
    case CURLOPT_WRITEFUNCTION: handle->write_callback = (curl_write_callback)va_arg(args, void *); break;
    case CURLOPT_HEADERFUNCTION: handle->header_callback = (curl_write_callback)va_arg(args, void *); break;
    case CURLOPT_READFUNCTION: handle->read_callback = (curl_write_callback)va_arg(args, void *); break;
    case CURLOPT_SEEKFUNCTION: handle->seek_callback = (curl_seek_callback)va_arg(args, void *); break;
    case CURLOPT_DEBUGFUNCTION: handle->debug_callback = (curl_debug_callback)va_arg(args, void *); break;
    case CURLOPT_ENCODING:
        string_value = va_arg(args, const char *);
        if (string_value && *string_value) {
            handle->unsupported_option = option;
            result = CURLE_NOT_BUILT_IN;
        }
        break;
    case CURLOPT_PROXYHEADER:
        if (va_arg(args, struct curl_slist *)) {
            handle->unsupported_option = option;
            result = CURLE_NOT_BUILT_IN;
        }
        break;
    case CURLOPT_SSL_CIPHER_LIST:
    case CURLOPT_CAPATH:
    case CURLOPT_CAINFO:
    case CURLOPT_SSLCERT:
    case CURLOPT_SSLCERTTYPE:
    case CURLOPT_SSLKEY:
    case CURLOPT_SSLKEYTYPE:
    case CURLOPT_KEYPASSWD:
    case CURLOPT_PINNEDPUBLICKEY:
    case CURLOPT_COOKIEJAR:
    case CURLOPT_PROXYUSERNAME:
    case CURLOPT_PROXYPASSWORD:
    case CURLOPT_PROXY_SSLCERT:
    case CURLOPT_PROXY_SSLKEY:
    case CURLOPT_PROXY_KEYPASSWD:
    case CURLOPT_PROXY_CAINFO:
        string_value = va_arg(args, const char *);
        if (string_value && *string_value) {
            handle->unsupported_option = option;
            result = CURLE_NOT_BUILT_IN;
        }
        break;
    case CURLOPT_NOPROXY:
        (void)va_arg(args, const char *);
        break;
    case CURLOPT_SOCKOPTFUNCTION:
        (void)va_arg(args, void *);
        break;
    default:
        result = curl_set_long_option(handle, option, va_arg(args, long));
        break;
    }
    va_end(args);
    return result;
}

static int curl_scheme_allowed(const char *url, long protocols, int redirect)
{
    (void)redirect;
    if (!strncasecmp(url, "https://", 8))
        return !!(protocols & CURLPROTO_HTTPS);
    if (!strncasecmp(url, "http://", 7))
        return !!(protocols & CURLPROTO_HTTP);
    return 0;
}

static int curl_same_origin(const char *a, const char *b)
{
    const char *a_host = strstr(a, "://");
    const char *b_host = strstr(b, "://");
    if (!a_host || !b_host)
        return 1;
    size_t a_scheme = a_host - a;
    size_t b_scheme = b_host - b;
    a_host += 3;
    b_host += 3;
    const char *a_end = strchr(a_host, '/');
    const char *b_end = strchr(b_host, '/');
    size_t a_len = a_end ? (size_t)(a_end - a_host) : strlen(a_host);
    size_t b_len = b_end ? (size_t)(b_end - b_host) : strlen(b_host);
    return a_scheme == b_scheme && a_len == b_len &&
        !strncasecmp(a, b, a_scheme) && !strncasecmp(a_host, b_host, a_len);
}

static char *curl_redirect_url(const char *base, const char *location)
{
    if (strstr(location, "://"))
        return curl_strdup(location);
    const char *scheme = strstr(base, "://");
    if (!scheme)
        return NULL;
    size_t needed = strlen(base) + strlen(location) + 3;
    char *result = malloc(needed);
    if (!result)
        return NULL;
    if (location[0] == '/' && location[1] == '/') {
        snprintf(result, needed, "%.*s:%s", (int)(scheme - base), base, location);
        return result;
    }

    const char *host = scheme + 3;
    const char *path = strchr(host, '/');
    const char *origin_end = path ? path : base + strlen(base);
    if (location[0] == '/') {
        snprintf(result, needed, "%.*s%s", (int)(origin_end - base), base, location);
        return result;
    }

    const char *end = path ? path + strlen(path) : origin_end;
    const char *query = path ? strpbrk(path, "?#") : NULL;
    if (query)
        end = query;
    if (location[0] == '?') {
        snprintf(result, needed, "%.*s%s", (int)(end - base), base, location);
        return result;
    }

    const char *last = NULL;
    for (const char *p = path; p && p < end; p++)
        if (*p == '/')
            last = p;
    size_t prefix = last ? (size_t)(last - base + 1) : (size_t)(origin_end - base);
    snprintf(result, needed, "%.*s%s", (int)prefix, base, location);
    return result;
}

static int curl_header_name_is(const char *line, size_t len, const char *name)
{
    size_t name_len = strlen(name);
    return len > name_len && line[name_len] == ':' && !strncasecmp(line, name, name_len);
}

static int curl_headers_contain(CURL *handle, const char *name)
{
    for (struct curl_slist *item = handle->headers; item; item = item->next)
        if (curl_header_name_is(item->data, strlen(item->data), name))
            return 1;
    return 0;
}

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *curl_basic_auth(CURL *handle)
{
    const char *user = handle->username;
    const char *password = handle->password;
    char *combined = NULL;
    if (!user && handle->userpwd) {
        combined = curl_strdup(handle->userpwd);
    } else if (user) {
        size_t len = strlen(user) + strlen(password ? password : "") + 2;
        combined = malloc(len);
        if (combined)
            snprintf(combined, len, "%s:%s", user, password ? password : "");
    }
    if (!combined)
        return NULL;
    size_t len = strlen(combined);
    size_t out_len = ((len + 2) / 3) * 4;
    char *out = malloc(out_len + 16);
    if (!out) {
        free(combined);
        return NULL;
    }
    memcpy(out, "Basic ", 6);
    size_t i = 0, o = 6;
    while (i < len) {
        size_t remaining = len - i;
        uint32_t a = (unsigned char)combined[i++];
        uint32_t b = remaining > 1 ? (unsigned char)combined[i++] : 0;
        uint32_t c = remaining > 2 ? (unsigned char)combined[i++] : 0;
        uint32_t value = (a << 16) | (b << 8) | c;
        out[o++] = b64_table[(value >> 18) & 63];
        out[o++] = b64_table[(value >> 12) & 63];
        out[o++] = remaining > 1 ? b64_table[(value >> 6) & 63] : '=';
        out[o++] = remaining > 2 ? b64_table[value & 63] : '=';
    }
    out[o] = 0;
    free(combined);
    return out;
}

static void curl_debug(CURL *handle, int type, const char *data, size_t size)
{
    if (handle->verbose && handle->debug_callback)
        handle->debug_callback(handle, type, (char *)data, size, handle->debug_data);
}

static int curl_add_request_headers(CURL *handle, TinyHttpsRequest *request,
        const char *url, const char *original_url)
{
    int same_origin = curl_same_origin(url, original_url);
    for (struct curl_slist *item = handle->headers; item; item = item->next) {
        const char *colon = strchr(item->data, ':');
        if (!colon)
            continue;
        size_t key_len = colon - item->data;
        const char *value = colon + 1;
        while (*value == ' ' || *value == '\t') value++;
        if (!*value)
            continue;
        if (!same_origin && (curl_header_name_is(item->data, key_len + 1, "Authorization") ||
                curl_header_name_is(item->data, key_len + 1, "Cookie")))
            continue;
        char key[128];
        if (key_len >= sizeof(key))
            return -1;
        memcpy(key, item->data, key_len);
        key[key_len] = 0;
        HttpsRequestAddHeader(request, key, value);
        char line[512];
        int len = snprintf(line, sizeof(line), "%s: %s\r\n", key, value);
        if (len > 0) {
            size_t size = (size_t)len < sizeof(line) ? (size_t)len : sizeof(line) - 1;
            curl_debug(handle, CURLINFO_HEADER_OUT, line, size);
        }
    }
    if (handle->user_agent && !curl_headers_contain(handle, "User-Agent"))
        HttpsRequestAddHeader(request, "User-Agent", handle->user_agent);
    if (handle->range && !curl_headers_contain(handle, "Range"))
        HttpsRequestAddHeader(request, "Range", handle->range);
    if (same_origin && (handle->httpauth & CURLAUTH_BASIC) &&
            !curl_headers_contain(handle, "Authorization")) {
        char *auth = curl_basic_auth(handle);
        if (auth) {
            HttpsRequestAddHeader(request, "Authorization", auth);
            free(auth);
        }
    }
    return 0;
}

static CURLcode curl_collect_request_body(CURL *handle, CurlBuffer *body)
{
    if (handle->post_fields) {
        size_t size = handle->post_size >= 0 ? (size_t)handle->post_size : strlen(handle->post_fields);
        if (curl_buffer_append(body, handle->post_fields, size))
            return curl_memory_error(handle);
        return CURLE_OK;
    }
    if (!handle->read_callback)
        return CURLE_OK;
    char buffer[16384];
    for (;;) {
        size_t want = sizeof(buffer);
        if (handle->infile_size >= 0) {
            if ((curl_off_t)body->size >= handle->infile_size)
                break;
            if ((curl_off_t)want > handle->infile_size - (curl_off_t)body->size)
                want = (size_t)(handle->infile_size - (curl_off_t)body->size);
        }
        size_t got = handle->read_callback(buffer, 1, want, handle->read_data);
        if (!got)
            break;
        if (got > want || curl_buffer_append(body, buffer, got)) {
            curl_set_error(handle, CURLE_READ_ERROR, "failed to buffer request body");
            return CURLE_READ_ERROR;
        }
        curl_debug(handle, CURLINFO_DATA_OUT, buffer, got);
    }
    return CURLE_OK;
}

static int curl_deliver_header(CURL *handle, const char *data, size_t size)
{
    curl_debug(handle, CURLINFO_HEADER_IN, data, size);
    if (!handle->header_callback)
        return 0;
    size_t written = handle->header_callback((char *)data, 1, size, handle->header_data);
    return written == size ? 0 : -1;
}

static int curl_process_response_headers(CURL *handle, TinyHttpsResponse *response)
{
    int status = HttpsResponseGetStatusCode(response);
    char status_line[64];
    int len = snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d\r\n", status);
    if (curl_deliver_header(handle, status_line, len))
        return -1;
    int count = BearHttpsResponse_get_headers_size(response);
    for (int i = 0; i < count; i++) {
        const char *key = BearHttpsResponse_get_header_key_by_index(response, i);
        const char *value = BearHttpsResponse_get_header_value_by_index(response, i);
        if (!key || !value)
            continue;
        size_t line_size = strlen(key) + strlen(value) + 5;
        char *line = malloc(line_size);
        if (!line)
            return -1;
        snprintf(line, line_size, "%s: %s\r\n", key, value);
        int failed = curl_deliver_header(handle, line, line_size - 1);
        free(line);
        if (failed)
            return -1;
        if (!strcasecmp(key, "Content-Type")) {
            if (curl_replace_string(&handle->content_type, value))
                return -1;
        } else if (!strcasecmp(key, "WWW-Authenticate")) {
            if (!strncasecmp(value, "Basic", 5)) handle->auth_avail |= CURLAUTH_BASIC;
            if (!strncasecmp(value, "Digest", 6)) handle->auth_avail |= CURLAUTH_DIGEST;
            if (!strncasecmp(value, "NTLM", 4)) handle->auth_avail |= CURLAUTH_NTLM;
            if (!strncasecmp(value, "Negotiate", 9)) handle->auth_avail |= CURLAUTH_GSSNEGOTIATE;
        }
    }
    return curl_deliver_header(handle, "\r\n", 2);
}

static CURLcode curl_deliver_body(CURL *handle, TinyHttpsResponse *response)
{
    if (handle->nobody)
        return CURLE_OK;
    unsigned char buffer[16384];
    for (;;) {
        int got = HttpsResponseReadBodyChunk(response, buffer, sizeof(buffer));
        if (got < 0) {
            curl_set_error(handle, CURLE_READ_ERROR, "failed while reading HTTP response body");
            return CURLE_READ_ERROR;
        }
        if (!got)
            break;
        curl_debug(handle, CURLINFO_DATA_IN, (char *)buffer, got);
        size_t written;
        if (handle->write_callback)
            written = handle->write_callback((char *)buffer, 1, got, handle->write_data);
        else
            written = fwrite(buffer, 1, got, stdout);
        if (written != (size_t)got) {
            curl_set_error(handle, CURLE_WRITE_ERROR, "HTTP response callback failed");
            return CURLE_WRITE_ERROR;
        }
    }
    return CURLE_OK;
}

static CURLcode curl_map_response_error(CURL *handle, TinyHttpsResponse *response)
{
    const char *message = HttpsResponseGetErrorMsg(response);
    int code = HttpsResponseGetErrorCode(response);
    CURLcode result = CURLE_COULDNT_CONNECT;
    if (code == 2)
        result = CURLE_URL_MALFORMAT;
    else if (code == 8)
        result = CURLE_OUT_OF_MEMORY;
    else if (code == 10 || code == 11)
        result = CURLE_COULDNT_RESOLVE_HOST;
    else if (code == 6 || code == 15 || code == 16 || code == 20)
        result = CURLE_READ_ERROR;
    else if ((code == 17 || code == 18) && handle->url &&
            !strncasecmp(handle->url, "https://", 8))
        result = CURLE_SSL_CONNECT_ERROR;
    else if (message && strstr(message, "timeout"))
        result = CURLE_OPERATION_TIMEDOUT;
    curl_set_error(handle, result, message ? message : "tinyhttpsc request failed");
    return result;
}

static CURLcode curl_perform(CURL *handle)
{
    if (!handle->url)
        return CURLE_URL_MALFORMAT;
    if (handle->unsupported_option) {
        curl_set_error(handle, CURLE_NOT_BUILT_IN, "requested curl option is not supported by tinyhttpsc");
        return CURLE_NOT_BUILT_IN;
    }
    if (handle->proxy && *handle->proxy) {
        curl_set_error(handle, CURLE_NOT_BUILT_IN, "HTTP proxy is not supported by tinyhttpsc");
        return CURLE_NOT_BUILT_IN;
    }
    if ((handle->cookie_file && *handle->cookie_file) || handle->resolve) {
        curl_set_error(handle, CURLE_NOT_BUILT_IN, "cookie files and custom DNS resolution are not supported");
        return CURLE_NOT_BUILT_IN;
    }
    if (!curl_scheme_allowed(handle->url, handle->protocols, 0)) {
        curl_set_error(handle, CURLE_UNSUPPORTED_PROTOCOL, "only HTTP and HTTPS are supported");
        return CURLE_UNSUPPORTED_PROTOCOL;
    }
    if (!strncasecmp(handle->url, "https://", 8) &&
            (!handle->ssl_verifypeer || !handle->ssl_verifyhost)) {
        curl_set_error(handle, CURLE_SSL_CERTPROBLEM,
                "disabling TLS certificate verification is not supported by tinyhttpsc");
        return CURLE_SSL_CERTPROBLEM;
    }

    CurlBuffer body = {0};
    CURLcode result = curl_collect_request_body(handle, &body);
    if (result != CURLE_OK) {
        free(body.data);
        return result;
    }

    char *current_url = curl_strdup(handle->url);
    if (!current_url) {
        free(body.data);
        return curl_memory_error(handle);
    }
    int redirects = 0;
    int has_body = handle->post_fields || handle->read_callback;
    const char *method = handle->custom_request;
    if (!method) method = handle->upload ? "PUT" : handle->post ? "POST" : handle->nobody ? "HEAD" : "GET";

    for (;;) {
        TinyHttpsRequest *request = NewHttpsRequest(current_url);
        if (!request) {
            result = curl_memory_error(handle);
            break;
        }
        HttpsRequestSetMaxRedirections(request, 0);
        HttpsRequestSetTimeout(request, 30000);
        HttpsRequestSetMethod(request, method);
        if (curl_add_request_headers(handle, request, current_url, handle->url)) {
            HttpsRequestFree(request);
            result = curl_memory_error(handle);
            break;
        }
        if (has_body)
            HttpsRequestSendAny(request, body.data, (long)body.size);
        TinyHttpsResponse *response = HttpsRequestFetch(request);
        HttpsRequestFree(request);
        if (!response) {
            curl_set_error(handle, CURLE_COULDNT_CONNECT, "tinyhttpsc returned no response");
            result = CURLE_COULDNT_CONNECT;
            break;
        }
        if (HttpsResponseError(response)) {
            result = curl_map_response_error(handle, response);
            HttpsResponseFree(response);
            break;
        }

        handle->response_code = HttpsResponseGetStatusCode(response);
        if (curl_replace_string(&handle->effective_url, current_url)) {
            HttpsResponseFree(response);
            result = curl_memory_error(handle);
            break;
        }
        if (curl_process_response_headers(handle, response)) {
            HttpsResponseFree(response);
            result = curl_memory_error(handle);
            break;
        }

        int status = (int)handle->response_code;
        int is_redirect = status == 301 || status == 302 || status == 303 || status == 307 || status == 308;
        const char *location = HttpsResponseGetHeaderValueByKey(response, "Location");
        if (handle->followlocation && is_redirect && location) {
            if (redirects++ >= handle->maxredirs) {
                HttpsResponseFree(response);
                curl_set_error(handle, CURLE_TOO_MANY_REDIRECTS, "too many HTTP redirects");
                result = CURLE_TOO_MANY_REDIRECTS;
                break;
            }
            char *next_url = curl_redirect_url(current_url, location);
            if (!next_url) {
                HttpsResponseFree(response);
                result = curl_memory_error(handle);
                break;
            }
            if (!curl_scheme_allowed(next_url, handle->redir_protocols, 1)) {
                free(next_url);
                HttpsResponseFree(response);
                curl_set_error(handle, CURLE_UNSUPPORTED_PROTOCOL, "redirect protocol is not allowed");
                result = CURLE_UNSUPPORTED_PROTOCOL;
                break;
            }
            if (status == 303 || ((status == 301 || status == 302) && !(handle->postredir & (status == 301 ? CURL_REDIR_POST_301 : CURL_REDIR_POST_302)))) {
                method = "GET";
                body.size = 0;
                has_body = 0;
            }
            free(current_url);
            current_url = next_url;
            HttpsResponseFree(response);
            continue;
        }

        if (handle->failonerror && status >= 400) {
            curl_set_error(handle, CURLE_HTTP_RETURNED_ERROR, "HTTP server returned an error");
            result = CURLE_HTTP_RETURNED_ERROR;
        } else {
            result = curl_deliver_body(handle, response);
        }
        HttpsResponseFree(response);
        break;
    }

    free(current_url);
    free(body.data);
    return result;
}

CURLcode curl_easy_perform(CURL *handle)
{
    if (!handle)
        return CURLE_FAILED_INIT;
    curl_clear_transfer_results(handle);
    CURLcode result = curl_perform(handle);
    handle->result = result;
    handle->completed = 1;
    return result;
}

CURLcode curl_easy_getinfo(CURL *handle, CURLINFO info, ...)
{
    if (!handle)
        return CURLE_FAILED_INIT;
    va_list args;
    va_start(args, info);
    switch (info) {
    case CURLINFO_RESPONSE_CODE:
        *va_arg(args, long *) = handle->response_code;
        break;
    case CURLINFO_HTTPAUTH_AVAIL:
        *va_arg(args, long *) = handle->auth_avail;
        break;
    case CURLINFO_HTTP_CONNECTCODE:
        *va_arg(args, long *) = handle->connect_code;
        break;
    case CURLINFO_CONTENT_TYPE:
        *va_arg(args, const char **) = handle->content_type;
        break;
    case CURLINFO_EFFECTIVE_URL:
        *va_arg(args, const char **) = handle->effective_url ? handle->effective_url : handle->url;
        break;
    default:
        va_end(args);
        return CURLE_NOT_BUILT_IN;
    }
    va_end(args);
    return CURLE_OK;
}

const char *curl_easy_strerror(CURLcode code)
{
    switch (code) {
    case CURLE_OK: return "no error";
    case CURLE_UNSUPPORTED_PROTOCOL: return "unsupported protocol";
    case CURLE_NOT_BUILT_IN: return "feature not supported by tinyhttpsc backend";
    case CURLE_URL_MALFORMAT: return "malformed URL";
    case CURLE_COULDNT_RESOLVE_HOST: return "could not resolve host";
    case CURLE_COULDNT_CONNECT: return "could not connect";
    case CURLE_HTTP_RETURNED_ERROR: return "HTTP returned an error";
    case CURLE_WRITE_ERROR: return "response write failed";
    case CURLE_READ_ERROR: return "transfer read failed";
    case CURLE_OUT_OF_MEMORY: return "out of memory";
    case CURLE_OPERATION_TIMEDOUT: return "operation timed out";
    case CURLE_SSL_CONNECT_ERROR: return "TLS connection failed";
    case CURLE_SSL_CERTPROBLEM: return "TLS certificate problem";
    case CURLE_TOO_MANY_REDIRECTS: return "too many redirects";
    default: return "HTTP transfer failed";
    }
}

char *curl_easy_escape(CURL *handle, const char *string, int length)
{
    (void)handle;
    if (!string)
        return NULL;
    if (!length)
        length = strlen(string);
    char *result = malloc((size_t)length * 3 + 1);
    if (!result)
        return NULL;
    static const char hex[] = "0123456789ABCDEF";
    size_t out = 0;
    for (int i = 0; i < length; i++) {
        unsigned char c = string[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result[out++] = c;
        } else {
            result[out++] = '%';
            result[out++] = hex[c >> 4];
            result[out++] = hex[c & 15];
        }
    }
    result[out] = 0;
    return result;
}

void curl_free(void *ptr)
{
    free(ptr);
}

CURLM *curl_multi_init(void)
{
    return calloc(1, sizeof(CURLM));
}

CURLMcode curl_multi_cleanup(CURLM *multi)
{
    if (!multi)
        return CURLM_BAD_HANDLE;
    while (multi->messages_head) {
        CurlMessageNode *node = multi->messages_head;
        multi->messages_head = node->next;
        free(node);
    }
    free(multi);
    return CURLM_OK;
}

CURLMcode curl_multi_add_handle(CURLM *multi, CURL *handle)
{
    if (!multi || !handle)
        return CURLM_BAD_EASY_HANDLE;
    if (handle->added)
        return CURLM_OK;
    handle->added = 1;
    handle->completed = 0;
    handle->next = multi->handles;
    multi->handles = handle;
    return CURLM_OK;
}

CURLMcode curl_multi_remove_handle(CURLM *multi, CURL *handle)
{
    if (!multi || !handle)
        return CURLM_BAD_EASY_HANDLE;
    CURL **link = &multi->handles;
    while (*link && *link != handle)
        link = &(*link)->next;
    if (*link)
        *link = handle->next;
    handle->next = NULL;
    handle->added = 0;
    return CURLM_OK;
}

static CURLMcode curl_queue_done(CURLM *multi, CURL *handle)
{
    CurlMessageNode *node = calloc(1, sizeof(*node));
    if (!node)
        return CURLM_OUT_OF_MEMORY;
    node->message.msg = CURLMSG_DONE;
    node->message.easy_handle = handle;
    node->message.data.result = handle->result;
    if (multi->messages_tail)
        multi->messages_tail->next = node;
    else
        multi->messages_head = node;
    multi->messages_tail = node;
    return CURLM_OK;
}

CURLMcode curl_multi_perform(CURLM *multi, int *running_handles)
{
    if (!multi)
        return CURLM_BAD_HANDLE;
    for (CURL *handle = multi->handles; handle; handle = handle->next) {
        if (handle->completed)
            continue;
        curl_easy_perform(handle);
        CURLMcode result = curl_queue_done(multi, handle);
        if (result != CURLM_OK)
            return result;
    }
    int running = 0;
    for (CURL *handle = multi->handles; handle; handle = handle->next)
        if (!handle->completed) running++;
    if (running_handles)
        *running_handles = running;
    return CURLM_OK;
}

CURLMsg *curl_multi_info_read(CURLM *multi, int *messages_in_queue)
{
    if (!multi)
        return NULL;
    CurlMessageNode *node = multi->messages_head;
    if (node) {
        multi->messages_head = node->next;
        if (!multi->messages_head)
            multi->messages_tail = NULL;
    }
    int count = 0;
    for (CurlMessageNode *item = multi->messages_head; item; item = item->next)
        count++;
    if (messages_in_queue)
        *messages_in_queue = count;
    if (!node)
        return NULL;
    static CURLMsg message;
    message = node->message;
    free(node);
    return &message;
}

CURLMcode curl_multi_timeout(CURLM *multi, long *milliseconds)
{
    if (!multi)
        return CURLM_BAD_HANDLE;
    if (milliseconds)
        *milliseconds = 0;
    return CURLM_OK;
}

CURLMcode curl_multi_fdset(CURLM *multi, fd_set *read_fd_set,
        fd_set *write_fd_set, fd_set *exc_fd_set, int *max_fd)
{
    (void)multi;
    (void)read_fd_set;
    (void)write_fd_set;
    (void)exc_fd_set;
    if (max_fd)
        *max_fd = -1;
    return CURLM_OK;
}

const char *curl_multi_strerror(CURLMcode code)
{
    return code == CURLM_OK ? "no error" : "curl multi interface error";
}

struct curl_slist *curl_slist_append(struct curl_slist *list, const char *data)
{
    struct curl_slist *node = calloc(1, sizeof(*node));
    if (!node)
        return NULL;
    node->data = curl_strdup(data);
    if (!node->data) {
        free(node);
        return NULL;
    }
    if (!list)
        return node;
    struct curl_slist *tail = list;
    while (tail->next)
        tail = tail->next;
    tail->next = node;
    return list;
}

void curl_slist_free_all(struct curl_slist *list)
{
    while (list) {
        struct curl_slist *next = list->next;
        free(list->data);
        free(list);
        list = next;
    }
}
