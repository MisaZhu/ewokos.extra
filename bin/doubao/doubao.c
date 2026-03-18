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

#define EWOK_HTTPS_PREVIEW_LIMIT -1
#define BUFFER_SIZE 1024
#define MAX_MESSAGES 100

// Message structure for conversation history
typedef struct {
    const char* role;
    char content[BUFFER_SIZE];
} Message;

static void print_preview(const char *body, int body_size) {
	int limit = EWOK_HTTPS_PREVIEW_LIMIT;
	if(limit < 0 || limit > body_size)
		limit = body_size;

	for (int i = 0; i < limit; ++i) {
		unsigned char ch = (unsigned char)body[i];
		if (ch == '\n' || ch == '\r' || ch == '\t' || (ch >= 32 && ch <= 126)) {
			putchar((int)ch);
		}
		else {
			putchar('.');
		}
	}
	if (limit > 0 && body[limit - 1] != '\n') {
		putchar('\n');
	}
}

static void print_usage(const char *prog) {
	printf("usage: %s <api_key> <prompt>\n", prog);
	printf("example: %s 863058a9-7b40-40e8-affc-f86b1496981e '你好，我是嵌入式开发者'\n", prog);
}

const char* chat(Message* messages, int message_count) {
	const char *api_key = "863058a9-7b40-40e8-affc-f86b1496981e";
	const char *body;
	TinyHttpsRequest *request;
	TinyHttpsResponse *response;
	int timeout_ms = 5000;
	int body_size;

	// Create request body in JSON format with conversation history
	char request_body[BUFFER_SIZE * 2]; // Increased size for longer messages
	snprintf(request_body, sizeof(request_body), "{\"model\":\"ep-20260318183410-gzgr5\",\"messages\":[");
	
	// Add all messages to the request body
	for (int i = 0; i < message_count; i++) {
		if (i > 0) {
			strcat(request_body, ",");
		}
		char message[BUFFER_SIZE];
		snprintf(message, sizeof(message), "{\"role\":\"%s\",\"content\":\"%s\"}", messages[i].role, messages[i].content);
		strcat(request_body, message);
	}
	
	strcat(request_body, "],\"stream\":false}");

	// Create HTTPS request to Doubao API
	request = NewHttpsRequest("https://ark.cn-beijing.volces.com/api/v3/chat/completions");
	if (request == NULL) {
		printf("error: cannot allocate request\n");
		return "";
	}

	HttpsRequestSetMethod(request, "POST");
	HttpsRequestSetTimeout(request, timeout_ms);
	HttpsRequestSetMaxRedirections(request, 0);
	// Create authorization header with Bearer prefix
	char header[256] = {0};
	snprintf(header, sizeof(header), "Bearer %s", api_key);
	HttpsRequestAddHeader(request, "Authorization", header);
	HttpsRequestAddHeader(request, "Content-Type", "application/json");

	// Send POST request with JSON body
	HttpsRequestSendBodyStr(request, request_body);

	response = HttpsRequestFetch(request);
	if (response == NULL) {
		printf("error: fetch returned null response\n");
		HttpsRequestFree(request);
		return "";
	}

	if (HttpsResponseError(response)) {
		int ssl_error = 0;
		printf("error: %s (code=%d, ssl=%d)\n",
			HttpsResponseGetErrorMsg(response),
			HttpsResponseGetErrorCode(response),
			ssl_error);
		HttpsResponseFree(response);
		HttpsRequestFree(request);
		return "";
	}

	static char response_body[BUFFER_SIZE] = {0};
	body = HttpsResponseReadBodyStr(response, &body_size);
	if (body != NULL && body_size > 0) {
		memcpy(response_body, body, body_size);
		response_body[body_size] = 0;
	} else {
		response_body[0] = 0;
	}

	HttpsResponseFree(response);
	HttpsRequestFree(request);
	return response_body;
}

const char* getMessageContent(const char* resp) {
	static char content[BUFFER_SIZE] = {0};
	const char* p = strstr(resp, "\"content\":\"");
	if(p == NULL)
		return "";
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
		return "";
	
	// Process the content, converting escape sequences
	int i = 0;
	const char* src = p;
	while(src < e && i < BUFFER_SIZE - 1) {
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

int main(int argc, char **argv) {
	setbuf(stdout, NULL);
	
	// Initialize conversation history
	Message messages[MAX_MESSAGES];
	int message_count = 0;

	while(true) {
		printf(": ");
		char prompt[BUFFER_SIZE+1];
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

		if(strcmp(prompt, "exit") == 0) {
			return -1;
		}

		// Add user message to conversation history
		if (message_count < MAX_MESSAGES) {
			messages[message_count].role = "user";
			strncpy(messages[message_count].content, prompt, BUFFER_SIZE - 1);
			messages[message_count].content[BUFFER_SIZE - 1] = '\0';
			message_count++;
		}

		// Get response from Doubao
		const char* resp = chat(messages, message_count);
		const char* content = getMessageContent(resp);

		// Print Doubao's response
		printf("doubao: ");
		printf("%s\n", content);

		// Add Doubao's response to conversation history
		if (message_count < MAX_MESSAGES) {
			messages[message_count].role = "assistant";
			strncpy(messages[message_count].content, content, BUFFER_SIZE - 1);
			messages[message_count].content[BUFFER_SIZE - 1] = '\0';
			message_count++;
		}
	}
	return 0;
}
