#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define MAX_DIGITS              16
#define COMMAND_MAX_LEN         32
#define BOOK_INFO_MAX_LEN       128
#define SID_MAX_LEN             256
#define SERVER_IP               "34.254.242.81"
#define SERVER_PORT             8080
#define SPACE_CHAR              32      // " "
#define SEMICOLON_CHAR          59      // ";"
#define OPEN_SQ_BRACKET_CHAR    91      // "["
#define OPEN_BRACKET_CHAR       123     // "{"
#define ERR_BAD_REQUEST         "400"
#define ERR_UNAUTHORIZED        "401"
#define ERR_FORBIDDEN           "403"
#define ERR_NOT_FOUND           "404"

int valid_number(char *number) {
    for (int i = 0; i < strlen(number); i++) {
        if (!isdigit(number[i])) {
            return 0;
        }
    }
    return 1;
}

void register_or_login(char *action, int *sockfd, char **message, char **response,
                    char **response_code, char ***cookies, int *cookies_count) {
    if (*cookies_count) {
        printf("ERROR - Already logged in!\n\n");
        return;
    }

    char *username = calloc(64, sizeof(char *));
    printf("username=");
    fgets(username, 64, stdin);
    username[strcspn(username, "\n")] = 0;

    char *password = calloc(64, sizeof(char *));
    printf("password=");
    fgets(password, 64, stdin);
    password[strcspn(password, "\n")] = 0;

    if (strchr(username, SPACE_CHAR) || strchr(password, SPACE_CHAR)) {
        printf("ERROR - Username or password contains spaces!\n\n");
        free(username);
        free(password);
        return;
    }

    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);
    json_object_set_string(json_object, "username", username);
    json_object_set_string(json_object, "password", password);
    char *json_payload = json_serialize_to_string_pretty(json_value);

    if (strcmp(action, "register") == 0) {
        *message = compute_post_request(SERVER_IP, "/api/v1/tema/auth/register", "application/json", json_payload, NULL, NULL, 0);
    }
    if (strcmp(action, "login") == 0) {
        *message = compute_post_request(SERVER_IP, "/api/v1/tema/auth/login", "application/json", json_payload, NULL, NULL, 0);
    }

    json_free_serialized_string(json_payload);
    json_value_free(json_value);

    *sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(*sockfd, *message);
    *response = receive_from_server(*sockfd);

    *response_code = calloc(3, sizeof(char *));
    memccpy(*response_code, strchr(*response, SPACE_CHAR) + 1, SPACE_CHAR, 3);

    if (strcmp(action, "register") == 0) {
        if (strncmp(*response_code, ERR_BAD_REQUEST, 3) == 0) {
            printf("ERROR %s - Username %s already taken!\n\n", *response_code, username);
        } else {
            printf("Username %s successfully registered!\n\n", username);
        }
    }
    if (strcmp(action, "login") == 0) {
        if (strncmp(*response_code, ERR_BAD_REQUEST, 3) == 0) {
            printf("ERROR %s - Wrong credentials for username %s!\n\n", *response_code, username);
        } else {
            *cookies = calloc(1, sizeof(char *));
            (*cookies)[0] = calloc(LINELEN, sizeof(char));
            memccpy((*cookies)[0], strstr(*response, "Set-Cookie: ") + strlen("Set-Cookie: "), SEMICOLON_CHAR, SID_MAX_LEN);
            (*cookies)[0][strcspn((*cookies)[0], ";")] = 0;
            (*cookies_count)++;
            printf("Login successful for username %s!\n\n", username);
        }
    }

    free(username);
    free(password);
}

void enter_library(int *sockfd, char **message, char **response, char **response_code,
                    char **token, char **cookies, int cookies_count) {
    if (*token) {
        printf("ERROR - You already have access to the library!\n\n");
        return;
    }

    *message = compute_get_request(SERVER_IP, "/api/v1/tema/library/access", NULL, NULL, cookies, cookies_count);
    *sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(*sockfd, *message);
    *response = receive_from_server(*sockfd);

    *response_code = calloc(3, sizeof(char *));
    memccpy(*response_code, strchr(*response, SPACE_CHAR) + 1, SPACE_CHAR, 3);

    if (strncmp(*response_code, ERR_UNAUTHORIZED, 3) == 0) {
        printf("ERROR %s - You are not logged in!\n\n", *response_code);
    } else {
        JSON_Value *json_value = json_parse_string(strchr(*response, OPEN_BRACKET_CHAR));
        JSON_Object *json_object = json_value_get_object(json_value);
        int token_size = json_object_get_string_len(json_object, "token");
        *token = calloc(token_size, sizeof(char *));
        memcpy(*token, json_object_get_string(json_object, "token"), token_size);

        json_value_free(json_value);
        printf("Library access granted!\n\n");
    }
}

void get_books(int *sockfd, char **message, char **response, char **response_code, char *token) {
    *message = compute_get_request(SERVER_IP, "/api/v1/tema/library/books", NULL, token, NULL, 0);
    *sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(*sockfd, *message);
    *response = receive_from_server(*sockfd);

    *response_code = calloc(3, sizeof(char *));
    memccpy(*response_code, strchr(*response, SPACE_CHAR) + 1, SPACE_CHAR, 3);

    if (strncmp(*response_code, ERR_FORBIDDEN, 3) == 0) {
        printf("ERROR %s - You don't have access to the library!\n\n", *response_code);
    } else {
        if (strcmp(strchr(*response, OPEN_SQ_BRACKET_CHAR), "[]") == 0) {
            printf("Your book list is empty!\n\n");
        } else {
            JSON_Value *json_value = json_parse_string(strchr(*response, OPEN_SQ_BRACKET_CHAR));
            JSON_Array *books = json_value_get_array(json_value);

            for (int i = 0; i < json_array_get_count(books); i++) {
                JSON_Object *book = json_array_get_object(books, i);
                printf("\nid=%d\n", (int)json_object_get_number(book, "id"));
                printf("title=%s\n", json_object_get_string(book, "title"));
            }
            printf("\n");

            json_value_free(json_value);
        }
    }
}

void get_or_delete_book(char *action, int *sockfd, char **message, char **response,
                        char **response_code, char *token) {
    char *id = calloc(MAX_DIGITS, sizeof(char *));
    printf("id=");
    fgets(id, MAX_DIGITS, stdin);
    id[strcspn(id, "\n")] = 0;

    if (!valid_number(id)) {
        printf("ERROR - Please enter a valid number!\n\n");
        free(id);
        return;
    }

    char *url = calloc(LINELEN, sizeof(char *));
    sprintf(url, "/api/v1/tema/library/books/%s", id);
    if (strcmp(action, "get") == 0) {
        *message = compute_get_request(SERVER_IP, url, NULL, token, NULL, 0);
    }
    if (strcmp(action, "delete") == 0) {
        *message = compute_delete_request(SERVER_IP, url, NULL, token, NULL, 0);
    }
    *sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(*sockfd, *message);
    *response = receive_from_server(*sockfd);

    *response_code = calloc(3, sizeof(char *));
    memccpy(*response_code, strchr(*response, SPACE_CHAR) + 1, SPACE_CHAR, 3);

    if (strncmp(*response_code, ERR_FORBIDDEN, 3) == 0) {
        printf("ERROR %s - You don't have access to the library!\n\n", *response_code);
    } else if (strncmp(*response_code, ERR_NOT_FOUND, 3) == 0) {
        printf("ERROR %s - The book with id %s was not found!\n\n", *response_code, id);
    } else {
        if (strcmp(action, "get") == 0) {
            JSON_Value *json_value = json_parse_string(strchr(*response, OPEN_BRACKET_CHAR));
            JSON_Object *book = json_value_get_object(json_value);

            printf("\nid=%d\n", (int)json_object_get_number(book, "id"));
            printf("title=%s\n", json_object_get_string(book, "title"));
            printf("author=%s\n", json_object_get_string(book, "author"));
            printf("publisher=%s\n", json_object_get_string(book, "publisher"));
            printf("genre=%s\n", json_object_get_string(book, "genre"));
            printf("page_count=%d\n\n", (int)json_object_get_number(book, "page_count"));

            json_value_free(json_value);
        }
        if (strcmp(action, "delete") == 0) {
            printf("The book with id %s was deleted successfully!\n\n", id);
        }
    }

    free(url);
    free(id);
}

void add_book(int *sockfd, char **message, char **response, char **response_code, char *token) {
    char *title = calloc(BOOK_INFO_MAX_LEN, sizeof(char *));
    printf("title=");
    fgets(title, BOOK_INFO_MAX_LEN, stdin);
    title[strcspn(title, "\n")] = 0;

    char *author = calloc(BOOK_INFO_MAX_LEN, sizeof(char *));
    printf("author=");
    fgets(author, BOOK_INFO_MAX_LEN, stdin);
    author[strcspn(author, "\n")] = 0;

    char *publisher = calloc(BOOK_INFO_MAX_LEN, sizeof(char *));
    printf("publisher=");
    fgets(publisher, BOOK_INFO_MAX_LEN, stdin);
    publisher[strcspn(publisher, "\n")] = 0;

    char *genre = calloc(BOOK_INFO_MAX_LEN, sizeof(char *));
    printf("genre=");
    fgets(genre, BOOK_INFO_MAX_LEN, stdin);
    genre[strcspn(genre, "\n")] = 0;

    char *page_count = calloc(MAX_DIGITS, sizeof(char *));
    printf("page_count=");
    fgets(page_count, MAX_DIGITS, stdin);
    page_count[strcspn(page_count, "\n")] = 0;

    if (!strlen(title) || !strlen(author) || !strlen(publisher) || !strlen(genre) || !strlen(page_count) || !valid_number(page_count)) {
        if (!valid_number(page_count)) {
            printf("ERROR - Please enter a valid number!\n\n");
        } else {
            printf("ERROR - All book fields are mandatory!\n\n");
        }
        free(title);
        free(author);
        free(publisher);
        free(genre);
        free(page_count);
        return;
    }

    JSON_Value *json_value = json_value_init_object();
    JSON_Object *json_object = json_value_get_object(json_value);

    json_object_set_string(json_object, "title", title);
    json_object_set_string(json_object, "author", author);
    json_object_set_string(json_object, "publisher", publisher);
    json_object_set_string(json_object, "genre", genre);
    json_object_set_number(json_object, "page_count", atoi(page_count));
    char *json_payload = json_serialize_to_string_pretty(json_value);

    *message = compute_post_request(SERVER_IP, "/api/v1/tema/library/books", "application/json", json_payload, token, NULL, 0);
    *sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(*sockfd, *message);
    *response = receive_from_server(*sockfd);
    json_free_serialized_string(json_payload);
    json_value_free(json_value);

    *response_code = calloc(3, sizeof(char *));
    memccpy(*response_code, strchr(*response, SPACE_CHAR) + 1, SPACE_CHAR, 3);

    if (strncmp(*response_code, ERR_FORBIDDEN, 3) == 0) {
        printf("ERROR %s - You don't have access to the library!\n\n", *response_code);
    } else {
        printf("Book successfully added!\n\n");
    }

    free(title);
    free(author);
    free(publisher);
    free(genre);
    free(page_count);
}

void logout(int *sockfd, char **message, char **response, char **response_code,
            char **token, char ***cookies, int *cookies_count) {
    *message = compute_get_request(SERVER_IP, "/api/v1/tema/auth/logout", NULL, NULL, *cookies, *cookies_count);
    *sockfd = open_connection(SERVER_IP, SERVER_PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(*sockfd, *message);
    *response = receive_from_server(*sockfd);

    *response_code = calloc(3, sizeof(char *));
    memccpy(*response_code, strchr(*response, SPACE_CHAR) + 1, SPACE_CHAR, 3);

    if (strncmp(*response_code, ERR_BAD_REQUEST, 3) == 0) {
        printf("ERROR %s - You are not logged in!\n\n", *response_code);
    } else {
        free(*token);
        *token = NULL;
        free((*cookies)[0]);
        free(*cookies);
        *cookies = NULL;
        *cookies_count = 0;
        printf("Logout successful!\n\n");
    }
}

int main(int argc, char *argv[]) {
    char **cookies = NULL;
    int cookies_count = 0;
    char *token = NULL;

    char *command = calloc(COMMAND_MAX_LEN, sizeof(char *));
    while (strcmp(fgets(command, COMMAND_MAX_LEN, stdin), "exit\n")) {
        int command_executed = 0;
        int sockfd = 0;
        char *message = NULL;
        char *response = NULL;
        char *response_code = NULL;

        if (strcmp(command, "register\n") == 0) {
            register_or_login("register", &sockfd, &message, &response, &response_code, &cookies, &cookies_count);
            command_executed++;
        }

        if (strcmp(command, "login\n") == 0) {
            register_or_login("login", &sockfd, &message, &response, &response_code, &cookies, &cookies_count);
            command_executed++;
        }

        if (strcmp(command, "enter_library\n") == 0) {
            enter_library(&sockfd, &message, &response, &response_code, &token, cookies, cookies_count);
            command_executed++;
        }

        if (strcmp(command, "get_books\n") == 0) {
            get_books(&sockfd, &message, &response, &response_code, token);
            command_executed++;
        }

        if (strcmp(command, "get_book\n") == 0) {
            get_or_delete_book("get", &sockfd, &message, &response, &response_code, token);
            command_executed++;
        }

        if (strcmp(command, "add_book\n") == 0) {
            add_book(&sockfd, &message, &response, &response_code, token);
            command_executed++;
        }

        if (strcmp(command, "delete_book\n") == 0) {
            get_or_delete_book("delete", &sockfd, &message, &response, &response_code, token);
            command_executed++;
        }

        if (strcmp(command, "logout\n") == 0) {
            logout(&sockfd, &message, &response, &response_code, &token, &cookies, &cookies_count);
            command_executed++;
        }

        if (!command_executed) {
            printf("ERROR - Invalid command! List of available commands:\n");
            printf("- register\n- login\n- enter_library\n- get_books\n- get_book\n- add_book\n- delete_book\n- logout\n- exit\n\n");
        } else if (sockfd) {
            free(message);
            free(response);
            free(response_code);
            close(sockfd);
        }
    }

    free(command);
    return 0;
}
