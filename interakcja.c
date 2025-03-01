#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // For close()
#include <winsock2.h>  // Windows sockets
#include <ws2tcpip.h>  // Extra functions for network communication

#pragma comment(lib, "ws2_32.lib")  // Link with Winsock library


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define MODEL_NAME "llama-3.2-1b-instruct"
#define BUFFER_SIZE 8192
#define MAX_HISTORY 100

// Function to escape JSON special characters
void escapeJson(const char *input, char *output) {
    int j = 0;
    for (int i = 0; input[i] != '\0'; i++) {
        switch (input[i]) {
            case '\"': output[j++] = '\\'; output[j++] = '\"'; break;
            case '\\': output[j++] = '\\'; output[j++] = '\\'; break;
            case '\n': output[j++] = '\\'; output[j++] = 'n'; break;
            case '\t': output[j++] = '\\'; output[j++] = 't'; break;
            default: output[j++] = input[i]; break;
        }
    }
    output[j] = '\0';
}

char *strdup(const char *s) {
    size_t len = strlen(s) + 1;  // +1 for null terminator
    char *copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

// Function to extract chatbot's message from JSON response
void extractChatbotResponse(const char *jsonResponse, char *output) {
    // Locate the "message" block
    char *msgStart = strstr(jsonResponse, "\"message\":");
    if (msgStart) {
        // Within the message, find the "content": " key (note the space after colon)
        char *contentKey = strstr(msgStart, "\"content\": \"");
        if (contentKey) {
            contentKey += strlen("\"content\": \""); // Move pointer past the key
            // Find the closing quote for the content value
            char *end = strchr(contentKey, '\"');
            if (end) {
                size_t len = end - contentKey;
                strncpy(output, contentKey, len);
                output[len] = '\0';
                // Convert escaped newlines ("\n") to actual newlines
                for (char *p = output; *p; p++) {
                    if (p[0] == '\\' && p[1] == 'n') {
                        *p = '\n';
                        memmove(p + 1, p + 2, strlen(p + 2) + 1);
                    }
                }
                return;
            }
        }
    }
    strcpy(output, "Error: Could not extract response.");
}


// Function to send an HTTP request via a socket and receive the response
char *sendRequest(const char *jsonPayload) {
    int sock;
    struct sockaddr_in server;
    char request[BUFFER_SIZE];
    static char response[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Could not create socket\n");
        return NULL;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    // Use inet_addr() instead of InetPton()
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    if (server.sin_addr.s_addr == INADDR_NONE) {
        printf("Invalid address\n");
        closesocket(sock);
        return NULL;
    }

    // Connect to LM Studio API server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connection failed\n");
        closesocket(sock);
        return NULL;
    }

    // Build HTTP request
    snprintf(request, sizeof(request),
             "POST /v1/chat/completions HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %lu\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             SERVER_IP, SERVER_PORT, strlen(jsonPayload), jsonPayload);

    // Send request
    if (send(sock, request, strlen(request), 0) < 0) {
        printf("Send failed\n");
        closesocket(sock);
        return NULL;
    }

    // Receive response
    int bytesRead = recv(sock, response, sizeof(response) - 1, 0);
    if (bytesRead < 0) {
        printf("Receive failed\n");
        closesocket(sock);
        return NULL;
    }
    response[bytesRead] = '\0'; // Null-terminate response

    closesocket(sock);
    return response;
}


int main() {

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed!\n");
        return 1;
    }

    char userInput[BUFFER_SIZE], escapedInput[BUFFER_SIZE], chatbotReply[BUFFER_SIZE];
    char *messages[MAX_HISTORY];
    int messageCount = 0;

    // Add system message
    messages[messageCount++] = strdup("{\"role\": \"system\", \"content\": \"You are a helpful assistant.\"}");

    printf("Chatbot is ready! Type 'exit' to quit.\n");

    while (1) {
        printf("You: ");
        if (!fgets(userInput, BUFFER_SIZE, stdin)) break; // Read user input
        userInput[strcspn(userInput, "\n")] = 0; // Remove newline

        if (strcmp(userInput, "exit") == 0) {
            printf("Chatbot session ended.\n");
            break;
        }

        escapeJson(userInput, escapedInput);
        char userMessage[BUFFER_SIZE];
        snprintf(userMessage, BUFFER_SIZE, "{\"role\": \"user\", \"content\": \"%s\"}", escapedInput);
        messages[messageCount++] = strdup(userMessage);

        // Build JSON payload
        char jsonPayload[BUFFER_SIZE] = "{ \"model\": \"" MODEL_NAME "\", \"messages\": [";
        for (int i = 0; i < messageCount; i++) {
            strcat(jsonPayload, messages[i]);
            if (i < messageCount - 1) strcat(jsonPayload, ",");
        }
        strcat(jsonPayload, "], \"temperature\": 0.7 }");

        // Send request and process response
        char *jsonResponse = sendRequest(jsonPayload);
        if (jsonResponse) {
            extractChatbotResponse(jsonResponse, chatbotReply);
            printf("Chatbot: %s\n", chatbotReply);

            // Add chatbot response to history
            char chatbotMessage[BUFFER_SIZE];
            snprintf(chatbotMessage, BUFFER_SIZE, "{\"role\": \"assistant\", \"content\": \"%s\"}", chatbotReply);
            messages[messageCount++] = strdup(chatbotMessage);
        } else {
            printf("Error: No response from chatbot.\n");
        }
    }

    WSACleanup(); // Cleanup Winsock on Windows

    // Free allocated memory
    for (int i = 0; i < messageCount; i++) {
        free(messages[i]);
    }

    return 0;
}
