#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // For close()
#include <arpa/inet.h>  // For socket(), connect(), send(), recv()

#ifdef _WIN32
    #include <winsock2.h>  // Windows sockets
    #include <ws2tcpip.h>  // Extra functions for network communication
    #pragma comment(lib, "ws2_32.lib")  // Link with Winsock library
#else
    #include <arpa/inet.h>  // For socket(), connect(), send(), recv() on Linux/Mac
    #include <sys/socket.h>
    #include <unistd.h>      // For close()
#endif

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

// Function to extract chatbot's message from JSON response
void extractChatbotResponse(const char *jsonResponse, char *output) {
    char *start = strstr(jsonResponse, "\"content\": \"");
    if (start) {
        start += 11; // Move past "content": "
        char *end = strchr(start, '\"'); // Find closing quote
        if (end) {
            strncpy(output, start, end - start);
            output[end - start] = '\0';
            // Convert escaped newlines
            for (char *p = output; *p; p++) {
                if (p[0] == '\\' && p[1] == 'n') {
                    *p = '\n'; memmove(p + 1, p + 2, strlen(p + 2) + 1);
                }
            }
        }
    } else {
        strcpy(output, "Error: Could not extract response.");
    }
}

// Function to send an HTTP request via a socket and receive the response
char *sendRequest(const char *jsonPayload) {
    int sock;
    struct sockaddr_in server;
    char request[BUFFER_SIZE];
    static char response[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return NULL;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock);
        return NULL;
    }

    // Connect to LM Studio API server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connection failed");
        close(sock);
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
        perror("Send failed");
        close(sock);
        return NULL;
    }

    // Receive response
    int bytesRead = recv(sock, response, sizeof(response) - 1, 0);
    if (bytesRead < 0) {
        perror("Receive failed");
        close(sock);
        return NULL;
    }
    response[bytesRead] = '\0'; // Null-terminate response

    close(sock);
    return response;
}

int main() {

    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed!\n");
        return 1;
    }
    #endif

      // Your networking code (socket creation, connection, etc.)

    #ifdef _WIN32
        WSACleanup(); // Cleanup Winsock on Windows
    #endif

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

    // Free allocated memory
    for (int i = 0; i < messageCount; i++) {
        free(messages[i]);
    }

    return 0;
}
