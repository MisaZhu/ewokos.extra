#include <stddef.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <ewoksys/kernel_tic.h>
#include <ewoksys/proc.h>

#include "tinyhttpsc/tinyhttpsc.h"

#define BUFFER_SIZE  512
#define MAX_MESSAGES 100

// Message structure for conversation history
typedef struct {
    const char* role;
    char content[BUFFER_SIZE];
} Message;

static char* chat_with_context(Message* messages, int message_count) {
	const char *api_key = "863058a9-7b40-40e8-affc-f86b1496981e";
	const char *body;
	TinyHttpsRequest *request;
	TinyHttpsResponse *response;
	int timeout_ms = 5000;
	int body_size;

	// Create request body in JSON format with conversation history
	// Use static allocation to avoid stack overflow
	static char request_body[BUFFER_SIZE * 4];
	request_body[0] = '\0';
	snprintf(request_body, sizeof(request_body), "{\"model\":\"ep-20260318183410-gzgr5\",\"messages\":[");
	size_t current_len = strlen(request_body);
	
	// Add all messages to the request body
	// Buffer needs to be large enough for JSON-escaped content
	static char message[BUFFER_SIZE * 2 + 128];
	for (int i = 0; i < message_count; i++) {
		if (i > 0) {
			strncat(request_body, ",", sizeof(request_body) - current_len - 1);
			current_len++;
		}
		snprintf(message, sizeof(message), "{\"role\":\"%s\",\"content\":\"%s\"}", messages[i].role, messages[i].content);
		strncat(request_body, message, sizeof(request_body) - current_len - 1);
		current_len = strlen(request_body);
	}
	
	strncat(request_body, "],\"stream\":false}", sizeof(request_body) - current_len - 1);

	// Create HTTPS request to Doubao API
	request = NewHttpsRequest("https://ark.cn-beijing.volces.com/api/v3/chat/completions");
	if (request == NULL) {
		printf("error: cannot allocate request\n");
		return NULL;
	}

	HttpsRequestSetMethod(request, "POST");
	HttpsRequestSetTimeout(request, timeout_ms);
	HttpsRequestSetMaxRedirections(request, 0);
	// Create authorization header with Bearer prefix
	static char header[256];
	header[0] = '\0';
	snprintf(header, sizeof(header), "Bearer %s", api_key);
	HttpsRequestAddHeader(request, "Authorization", header);
	HttpsRequestAddHeader(request, "Content-Type", "application/json");

	// Send POST request with JSON body
	HttpsRequestSendBodyStr(request, request_body);
	BearHttpsRequest_represent(request);

	response = HttpsRequestFetch(request);
	if (response == NULL) {
		printf("error: fetch returned null response\n");
		HttpsRequestFree(request);
		return NULL;
	}

	if (HttpsResponseError(response)) {
		int ssl_error = 0;
		printf("error: %s (code=%d, ssl=%d)\n",
			HttpsResponseGetErrorMsg(response),
			HttpsResponseGetErrorCode(response),
			ssl_error);
		HttpsResponseFree(response);
		HttpsRequestFree(request);
		return NULL;
	}

	char *response_body = NULL;
	body = HttpsResponseReadBodyStr(response, &body_size);
	if (body != NULL && body_size > 0) {
		response_body = (char*)malloc(body_size+1);
		memcpy(response_body, body, body_size);
		response_body[body_size] = 0;
	} 

	HttpsResponseFree(response);
	HttpsRequestFree(request);
	return response_body;
}

const char* getMessageContent(const char* resp) {
	if(resp == NULL)
		return NULL;
	uint32_t len = strlen(resp);
	if(len == 0)
		return NULL;
	const char* p = strstr(resp, "\"content\":\"");
	if(p == NULL)
		return NULL;
	p += strlen("\"content\":\"");
	const char* e = p;
	int escape = 0;
	while(*e != '\0') {
		if(*e == '\\' && escape == 0) {
			escape = 1;
			e++;
			continue;
		}
		if(*e == '\"' && escape == 0) {
			break;
		}
		escape = 0;
		e++;
	}
	if(e == NULL || *e != '\"')
		return NULL;

	char* content = (char*)malloc(len+1);
	if(content == NULL)
		return NULL;
	// Process the content, converting escape sequences
	int i = 0;
	const char* src = p;
	while(src < e && i < len) {
		if(*src == '\\' && src + 1 < e) {
			src++;
			switch(*src) {
			case 'n':
				content[i++] = '\n';
				break;
			case 'r':
				content[i++] = '\r';
				break;
			case 't':
				content[i++] = '\t';
				break;
			case '"':
				content[i++] = '"';
				break;
			case '\\':
				content[i++] = '\\';
				break;
			default:
				content[i++] = '\\';
				content[i++] = *src;
				break;
			}
			src++;
		} else {
			content[i++] = *src++;
		}
	}
	content[i] = '\0';
	return content;
}

#define THINKING "thinking ... "

static Message messages[MAX_MESSAGES];
static int message_count = 0;

// JSON escape special characters in a string
// Returns the number of characters written to dest, or -1 if dest is too small
static int json_escape(const char* src, char* dest, size_t dest_size) {
	size_t j = 0;
	for (size_t i = 0; src[i] != '\0'; i++) {
		char c = src[i];
		const char* escape = NULL;
		
		switch (c) {
			case '"': escape = "\\\""; break;
			case '\\': escape = "\\\\"; break;
			case '\b': escape = "\\b"; break;
			case '\f': escape = "\\f"; break;
			case '\n': escape = "\\n"; break;
			case '\r': escape = "\\r"; break;
			case '\t': escape = "\\t"; break;
			default:
				// Control characters must be escaped as \u00XX
				if (c < 0x20) {
					if (j + 6 >= dest_size) return -1;
					snprintf(&dest[j], 7, "\\u%04x", (unsigned char)c);
					j += 6;
					continue;
				}
				// Regular character
				if (j + 1 >= dest_size) return -1;
				dest[j++] = c;
				continue;
		}
		
		// Write escaped sequence
		if (escape) {
			size_t escape_len = strlen(escape);
			if (j + escape_len >= dest_size) return -1;
			strcpy(&dest[j], escape);
			j += escape_len;
		}
	}
	
	if (j >= dest_size) return -1;
	dest[j] = '\0';
	return (int)j;
}

void add_context(bool user, const char* context) {
	if(context == NULL || context[0] == 0)
		return;
	
	// Add user message to conversation history
	if (message_count < MAX_MESSAGES) {
		if(user)
			messages[message_count].role = "user";
		else
			messages[message_count].role = "assistant";
		
		// JSON escape the context
		static char escaped[BUFFER_SIZE];
		int escaped_len = json_escape(context, escaped, BUFFER_SIZE-16);
		if (escaped_len < 0) {
			strcpy(messages[message_count].content, "ok");
			// Escaped string too long, truncate original
			/*uint32_t len = strlen(context);
			if(len >= BUFFER_SIZE)
				len = BUFFER_SIZE - 1;
			strncpy(messages[message_count].content, context, len);
			messages[message_count].content[len] = '\0';
			*/
		} else {
			// Copy escaped string (truncate if needed)
			if ((size_t)escaped_len >= BUFFER_SIZE) {
				memcpy(messages[message_count].content, escaped, BUFFER_SIZE - 1);
				messages[message_count].content[BUFFER_SIZE - 1] = '\0';
			} else {
				strcpy(messages[message_count].content, escaped);
			}
		}
		message_count++;
	}
}

static int  chat(const char* prompt) {
	add_context(true, prompt);
	printf("doubao: %s", THINKING);

	// Get response from Doubao
	char* content = NULL;
	char* resp = chat_with_context(messages, message_count);
	if(resp != NULL) {
		content = getMessageContent(resp);
		free(resp);
	}

	uint32_t len = strlen(THINKING);
	for(uint32_t i=0; i<len; i++) {
		write(1, "\b \b", 3);
	}

	// Print Doubao's response
	if(content != NULL) {
		printf("%s\n", content);
	}
	else
		printf("\n");

	if(content != NULL) {
		if (content[0] != 0)
			add_context(false, content);
		free(content);
	}
	return 0;
}

int main(int argc, char **argv) {
	setbuf(stdout, NULL);

	while(true) {
		printf(": ");
		static char prompt[BUFFER_SIZE+1];
		int i=0;
		for(i=0; i<BUFFER_SIZE; i++) {
			char c;
			int r = read(0, &c, 1);
			if(r <= 0) {
				return -1;
			}

			// Handle backspace
			if(c == 127 || c == '\b') {
				if(i > 0) {
					i--;
					// Clear the character in the buffer
					prompt[i] = 0;
					i--;
					// Send backspace, space, backspace to erase the character on screen
					write(1, "\b \b", 3);
				}
				continue;
			}

			prompt[i] = c;
			write(1, &c, 1);
			if(c == '\n' || c == '\r') {
				prompt[i] = 0;
				break;
			}
		}

		if(prompt[0] == 0)
			continue;

		if(strcmp(prompt, "exit") == 0) {
			return -1;
		}
		else if(strcmp(prompt, "clear") == 0) {
			message_count = 0;
			continue;
		}

		chat(prompt);
	}
	return 0;
}
