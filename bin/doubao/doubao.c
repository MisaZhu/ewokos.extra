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

#define API_KEY "863058a9-7b40-40e8-affc-f86b1496981e"
#define MODEL_ID "ep-20260318183410-gzgr5"

// Message structure for conversation history
typedef struct {
    const char* role;
    char content[BUFFER_SIZE];
} Message;
#define MAX_MESSAGES 128

static Message messages[MAX_MESSAGES];
static int message_count = 0;
static int message_start = 0;  // Index of the oldest message (for circular buffer)

// Get the actual index in the circular buffer
// idx: 0 = oldest message, message_count-1 = newest message
static int get_message_index(int idx) {
	return (message_start + idx) % MAX_MESSAGES;
}

static char* chat_with_context(Message* messages, int message_count) {
	const char *body;
	TinyHttpsRequest *request;
	TinyHttpsResponse *response;
	int timeout_ms = 5000;
	int body_size;

	// Create request body in JSON format with conversation history
	// Use static allocation to avoid stack overflow
	static char request_body[BUFFER_SIZE * 4];
	request_body[0] = '\0';
	snprintf(request_body, sizeof(request_body), "{\"model\":\"%s\",\"messages\":[", MODEL_ID);
	size_t current_len = strlen(request_body);
	
	// Add all messages to the request body in chronological order (oldest first)
	// Buffer needs to be large enough for JSON-escaped content
	static char message[BUFFER_SIZE * 2 + 128];
	for (int i = 0; i < message_count; i++) {
		if (i > 0) {
			strncat(request_body, ",", sizeof(request_body) - current_len - 1);
			current_len++;
		}
		// Get the actual index in the circular buffer
		int idx = get_message_index(i);
		snprintf(message, sizeof(message), "{\"role\":\"%s\",\"content\":\"%s\"}", messages[idx].role, messages[idx].content);
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
	snprintf(header, sizeof(header), "Bearer %s", API_KEY);
	HttpsRequestAddHeader(request, "Authorization", header);
	HttpsRequestAddHeader(request, "Content-Type", "application/json");

	// Send POST request with JSON body
	HttpsRequestSendBodyStr(request, request_body);
	//BearHttpsRequest_represent(request);

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

char* getMessageContent(const char* resp) {
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
	while(src < e && i < (int)len) {
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

static bool think_removed = false;
static void remove_thinking(void) {
	uint32_t len = strlen(THINKING);
	for(uint32_t i=0; i<len; i++) {
		write(1, "\b \b", 3);
	}
}

// Example callback for stream reading
// This callback receives data chunks as they arrive from the server
// user_data is a pointer to a buffer that accumulates all content
static int stream_callback(const char* data, int size, void* user_data) {
	if(!think_removed) {
		remove_thinking();
		think_removed = true;
	}

	// Print the received chunk
	// Note: data may not be null-terminated, so use size
	char* full_content = (char*)user_data;
	char* content = getMessageContent(data);
	if(content != NULL) {
		printf("\033[1m%s\033[0m", content);
		// Append content to full_content with a space
		if(full_content != NULL) {
			if((strlen(full_content) + strlen(content) + 1) < BUFFER_SIZE) {
				strcat(full_content, content);
				strcat(full_content, " ");
			}
		}
		free(content);
	}
	// Return 0 to continue reading, non-zero to abort
	return 0;
}

// Example: chat with stream mode (for SSE - Server-Sent Events)
// This function demonstrates how to use the new stream API
static char* chat_with_stream(Message* messages, int message_count) {
	TinyHttpsRequest *request;
	TinyHttpsResponse *response;
	int timeout_ms = 60000; // Longer timeout for streaming

	// Create request body with stream=true
	static char request_body[BUFFER_SIZE * 4];
	request_body[0] = '\0';
	snprintf(request_body, sizeof(request_body), "{\"model\":\"%s\",\"messages\":[", MODEL_ID);
	size_t current_len = strlen(request_body);
	
	static char message[BUFFER_SIZE * 2 + 128];
	for (int i = 0; i < message_count; i++) {
		if (i > 0) {
			strncat(request_body, ",", sizeof(request_body) - current_len - 1);
			current_len++;
		}
		int idx = get_message_index(i);
		snprintf(message, sizeof(message), "{\"role\":\"%s\",\"content\":\"%s\"}", messages[idx].role, messages[idx].content);
		strncat(request_body, message, sizeof(request_body) - current_len - 1);
		current_len = strlen(request_body);
	}
	
	strncat(request_body, "],\"stream\":true}", sizeof(request_body) - current_len - 1);

	request = NewHttpsRequest("https://ark.cn-beijing.volces.com/api/v3/chat/completions");
	if (request == NULL) {
		printf("error: cannot allocate request\n");
		return NULL;
	}

	HttpsRequestSetMethod(request, "POST");
	HttpsRequestSetTimeout(request, timeout_ms);
	HttpsRequestSetMaxRedirections(request, 0);
	
	static char header[256];
	header[0] = '\0';
	snprintf(header, sizeof(header), "Bearer %s", API_KEY);
	HttpsRequestAddHeader(request, "Authorization", header);
	HttpsRequestAddHeader(request, "Content-Type", "application/json");

	HttpsRequestSendBodyStr(request, request_body);

	response = HttpsRequestFetch(request);
	if (response == NULL) {
		printf("error: fetch returned null response\n");
		HttpsRequestFree(request);
		return NULL;
	}

	if (HttpsResponseError(response)) {
		printf("error: %s\n", HttpsResponseGetErrorMsg(response));
		HttpsResponseFree(response);
		HttpsRequestFree(request);
		return NULL;
	}

	// Allocate a larger buffer to accumulate all stream content
	char* ret = (char*)malloc(BUFFER_SIZE+1);
	if(ret != NULL) {
		ret[0] = '\0';  // Initialize empty string
	}
	
	// Read response in streaming mode
	HttpsResponseReadBodyStream(response, stream_callback, ret);

	HttpsResponseFree(response);
	HttpsRequestFree(request);
	return ret;
}

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
	
	int idx;
	if (message_count < MAX_MESSAGES) {
		// Buffer not full yet, add at the end
		idx = message_count;
		message_count++;
	} else {
		// Buffer full, overwrite the oldest message
		idx = message_start;
		message_start = (message_start + 1) % MAX_MESSAGES;
	}
	
	if(user)
		messages[idx].role = "user";
	else
		messages[idx].role = "assistant";
	
	// JSON escape the context
	static char escaped[BUFFER_SIZE];
	int escaped_len = json_escape(context, escaped, BUFFER_SIZE-16);
	if (escaped_len < 0) {
		strcpy(messages[idx].content, "responsed");
	} else {
		// Copy escaped string (truncate if needed)
		if ((size_t)escaped_len >= BUFFER_SIZE) {
			memcpy(messages[idx].content, escaped, BUFFER_SIZE - 1);
			messages[idx].content[BUFFER_SIZE - 1] = '\0';
		} else {
			strcpy(messages[idx].content, escaped);
		}
	}
}


static bool chat(const char* prompt, bool stream) {
	printf("\033[1m: %s\033[0m", THINKING);
	think_removed = false;

	add_context(true, prompt);

	// Get response from Doubao
	char* content = NULL;
	if(stream) {
		// Use stream mode - output is printed in real-time via callback
		content = chat_with_stream(messages, message_count);
	}
	else {
		char* resp = chat_with_context(messages, message_count);
		remove_thinking();
		if(resp != NULL) {
			content = getMessageContent(resp);
			free(resp);
			printf("\033[1m%s\033[0m", content);
		}
	}

	if(content == NULL)
		return false;

	if (content[0] != 0)
		add_context(false, content);
	free(content);
	return true;  // Stream mode doesn't return accumulated content
}

static int read_prompt(char* prompt, int32_t size) {
	int i=0;
	while(i < size) {
		char c;
		int r = read(0, &c, 1);
		if(r <= 0) {
			return -1;
		}

		// Handle backspace
		if(c == 127 || c == '\b') {
			if(i > 0) {
				write(1, "\b \b", 3); // Clear the character in the buffer
				i--;
				prompt[i] = 0;
			}
			continue;
		}

		prompt[i] = c;
		write(1, &c, 1);
		if(c == '\n' || c == '\r') {
			prompt[i] = 0;
			break;
		}
		i++;
	}
	return i;
}

int main(int argc, char **argv) {
	setbuf(stdout, NULL);

	printf("==( doubao chat, 'exit' to quit )==\n");
	while(true) {
		printf("$ ");
		static char prompt[BUFFER_SIZE+1] = {0};
		int r = read_prompt(prompt, BUFFER_SIZE);
		if(r < 0)
			return -1;

		if(prompt[0] == 0)
			continue;

		if(strcmp(prompt, "exit") == 0) {
			return -1;
		}
		else if(strcmp(prompt, "clear") == 0) {
			message_count = 0;
			message_start = 0;
			continue;
		}

		// Call chat - in stream mode, output is printed in real-time
		chat(prompt, true);
		printf("\n\n");
	}
	return 0;
}
