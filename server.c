#include "server.h"

ID_Command id_command[] = {
    { "USER", ID_CMD_USER },
    { "PASS", ID_CMD_PASS },
    { "ACCT", ID_CMD_ACCT },
    { "CWD" , ID_CMD_CWD  },
    { "QUIT", ID_CMD_QUIT },
    { "PORT", ID_CMD_PORT },
    { "LIST", ID_CMD_LIST },
    { "SYST", ID_CMD_SYST },
    { "PWD" , ID_CMD_PWD  },
    { "RETR", ID_CMD_RETR },
    { "MKD" , ID_CMD_MKD  },
    { "RMD" , ID_CMD_RMD  },
    { "DELE", ID_CMD_DELE },
    { "STOR", ID_CMD_STOR },
    { "RNFR", ID_CMD_RNFR },
    { "NOOP", ID_CMD_NOOP },
    { "ABOR", ID_CMD_ABOR }
    };

/*  Запуск сервера */
int server_start() {
    setlocale(LC_ALL, "Russian");
    printf("[info] Start FTP server\n");

    int server_socket = init_server_sock();
    if (server_socket == -1) {
        return -1;
    }

#if 0
    /*  ждем подключения клиентов */
    while (1) {
        pthread_t thread;
        pthread_create(&thread, NULL, thread_server_listening, server_socket);
    }
#endif // 0

    while (1) {
        Client *c = init_client(server_socket);
        c->server_socket = server_socket;
        hello_server(c);

        pthread_t thread;
        pthread_create(&thread, NULL, thread_server_processing, c);
    }

    closesocket(server_socket);
    return 0;
}

/*  Функции для запуска многопоточности */
void *thread_server_processing(void *c) {
    int event = 0;
    while (!event) {
        event = server_processing(c);
    }
    return 0;
}

/*  Инициализация сокета */
int init_server_sock() {
    int server_socket;
    WSADATA WSAData;
    /*  Инициализация библиотеки Ws2_32.dll. */
    if(WSAStartup(MAKEWORD(2,2), &WSAData) != 0) {
        printf("[info] Initialization WSAStartup - error\n");
        return -1;
    }
    printf("[info] Initialization WSAStartup - success\n");

    /*  Создаем сокет */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket server - invalid\n");
        return -1;
    }
    printf("[info] Socket server - success\n");

    /*  Настройка */
    SOCKADDR_IN server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY; //inet_addr("127.0.0.1");
    server_address.sin_port = htons(PORT_SERVER);

    /*  Привязка настройки и сокета */
    if (bind(server_socket,(SOCKADDR *)&server_address, sizeof server_address) == SOCKET_ERROR) {
        printf("Bind server socket - failed\n");
        return -1;
    }
    printf("[info] Bind server socket - success\n");

    return server_socket;
}

/*  Инициализвация клиента */
Client* init_client(int server_socket){
    printf("[info] Waiting for incoming connections...\n");
    listen(server_socket,5);
    Client *c = (Client*)malloc(sizeof (Client));
    SOCKADDR_IN client_address;
    int size_sockaddr_client = sizeof(client_address);
    int client_socket = accept(server_socket, &client_address, &size_sockaddr_client);
    if (client_socket == INVALID_SOCKET) {
        printf("[info] Socket client - INVALID SOCKET\n");
        exit(-1);
    } else {
        printf("[info] Socket client - success\n");
    }
   // char ip_temp[24] = inet_ntoa(client_address.sin_addr);
    printf("[info] Client connect [%s:%d]\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
    c->client_addrin = client_address;
    c->command_socket = client_socket;
    c->flag_password_correct = 0;
    c->flag_username_correct = 0;
    c->flag_dev = 1;
    memset(c->pwd, 0, sizeof(c->pwd));
    sprintf(c->pwd, "%s", DEFAULT_PWD);
    return c;
}

/*  Отправка сообщения */
void send_message(Client *c)  {
    printf("[%s:%d] -> : %s", inet_ntoa(c->client_addrin.sin_addr), ntohs(c->client_addrin.sin_port), c->message);
    send(c->command_socket, c->message, strlen(c->message), 0);
}

/*  Прием сообщения */
st_cmd recv_message(Client *c) {
    st_cmd scmd;
    char message_buffer[SIZE_BUFFER];
    memset(message_buffer,0 ,sizeof message_buffer);
    if (recv(c->command_socket, message_buffer, SIZE_BUFFER, 0) <= 0) {
        sprintf(scmd.cmd, "%s", "VOID");
        return scmd;
    }
    printf("[%s:%d] <- : %s", inet_ntoa(c->client_addrin.sin_addr), ntohs(c->client_addrin.sin_port), message_buffer);
    c->message = message_buffer;
    scmd = parse_command(c);
    return scmd;
}

/*  Отправка информации */
void send_data(Client *c)  {
#ifdef DEBUG
    printf("-|: %s", c->message);
#endif // DEBUG
    send(c->transceiver_socket, c->message, c->size_message, 0);
}

/*  Обработка команд */
int server_processing(Client *c) {
    st_cmd scmd = recv_message(c);

    int id;
    for (id = 0; id < ID_CMD_ENDOE; id ++) {
        if (strcmp(scmd.cmd, id_command[id].cmd) == 0) {
            break;
        }
    }
    switch (id) {
        case ID_CMD_USER : cmd_user(c, scmd.arg)     ; break;
        case ID_CMD_PASS : cmd_pass(c, scmd.arg)     ; break;
        case ID_CMD_ACCT :                             break;
        case ID_CMD_CWD  : cmd_cwd (c, scmd.arg)     ; break;
        case ID_CMD_QUIT : cmd_quit(c); return 1     ; break;
        case ID_CMD_PORT : cmd_port(c, scmd.arg)     ; break;
        case ID_CMD_LIST : cmd_list(c, scmd.arg)     ; break;
        case ID_CMD_SYST : cmd_syst(c)               ; break;
        case ID_CMD_PWD  : cmd_pwd (c)               ; break;
        case ID_CMD_RETR : cmd_retr(c, scmd.arg)     ; break;
        case ID_CMD_MKD  : cmd_mkd (c, scmd.arg)     ; break;
        case ID_CMD_RMD  : cmd_rmd (c, scmd.arg)     ; break;
        case ID_CMD_DELE : cmd_dele(c, scmd.arg)     ; break;
        case ID_CMD_STOR : cmd_stor(c, scmd.arg)     ; break;
        case ID_CMD_RNFR : cmd_rnfr(c, scmd.arg)     ; break;
        case ID_CMD_NOOP : cmd_noop(c)               ; break;
 //       case ID_CMD_ABOR : cmd_abor(c)               ; break;
        default          : cmd_unkn(c)               ; break;
    }
    return 0;
}
/*  Парсинг команд */
st_cmd parse_command(Client *c) {
    st_cmd scmd;
    memset(scmd.arg, 0, sizeof scmd.arg);
    memset(scmd.cmd, 0, sizeof scmd.cmd);

    /*  До первого пробела CMD после ARG */
    char first_space = 0;
    int n = 0;
    for(int i = 0; i < (strlen(c->message) - 2); i++) {
        if (first_space) {
            scmd.arg[n++] = c->message[i];
        } else
        if ((!first_space) && (c->message[i] == ' ')) {
            first_space = 1;

        } else {
            scmd.cmd[i] = c->message[i];
        }
    }
    return scmd;
}

/*  Приветсвие сервера */
void hello_server(Client *c) {
    c->message = "220 Connection success.\n";
    send_message(c);
    c->message = "220 Welcome to my FTP server.\n";
    send_message(c);
}

/*  Неизввестная команда */
void cmd_unkn(Client *c) {
    c->message = "500 Unknown command\n";
    send_message(c);
}

/*  Доступные команды */
/*  Команда : Авторизация пользователя */
void cmd_user(Client *c, char *arg) {
    if (strcmp(arg, "123") == 0){
        c->username = malloc(32);
        memset(c->username, 0, 32);
        strcpy(c->username, arg);
        char temp[SIZE_BUFFER];
        snprintf(temp, SIZE_BUFFER, "331 [%s] login correct.\n", arg);
        c->flag_username_correct = 1;
        c->message = temp;
    } else {
        c->message = "530 Invalid USER.\n";
    }
    send_message(c);
}

/*  Команда : Ввод пароля */
void cmd_pass(Client *c, char *arg) {
    if (c->flag_username_correct == 1) {
        if ((strcmp(arg, "pass") == 0) && (c->flag_username_correct)){
            c->password = malloc(32);
            memset(c->password, 0, 32);
            strcpy(c->password, arg);
            c->flag_password_correct = 1;
            c->message = "230 User identified.\n";
        } else {
            c->message = "501 Invalid password.\n";
        }
    } else {
        c->message = "530 USER is not corrected.\n";
    }
    send_message(c);
}

/*  Команда : Закончить работу сервера */
void cmd_quit(Client *c) {
    c->message = "221 Goodbye.\n";
    send_message(c);
    closesocket(c->command_socket);
    closesocket(c->transceiver_socket);
    free(c);
}

/*  Команда : Переход в активный режим */
void cmd_port(Client *c, char *arg) {
    if (c->flag_password_correct) {
        int temp_up, temp_down = 0;
        int ip[4];
        sscanf(arg, "%d,%d,%d,%d,%d,%d", &ip[0], &ip[1], &ip[2], &ip[3], &temp_up, &temp_down);
        int port = (temp_up << 8) + temp_down;
        printf("[%s:%d]    Transceiver ip = %d.%d.%d.%d:%d\n", inet_ntoa(c->client_addrin.sin_addr), ntohs(c->client_addrin.sin_port) , ip[0], ip[1], ip[2], ip[3], port);

        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s == INVALID_SOCKET) {
            printf("Transceiver socket - invalid\n");
            c->message = "451 Transceiver socket - invalid.\n";
            send_message(c);
            return 0;
        }
        printf("[info] Transceiver socket - success\n");

        char str_ip[15];
        sprintf(str_ip, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        SOCKADDR_IN t_address;
        t_address.sin_family = AF_INET;
        t_address.sin_addr.s_addr = inet_addr(str_ip);
        t_address.sin_port = htons(port);

        if (connect(s,(SOCKADDR *)&t_address, sizeof t_address) == SOCKET_ERROR) {
            printf("Connect - failed\n");
            c->message = "451 Connect - failed.\n";
            send_message(c);
            return 0;
        }
        c->transceiver_socket = s;
        printf("[info] Connect - success\n");

        c->message = "200 PORT command successful\n";

    } else {
        c->message = "530 Login failed.\n";
    }

    send_message(c);
}

/*  Команда : Просмотр содержимого текущей директории */
/*  Формат отправляемых данных:__
*
*   --- TTT DD HH:mm:ss YYYY dS* n*
*   ____________________________________________________________
*    --- - Передача дня недели (Игнорируется)
*    TTT - Трёхбуквенное название месяца (Jan, Feb, Mar, ...);
*     DD - День;
*     hh - Часы;
*     mm - Минуты;
*     ss - Секунды;
*   YYYY - Год;
*      d - Атрибут каталога (<DIR>);
*      S - Размер файла;
*      * - Символ: до первого нецифрового символа;
*      n - Имя файла;
*      * - Символ: использовать все символы до конца строки;
*   ____________________________________________________________
*
*/
void cmd_list(Client *c, char* arg) {
    if (!c->flag_password_correct) {
        c->message = "530 Login failed.\n";
        send_message(c);
        return;
    }

    c->message = "125 Channel open, data exchange started.\n";
    send_message(c);

    DIR *folder;
    struct dirent *entry;
    struct stat filestat;
    char cwdpath[256];
    int count = 1;
    long long total = 0;

    printf("[info] Read directory : %s\n", c->pwd);
    folder = opendir(c->pwd);
    if(folder == NULL){
        perror("Unable to read directory");
        return;
    }

    /* Получение имени текущего каталога */
    getcwd(cwdpath, 256);

    char temp[SIZE_TEMP];
    char temp_name[SIZE_TEMP];
    memset(temp, 0, 128);
    /* Чтение из каталога*/
    while( (entry = readdir(folder)) )
    {
        memset(temp, 0, sizeof temp);
        memset(temp_name, 0, sizeof temp_name);
        snprintf(temp_name, SIZE_TEMP, "%s\\%s", c->pwd, entry->d_name);
        /* Получение имени файла */
        stat(temp_name, &filestat);
        char temp_size_file[24];
        /* Размер файла */
        if(S_ISDIR(filestat.st_mode)) {
            sprintf(temp_size_file,"%s", "d");
            //sprintf(temp, "%-16s%-8s  \t", entry->d_name, "<DIR>");
        } else {
            sprintf(temp_size_file," %ld", filestat.st_size);
            //sprintf(temp,"%-16s%8lld  \t", entry->d_name, filestat.st_size);
            total += filestat.st_size;
        }

        /* Получение даты и времени создания */
        char temp_ctime[26];
        memset(temp_ctime, 0, 26);
        snprintf(temp_ctime, 24 ,"%s",ctime(&filestat.st_mtime));
        sprintf(temp,"%s %s %s\r\n", temp_ctime, temp_size_file, entry->d_name);
        count++;
        c->message = temp;
        c->size_message = strlen(temp);
        send_data(c);
    }
    closedir(folder);

    c->message = "250 File transfer completed.\n";
    send_message(c);

    c->message = "226 Close the data connection.\n";
    send_message(c);
    closesocket(c->transceiver_socket);

}

/*  Команда : Информация о системе */
void cmd_syst(Client *c) {
    if (c->flag_password_correct) {
        c->message = "257 Windows.\n";
    } else {
        c->message = "530 Login failed.\n";
    }
    send_message(c);
}

/*  Команда : Вывод рабочего каталога */
/*  Текущая директория передается в строке ответа заключенной в кавычки */
void cmd_pwd(Client *c) {
    if (c->flag_password_correct) {
        char temp[SIZE_TEMP];
        snprintf(temp, SIZE_TEMP, "257 \"%s\"\n", c->pwd);
        c->message = temp;;
    } else {
        c->message = "530 Login failed.\n";
    }
    send_message(c);
}

/*  Команда : Изменение рабочего каталога */
void cmd_cwd(Client *c, char *arg) {
    if (c->flag_password_correct) {
        if ((strstr(arg, "\\") != 0) || ((strstr(arg, "/")) != 0)) {
            sprintf(c->pwd ,"%s", ++arg);
        } else
        if (strcmp(arg, "..") == 0 ) {
            char first_slash_r = 0;
            int i = strlen(c->pwd);
            while ((!first_slash_r)&&(i > 0)) {
                if (c->pwd[i] == '\\') first_slash_r = 1;
                c->pwd[i--] = 0;
            }
            if (i == 1) sprintf(c->pwd ,"%s", DEFAULT_PWD);
        } else {
            strcat(c->pwd,"\\");
            strcat(c->pwd,arg);
        }
#ifdef DEBUG
        printf("\tPWD = '%s'\n", c->pwd);
#endif // DEBUG
        c->message = "250 Directory successfully changed.\n";
    } else {
        c->message = "530 Login failed.\n";
    }

    send_message(c);
}

/*  Команда : Отправить файл */
void cmd_retr(Client *c, char *arg) {
    if (c->flag_password_correct) {
        char temp[SIZE_BUFFER_PWD];
        snprintf(temp,SIZE_BUFFER_PWD,"%s\\%s", c->pwd, arg);
        FILE *file = fopen(temp, "rb");
        if (file == 0) {
            c->message = "550 Failed to read file.\n";
            send_message(c);
            return;
        } else {
            c->message = "150 Opening BINARY mode data connection.\n";
            send_message(c);
        }

        /* Вычисление размера файла */
        long int save_pos, size_file;
        save_pos = ftell( file );
        fseek( file, 0L, SEEK_END );
        size_file = ftell( file );
        fseek( file, save_pos, SEEK_SET );

        /*  Перебор по всем байтам */
        char symbol;
        char tx_buffer[SIZE_BUFFER+1];
        memset(tx_buffer, 0, SIZE_BUFFER);
        c->message = tx_buffer;
        for (long int inc = 0; inc < size_file; inc ++) {
            symbol = fgetc(file);
            tx_buffer[(inc) % SIZE_BUFFER] = symbol;
            if ((((inc + 1) % SIZE_BUFFER) == 0) || (inc == size_file-1)) {
                tx_buffer[SIZE_BUFFER] = '\0';
                c->size_message = (inc) % SIZE_BUFFER + 1;
                send_data(c);
                memset(tx_buffer, 0, SIZE_BUFFER);
            }
        }
        fclose(file);
        closesocket(c->transceiver_socket);
        c->message = "226 File send OK.\n";
    } else {
        c->message = "530 Login failed.\n";
    }
    send_message(c);
}

/*  Команда : Создать директорию */
void cmd_mkd(Client *c, char *arg) {
    if (c->flag_dev) {
        char temp[SIZE_TEMP];
        memset(temp, 0, sizeof temp);
        snprintf(temp, SIZE_TEMP, "%s\\%s", c->pwd, arg);
        if (mkdir(temp) == 0) {
            c->message = "257 PATH is created.\n";
        } else {
            c->message = "501 PATH could not be created.\n";
        }
    } else {
        c->message = "530 Login failed.\n";
    }
    send_message(c);
}

/*  Команда : Удалить директорию */
void cmd_rmd(Client *c, char *arg) {
    if (c->flag_dev) {
        char temp[SIZE_TEMP];
        memset(temp, 0, sizeof temp);
        snprintf(temp, SIZE_TEMP, "%s\\%s", c->pwd, arg);
        if (rmdir(temp) == 0) {
            c->message = "250 PATH removed.\n";
        } else {
            c->message = "501 PATH could not be removed.\n";
        }
    } else {
        c->message = "530 Login failed.\n";
    }
    send_message(c);
}

/*  Команда : Удалить файл */
void cmd_dele(Client *c, char *arg) {
    if (c->flag_dev) {
        char temp[SIZE_TEMP];
        memset(temp, 0, sizeof temp);
        snprintf(temp, SIZE_TEMP, "%s\\%s", c->pwd, arg);
        if (remove(temp) == 0) {
            c->message = "250 file removed.\n";
        } else {
            c->message = "501 file could not be removed.\n";
        }
    } else {
        c->message = "530 Login failed.\n";
    }
    send_message(c);
}

/*  Команда : Скачать файл */
void cmd_stor(Client *c, char *arg) {
    if (c->flag_dev) {
        char temp[SIZE_TEMP];
        snprintf(temp,SIZE_TEMP , "%s\\%s", c->pwd, arg);
        FILE *file = fopen(temp,"wb");
        if (file == NULL) {
            c->message = "452 Error writing file.\n";
            send_message(c);
            return;
        }
        char buffer[SIZE_BUFFER];
        memset(buffer, 0, sizeof(buffer));

        c->message = "150 Opening BINARY mode data connection.\n";
        send_message(c);
        int byte = 1;
        while (byte > 0) {
            byte = recv(c->transceiver_socket, buffer, SIZE_BUFFER, 0);
            fwrite(buffer, byte, 1, file);
        }
        if (byte == -1) {
            c->message = "451 Local error.\n";
        }
        closesocket(c->transceiver_socket);
        fclose(file);
        c->message = "226 File send OK.\n";
    } else {
        c->message = "530 Login failed.\n";
    }
    send_message(c);
}

/*  Команда : Переименовать файл */
void cmd_rnfr(Client *c, char *arg) {
    if (c->flag_dev) {
        char temp[SIZE_BUFFER_ARG];
        sprintf(temp,"%s", arg);
        char pwd1_temp[SIZE_BUFFER_PWD];
        snprintf(pwd1_temp,SIZE_BUFFER_PWD, "%s\\%s", c->pwd, temp);
        c->message = "350 Need more information.\n";
        send_message(c);
        st_cmd st = recv_message(c);
        if (strcmp(st.cmd, "RNTO") == 0) {
            char pwd2_temp[SIZE_BUFFER_PWD];
            snprintf(pwd2_temp, SIZE_BUFFER_PWD, "%s\\%s", c->pwd, st.arg);
            if (rename(pwd1_temp, pwd2_temp) == 0 ){
                c->message = "250 Request was successful.\n";
            } else {
                c->message = "451 Local error.\n";
            }
        } else {
            c->message = "450 RNTO command needed\n";
        }
    } else {
        c->message = "530 Login failed.\n";
    }

    send_message(c);
}

/*  Команда : PING */
void cmd_noop(Client *c) {
    if (c->flag_password_correct) {
        c->message = "200 Ping\n";
    } else{
        c->message = "530 Login failed.\n";
    }
}

