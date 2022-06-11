/*
*
* ------------------------------------------------------------------------------
*     Список доступных команд                  Запрос     Ответ
*
*   - Изменение рабочего каталога               CWD       250(530)
*   - Удалить файл                              DELE      250(530,501)
*   - Просмотр содержимого текущей директории   LIST      125,250,226(530)
*   - Создать директорию                        MKD       257(530,501)
*   - PING                                      NOOP      200(530)
*   - Ввод пароля                               PASS      230(510,530)
*   - Переход в активный режим                  PORT      200(530)
*   - Вывод рабочего каталога                   PWD       257(530)
*   - Закончить работу сервера                  QUIT      221
*   - Отправить файл                            RETR      150,226(530,550)
*   - Удалить директорию                        RMD       250(530,501)
*   - Переименовать файл                      RNFN/RNTO   350,250(530,450,451)
*   - Скачать файл                              STOR      150,226(530,451,452)
*   - Информация о системе                      SYST      257(530)
*   - Неизввестная команда                                550
*   - Авторизация пользователя                  USER      331(530)
* ------------------------------------------------------------------------------
*   Для работы в Total Commander необходимо:
*   1. "FTP" - "Соединится с FTP-сервором" [Ctrl + F];
*   2. "Добавить"
*   3. Ввести "Имя соединения", "Сервер [:порт]", "Учетную запись" и "пароль";
*   4. Вкладка "Расширенные" - "Тип сервера" - Определить новый тип;
*   5. Запускаем Сервер, подключаемся;
*   6. Определить тип сервера - Строка-шаблон:[--- TTT DD hh:mm:ss YYYY dS* n*]
*   6. Готово
* ------------------------------------------------------------------------------
*/

#ifndef _SERVER_H
#define _SERVER_H

#define DEBUG1

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <dir.h>
#include <dirent.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <unistd.h>
#include <pthread.h>

#include <winsock2.h>

#define SIZE_BUFFER     1024
#define SIZE_TEMP       1024
#define SIZE_BUFFER_CMD 6
#define SIZE_BUFFER_ARG 256
#define SIZE_BUFFER_PWD 512

#define PORT_SERVER 21
#define DEFAULT_PWD "C:\\temp"

/* Структура клиента */
typedef struct {
    /* Сокет для сервера (для передачи в поток)*/
    int server_socket;
    /* Сокет клиента для команд */
    int command_socket;
    /* Сокет клиента для передачи/приема */
    int transceiver_socket;
    /* Информация о клиенте */
    SOCKADDR_IN client_addrin;
    /* Имя пользователя */
    char *username;
    /* Пароль пользователя */
    char *password;

    /* Ссылка на сообщение/информацию, отправляемой пользователю */
    char *message;

    /* Размер отправляемых данных (для send_data) */
    int size_message;

    /* Текущаяя дериктория пользователя */
    char pwd[SIZE_BUFFER_PWD];

    /* Флаги */
    char flag_username_correct;
    char flag_password_correct;
    char flag_dev;

    char mode;
} Client;

/* Структура для парсинга*/
typedef struct {
    char cmd[SIZE_BUFFER_CMD];
    char arg[SIZE_BUFFER_ARG];
} st_cmd;


typedef struct {
    char cmd[SIZE_BUFFER_CMD];
    int id;
} ID_Command;


typedef enum {
    ID_CMD_USER, ID_CMD_PASS, ID_CMD_ACCT, ID_CMD_CWD , ID_CMD_QUIT,
    ID_CMD_PORT, ID_CMD_LIST, ID_CMD_SYST, ID_CMD_PWD , ID_CMD_RETR,
    ID_CMD_MKD , ID_CMD_RMD , ID_CMD_DELE, ID_CMD_STOR, ID_CMD_RNFR,
    ID_CMD_NOOP, ID_CMD_ABOR,
    ID_CMD_ENDOE
} en_id_cmd;

typedef enum {
    PASSIVE_MODE, ACTIVE_MODE
} mode;

void hello_server(Client *c);

st_cmd parse_command(Client *c);

Client* init_client(int server_socket);
int init_server_sock();
st_cmd recv_message(Client *c);
void send_data(Client *c);
void send_message(Client *c);
int server_processing(Client *c);
int server_start();

void cmd_cwd (Client *c, char *arg);
void cmd_dele(Client *c, char *arg);
void cmd_list(Client *c, char* arg);
void cmd_mkd (Client *c, char *arg);
void cmd_noop(Client *c);
void cmd_pass(Client *c, char *arg);
void cmd_port(Client *c, char *arg);
void cmd_pwd (Client *c);
void cmd_quit(Client *c);
void cmd_retr(Client *c, char *arg);
void cmd_rmd (Client *c, char *arg);
void cmd_rnfr(Client *c, char *arg);
void cmd_stor(Client *c, char *arg);
void cmd_syst(Client *c);
void cmd_unkn(Client *c);
void cmd_user(Client *c, char *arg);

void *thread_server_processing (void *server_socket);

#endif // _SERVER_H
