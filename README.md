## PIPE 기반 멀티 채팅 서버

### 1. 개요
이 프로젝트는 C 언어로 구현된 서버-클라이언트 기반 채팅 시스템입니다. 이 시스템은 서버와 클라이언트 간의 메시지 송수신을 **FIFO(이름 있는 파이프)** 를 통해 처리하며, 서버는 여러 클라이언트와 통신할 수 있는 멀티플렉싱 기능을 수행합니다. 서버는 데몬화되어 백그라운드에서 지속적으로 실행되며, 클라이언트는 로그인 및 회원가입 기능을 통해 서버에 연결됩니다.

### 2. 전체 아키텍처 설계
아래의 구조 설명은 서버와 클라이언트의 동작 및 통신 흐름을 설명합니다.

#### 서버 (Server)
- 데몬화: 서버는 백그라운드에서 실행되며, 표준 입출력(STDIN, STDOUT, STDERR)을 모두 닫고 루트 디렉터리로 변경됩니다.
- FIFO 생성: 서버는 /tmp/addition_fifo_server라는 이름으로 FIFO 파일을 생성하여 클라이언트의 메시지를 수신합니다.
- 클라이언트 관리: 서버는 여러 클라이언트의 이름과 FIFO 경로를 저장하여 클라이언트로부터 메시지를 수신하고 다른 클라이언트로 브로드캐스트합니다.
- 파이프 기반 통신: 서버는 fork()를 통해 부모와 자식 프로세스를 나눠 메시지 처리 및 브로드캐스트를 수행합니다.

#### 클라이언트 (Client)
- 로그인/회원가입: 클라이언트는 로그인 또는 회원가입을 통해 서버에 연결됩니다. 로그인이 성공하면, 클라이언트는 고유의 FIFO 파일을 생성하고 서버에 자신의 정보(이름, FIFO 경로)를 전송합니다.
- 메시지 송수신: 클라이언트는 서버로 메시지를 전송하고, 서버는 이를 다른 클라이언트들에게 브로드캐스트합니다. 클라이언트는 또한 서버로부터 메시지를 수신하여 화면에 표시합니다.
- 중복 로그인 방지: 클라이언트는 .user 파일을 생성하여 중복 로그인을 방지하며, 로그아웃 시 이 파일을 삭제합니다.

### 3. 통신 흐름
##### 통신 흐름 설명
<img width="908" alt="image" src="https://github.com/user-attachments/assets/8f300dd8-22b2-47b4-8a7a-83b42cc4dff9">
  

#### 파이프 및 FIFO 통신 흐름
서버의 부모-자식 프로세스 간 파이프 생성
- pipe() 시스템 호출을 사용하여 두 개의 파이프를 생성.
- parent_to_child: 부모 프로세스에서 자식 프로세스로 메시지를 전송.
- child_to_parent: 자식 프로세스에서 부모 프로세스로 메시지를 전송.

#### 부모 프로세스 (메시지 수신 및 처리)
부모 프로세스는 서버 FIFO에서 클라이언트가 보낸 메시지를 수신합니다.

#### 자식 프로세스 (메시지 브로드캐스트)
자식 프로세스는 부모로부터 파이프를 통해 받은 메시지를 다른 클라이언트들에게 전송합니다.
각 클라이언트의 FIFO를 열어, 해당 클라이언트로 메시지를 전달하고 FIFO를 닫습니다.

### 4. 로그인과 로그아웃
**[로그인]**
클라이언트가 실행되면, 사용자는 로그인 또는 회원가입 옵션을 선택합니다.
로그인 시, users.txt 파일에서 해당 사용자의 정보(이름, 비밀번호)를 확인합니다. 중복 로그인을 방지하기 위해 .user 파일이 생성되어 있는지 확인합니다.
로그인이 성공하면, 클라이언트는 고유한 FIFO 파일을 생성하고 서버에 자신의 정보를 전송합니다.

**[로그아웃]**
사용자가 "exit" 명령어를 입력하면 로그아웃 절차가 시작됩니다.
클라이언트는 .user 파일을 삭제하고, 서버와의 연결을 종료합니다.

### 5. Makefile 및 Script 설명
**[Makefile for Chat Server and Client]**
```
CC = gcc
LDFLAGS = -pthread
TARGET_SERVER = chat_server
TARGET_CLIENT = chat_client

SRCS_SERVER = server.c
SRCS_CLIENT = client.c
OBJS_SERVER = $(SRCS_SERVER:.c=.o)
OBJS_CLIENT = $(SRCS_CLIENT:.c=.o)

CFLAGS += -Wno-unused-variable -Wno-unused-function -Wno-implicit-function-declaration -pthread -Ilib/include

# Default rule
all: $(TARGET_SERVER) $(TARGET_CLIENT)

# Server build
$(TARGET_SERVER): $(OBJS_SERVER)
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) $(OBJS_SERVER) $(LDFLAGS)

# Client build
$(TARGET_CLIENT): $(OBJS_CLIENT)
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) $(OBJS_CLIENT) $(LDFLAGS)

# Object file creation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS_SERVER) $(OBJS_CLIENT) $(TARGET_SERVER) $(TARGET_CLIENT)

# Run server
run_server:
	./$(TARGET_SERVER)

# Run client
run_client:
	./$(TARGET_CLIENT)

.PHONY: all clean run_server run_client
CC: 사용될 컴파일러는 gcc.
CFLAGS: 경고 무시 옵션을 포함한 컴파일 플래그, -pthread를 통해 스레드 관련 기능 활성화.
LDFLAGS: 스레드 라이브러리를 링크하는 플래그.
all: 서버와 클라이언트를 각각 컴파일하여 실행 파일(chat_server, chat_client)을 생성.
clean: 중간 파일 및 실행 파일을 삭제.
```

**[daemon_start.sh]**
```
#!/bin/bash
# start_chat_server.sh

SERVER_PATH="/home/pi/Desktop/veda/workspace/save"

start() {
    echo "Starting chat server..."
    # nohup으로 백그라운드 실행 시 stdin을 /dev/null로 연결하여 입력 대기 없앰
    nohup $SERVER_PATH/chat_server > /dev/null 2>&1 < /dev/null &
    echo "Chat server started."
}

stop() {
    echo "Stopping chat server..."
    pkill -f chat_server
    echo "Chat server stopped."
}

restart() {
    stop
    start
}

status() {
    if pgrep -f chat_server > /dev/null
    then
        echo "Chat server is running."
    else
        echo "Chat server is not running."
    fi
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        restart
        ;;
    status)
        status
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac
exit 0
```

### 사용방법
> ./daemon_start [ start | stop  | restart | status ]
1. start: nohup 명령어로 서버를 백그라운드에서 실행하고, 출력과 입력을 /dev/null로 연결하여 화면에 출력되지 않도록 처리.
2. stop: pkill 명령어로 실행 중인 서버를 종료.
3. restart: 서버를 종료한 후 다시 시작.
4. status: 서버가 실행 중인지 여부를 확인.

#### make 및 스크립트 실행
<img width="904" alt="image" src="https://github.com/user-attachments/assets/04f9e4c1-7353-4d4b-b399-b1f0a56dc364">

#### 회원가입 후 채팅
<img width="920" alt="image" src="https://github.com/user-attachments/assets/1786455e-979e-4132-9a1c-c3690681dba9">

----------------------

## fork(), pipe() 통신 흐름 요약
### fork()와 pipe() 통신 흐름
#### `fork()`
1. 서버 프로그램은 fork()를 사용해 부모 프로세스와 자식 프로세스로 나뉩니다.
2. 부모 프로세스는 클라이언트로부터 메시지를 수신하고, 메시지를 파싱한 후 이를 자식 프로세스에 전달합니다.
3. 자식 프로세스는 부모로부터 받은 메시지를 다른 클라이언트에게 브로드캐스트하는 역할을 합니다.

#### `pipe()`
1. parent_to_child 파이프는 부모 프로세스가 클라이언트로부터 수신한 메시지를 자식 프로세스에 전달하는 데 사용됩니다.
2. child_to_parent 파이프는 자식 프로세스에서 부모 프로세스로 데이터를 전송할 때 사용되지만, 이 코드에서는 사용되지 않았습니다.

### 통신 흐름 정리
1. 클라이언트는 자신의 FIFO 경로와 사용자 이름을 서버에 전달합니다.
2. 서버는 새로운 클라이언트를 등록하고, 클라이언트로부터 메시지를 수신합니다.
3. 서버 부모 프로세스는 클라이언트가 보낸 메시지를 파싱하여 브로드캐스트 여부를 결정합니다.
4. 부모 프로세스는 파이프를 통해 자식 프로세스에 메시지를 전달하고, 자식 프로세스는 모든 클라이언트로 메시지를 브로드캐스트합니다.
5. 자식 프로세스는 클라이언트의 FIFO 파일을 열어 메시지를 각 클라이언트로 전송합니다.

----------------

## Server 코드 기술 상세 설명

이 코드는 FIFO(named pipe)와 fork() 및 pipe() 시스템 호출을 이용하여 클라이언트와 서버 간에 통신하는 채팅 서버 프로그램을 구현한 것입니다. 프로그램은 다중 클라이언트를 관리하며, 클라이언트로부터 메시지를 받아 다른 클라이언트로 브로드캐스트하는 역할을 합니다. 이 프로그램은 데몬화되어 백그라운드에서 실행되며, 클라이언트와의 통신을 위한 다양한 자원들을 관리합니다.

### 1. 헤더 파일 및 상수 정의
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#define SERVER_FIFO "/tmp/addition_fifo_server"  // 서버에서 사용할 FIFO 경로
char client_names[20][100];  // 클라이언트 이름을 저장하는 배열
```
이 코드에서는 클라이언트 이름과 FIFO 경로를 저장하고, 이를 통해 클라이언트 메시지를 관리합니다.
SERVER_FIFO는 서버가 사용하는 FIFO 파일의 경로를 나타냅니다. 이 파일을 통해 클라이언트가 서버로 메시지를 보낼 수 있습니다.

### 2. 데몬화 함수
```
void daemonize() {
    pid_t pid;

    // 1. 새로운 프로세스를 생성하고 부모 프로세스를 종료
    pid = fork();

    if (pid < 0) {
        perror("fork 실패");
        exit(1);
    }

    if (pid > 0) {
        // 부모 프로세스 종료
        exit(0);
    }

    // 2. 새로운 세션 생성
    if (setsid() < 0) {
        perror("setsid 실패");
        exit(1);
    }

    // 3. 표준 입출력과 에러 출력을 닫음
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 4. 루트 디렉터리로 변경
    if (chdir("/") < 0) {
        perror("chdir 실패");
        exit(1);
    }
}
```
daemonize() 함수는 서버 프로세스를 데몬화하여 백그라운드에서 실행되도록 만듭니다.
이 함수는 fork()를 사용해 자식 프로세스를 생성하고, 부모 프로세스를 종료시킵니다.
이후, 새로운 세션을 생성하고 표준 입출력과 에러 출력 파일을 닫습니다. 이로 인해 서버는 독립적인 데몬 프로세스로 동작하게 됩니다.

### 3. 메인 함수의 구조
```
int main() {
    printf("Server Program...\n");
    
    daemonize();  // 서버를 데몬화
```
메인 함수는 서버를 데몬화한 후 실행을 시작합니다.

### 3.1 서버용 FIFO 및 파이프 생성
```
    int fd, fd_client, bytes_read;
    char fifo_name_set[20][100];  // 각 클라이언트의 FIFO 경로를 저장하는 배열
    int counter = 0;
    char buf[4096];  // 클라이언트로부터 받은 데이터를 저장할 버퍼
    char server_msg[1024];  // 서버에서 브로드캐스트할 메시지

    // 부모-자식 간 파이프 생성
    int parent_to_child[2], child_to_parent[2];
    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        perror("pipe");
        exit(1);
    }
    
    // 서버용 이름 지정된 파이프 생성
    if ((mkfifo(SERVER_FIFO, 0664) == -1) && (errno != EEXIST)) {
        perror("mkfifo");
        exit(1);
    }

    // 서버용 FIFO 열기
    if ((fd = open(SERVER_FIFO, O_RDONLY)) == -1) {
        perror("open server fifo");
        exit(1);
    }
```
- pipe() 시스템 호출을 사용해 두 개의 파이프를 생성합니다.
- parent_to_child 파이프: 부모 프로세스가 자식 프로세스로 데이터를 전송하는 데 사용됩니다.
- child_to_parent 파이프: 자식 프로세스가 부모 프로세스로 데이터를 전송하는 데 사용됩니다.
- FIFO(mkfifo)를 통해 서버는 클라이언트로부터 데이터를 수신할 수 있는 파일 경로(/tmp/addition_fifo_server)를 생성합니다.

### 3.2 클라이언트 FIFO 초기화
```
    // 클라이언트 FIFO 배열 초기화
    for (int i = 0; i < 20; i++) {
        strcpy(fifo_name_set[i], "");
    }
```
fifo_name_set 배열은 클라이언트의 FIFO 파일 경로를 저장합니다. 이 배열은 최대 20개의 클라이언트를 처리할 수 있도록 설계되어 있으며, 처음에는 모든 항목을 빈 문자열로 초기화합니다.

### 4. 프로세스 분기 (fork)
#### 4.1 자식 프로세스: 메시지 브로드캐스트 처리
```
    if (fork() == 0) {  // 자식 프로세스
        close(parent_to_child[1]);  // 부모 -> 자식 쓰기용 파이프 닫기
        close(child_to_parent[0]);  // 자식 -> 부모 읽기용 파이프 닫기

        // 자식 프로세스: 서버로부터 메시지 수신 및 브로드캐스트
        while(1) {
            char received_msg[1024];
            if (read(parent_to_child[0], received_msg, sizeof(received_msg)) > 0) {
                // 모든 클라이언트에게 메시지 전송
                for (int i = 0; i < counter; i++) {
                    if (strlen(fifo_name_set[i]) > 0) {
                        if ((fd_client = open(fifo_name_set[i], O_WRONLY)) == -1) {
                            perror("open client fifo");
                            continue;
                        }
                        if (write(fd_client, received_msg, strlen(received_msg)) != strlen(received_msg)) {
                            perror("write");
                        }
                        if (close(fd_client) == -1) {
                            perror("close client fifo");
                        }
                    }
                }
            }
        }
    }
```
자식 프로세스는 부모 프로세스로부터 parent_to_child 파이프를 통해 메시지를 읽고, 이 메시지를 모든 클라이언트에게 브로드캐스트하는 역할을 합니다.
각 클라이언트의 FIFO 경로가 fifo_name_set 배열에 저장되어 있으며, 이를 통해 클라이언트 FIFO 파일에 메시지를 전송합니다.

#### 4.2 부모 프로세스: 클라이언트로부터 메시지 수신 및 처리
```
    } else {  // 부모 프로세스
        close(parent_to_child[0]);  // 부모 -> 자식 읽기용 파이프 닫기
        close(child_to_parent[1]);  // 자식 -> 부모 쓰기용 파이프 닫기

        while (1) {
            memset(buf, '\0', sizeof(buf));

            // 서버에서 클라이언트로부터 메시지 읽기
            if ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                char *token;
                char *rest = buf;
                char client_fifo[100];
                char client_name[100];
                char msg[4096] = "";
                int is_new_client = 0;
                int is_broadcast = 0;

                // 메시지 파싱
                token = strtok_r(rest, " ", &rest);
                if (token != NULL && strcmp(token, "new") == 0) {
                    // 새 클라이언트 등록
                    token = strtok_r(rest, " ", &rest);  // 클라이언트 이름
                    if (token != NULL) {
                        strcpy(client_name, token);
                        strcpy(fifo_name_set[counter], rest);  // 클라이언트 FIFO 경로 저장
                        strcpy(client_names[counter], client_name);  // 클라이언트 이름 저장
                        counter++;
                        is_new_client = 1;
                    }
                } else if (token != NULL) {
                    // 클라이언트 메시지 전송 모드
                    strcpy(client_fifo, token);
                    token = strtok_r(rest, " ", &rest);
                    if (token != NULL && strcmp(token, "send") == 0) {
                        int client_index = -1;
                        for (int i = 0; i < counter; i++) {
                            if (strcmp(fifo_name_set[i], client_fifo) == 0) {
                                client_index = i;
                                break;
                            }
                        }
                        if (client_index != -1) {
                            // 클라이언트 이름 포함 메시지 작성
                            sprintf(msg, "%s: %s", client_names[client_index], rest);
                            is_broadcast = 1;
                        }
                    }
                }

                // 메시지 브로드캐스트
                if (is_broadcast) {
                    for (int i = 0; i < counter; i++) {
                        if (strcmp(fifo_name_set[i], client_fifo) != 0 && strlen(fifo_name_set[i]) > 0) {
                            if ((fd_client = open(fifo_name_set[i], O_WRONLY)) == -1) {
                                perror("open client fifo");
                                continue;
                            }
                            if (write(fd_client, msg, strlen(msg)) != strlen(msg)) {
                                perror("write");
                            }
                            if (close(fd_client) == -1) {
                                perror("close client fifo");
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
```
1. 부모 프로세스는 서버에서 클라이언트가 보낸 메시지를 read()를 통해 수신합니다.
2. 수신한 메시지는 파싱을 통해 클라이언트가 새로운 클라이언트인지, 기존 클라이언트가 메시지를 전송했는지 확인합니다.
3. 클라이언트가 새로운 경우, 클라이언트의 이름과 FIFO 경로를 저장합니다.
4. 클라이언트가 메시지를 보낸 경우, 해당 클라이언트의 이름과 메시지를 다른 클라이언트들에게 브로드캐스트하도록 설정합니다.
5. 부모 프로세스는 파싱한 메시지를 자식 프로세스로 전달하기 위해 parent_to_child 파이프를 사용합니다.


----------------------


## Client 코드 기술 상세 설명

> 클라이언트 코드 프로그램은 사용자의 로그인/회원가입을 처리하고, 클라이언트가 서버에 연결되어 메시지를 주고받는 시스템을 구현한 코드 입니다.

#### 1. 필요한 헤더 및 상수 정의
```
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

#define SERVER_FIFO "/tmp/addition_fifo_server"  // 서버에서 사용하는 FIFO 경로
#define USERS_FILE "users.txt"  // 사용자 정보가 저장되는 파일
```
프로그램에서 사용할 라이브러리와 상수들이 정의됩니다.
SERVER_FIFO는 서버에서 메시지를 받을 때 사용하는 FIFO 파일의 경로입니다.
USERS_FILE은 사용자 이름과 비밀번호가 저장되는 텍스트 파일입니다.

#### 2. 사용자 정보를 파일에 저장하는 함수
```
/**
 * @brief 사용자 정보를 파일에 저장하는 함수
 * @param username 사용자 이름
 * @param password 사용자 비밀번호
 * @return void
 */
void save_user_info(const char *username, const char *password) {
    FILE *file = fopen(USERS_FILE, "a");  // 파일을 append 모드로 엽니다.
    if (file == NULL) {
        perror("사용자 정보 파일 열기 실패");
        exit(1);  // 파일 열기에 실패하면 프로그램 종료
    }
    fprintf(file, "%s %s\n", username, password);  // 파일에 사용자 이름과 비밀번호를 저장
    fclose(file);  // 파일을 닫습니다.
}
```
save_user_info 함수는 새로 등록한 사용자의 이름과 비밀번호를 users.txt 파일에 저장합니다.
파일이 제대로 열리지 않으면 오류 메시지를 출력하고 프로그램을 종료합니다.
파일에 정보를 저장한 후 닫습니다.

#### 3. 사용자 존재 여부를 확인하는 함수
```
/**
 * @brief 사용자가 이미 존재하는지 확인하는 함수
 * @param username 사용자 이름
 * @return true 사용자 존재함, false 사용자 없음
 */
bool check_user_exists(const char *username) {
    FILE *file = fopen(USERS_FILE, "r");  // 파일을 읽기 모드로 엽니다.
    if (file == NULL) {
        // 파일이 없을 때 false 반환 (즉, 사용자가 존재하지 않음)
        return false;
    }

    char file_username[100];
    char file_password[100];

    // 파일을 한 줄씩 읽어와서 사용자 이름을 비교합니다.
    while (fscanf(file, "%s %s", file_username, file_password) != EOF) {
        if (strcmp(file_username, username) == 0) {
            fclose(file);  // 사용자를 찾으면 파일을 닫고 true 반환
            return true;
        }
    }

    fclose(file);  // 파일에서 사용자를 찾지 못했을 때 false 반환
    return false;
}
```
check_user_exists 함수는 users.txt 파일에서 해당 사용자가 존재하는지 확인합니다.
파일에서 사용자 이름을 검색하여 일치하는 사용자가 있으면 true를 반환하고, 없으면 false를 반환합니다.

### 4. 로그인 상태 확인 함수
```
/**
 * @brief 사용자가 이미 로그인 상태인지 확인하는 함수
 * @param username 사용자 이름
 * @return true 로그인 상태, false 비로그인 상태
 */
bool is_user_logged_in(const char *username) {
    char filename[128];
    sprintf(filename, ".%s.user", username);  // ".username.user" 형태의 파일명을 생성

    // 해당 파일이 존재하면 이미 로그인된 상태로 간주
    if (access(filename, F_OK) == 0) {
        return true;
    }
    return false;
}
```
is_user_logged_in 함수는 사용자가 이미 로그인되어 있는지 확인합니다.
.username.user 형식의 파일이 존재하면 사용자가 로그인된 상태로 간주하고 true를 반환합니다.

#### 5. 로그인 상태 파일 생성 함수
```
/**
 * @brief 로그인 시 사용자 상태 파일을 생성하는 함수
 * @param username 사용자 이름
 * @return void
 */
void create_user_file(const char *username) {
    char filename[128];
    sprintf(filename, ".%s.user", username);  // ".username.user" 파일명을 생성

    FILE *file = fopen(filename, "w");  // 쓰기 모드로 파일 생성
    if (file == NULL) {
        perror("파일 생성 실패");
        exit(1);  // 파일 생성 실패 시 프로그램 종료
    }
    fprintf(file, "User: %s\n", username);  // 파일에 사용자 정보를 기록
    fclose(file);  // 파일을 닫습니다.
}
```
사용자가 로그인할 때 .username.user 파일을 생성하여 해당 사용자가 로그인 상태임을 표시합니다.
파일을 생성하지 못하면 오류 메시지를 출력하고 프로그램을 종료합니다.

#### 6. 로그아웃 시 파일 삭제 함수
```
/**
 * @brief 로그아웃 시 사용자 상태 파일을 삭제하는 함수
 * @param username 사용자 이름
 * @return void
 */
void delete_user_file(const char *username) {
    char filename[128];
    sprintf(filename, ".%s.user", username);  // 삭제할 파일명 생성

    if (unlink(filename) == -1) {  // 파일 삭제 시도
        perror("파일 삭제 실패");
    } else {
        printf("%s님 로그아웃. 파일 삭제 완료.\n", username);  // 성공 시 메시지 출력
    }
}
```
사용자가 로그아웃할 때 .username.user 파일을 삭제하여 로그아웃 상태로 만듭니다.
파일 삭제에 실패하면 오류 메시지를 출력합니다.

#### 7. 메인 함수 작동 순서
```
int main(int argc, char *argv[]) {
    int fd_server, fd;
    char buf1[512], buf2[1024];
    char my_fifo_name[128];
    char username[100];
    char password[100];
    int choice;
```
##### 7.1 명령행 인자 확인
```
    // 잘못된 명령행 인자가 전달된 경우
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <server_ip>\n", argv[0]);
        exit(1);
    }
```
사용자가 프로그램을 실행할 때 서버 IP 주소를 명령행 인자로 제공하지 않으면 에러 메시지를 출력하고 프로그램이 종료됩니다.

##### 7.2 로그인 또는 회원가입 선택
```
    while (1) {
        printf("1. 로그인\n");
        printf("2. 회원가입\n");
        printf("선택: ");
        scanf("%d", &choice);
        getchar();  // 입력 버퍼 비우기

        if (choice == 1) {  // 로그인
```
사용자는 프로그램 실행 후 로그인 또는 회원가입을 선택할 수 있습니다.
1을 선택하면 로그인 절차로, 2를 선택하면 회원가입 절차로 넘어갑니다.

##### 7.3 로그인 처리
```
            system("clear");
            printf("사용할 이름을 입력하세요: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';  // 개행 문자 제거

            // 중복 로그인 확인
            if (is_user_logged_in(username)) {
                printf("이미 로그인 중입니다. 다른 사용자로 시도하세요.\n");
                continue;
            }

            printf("비밀번호를 입력하세요: ");
            fgets(password, sizeof(password), stdin);
            password[strcspn(password, "\n")] = '\0';  // 개행 문자 제거

            // 사용자 정보 확인
            if (!check_user_exists(username)) {
                printf("사용자가 존재하지 않습니다. 회원가입을 먼저 진행하세요.\n");
                continue;
            }

            printf("%s님, 로그인에 성공했습니다!\n", username);
            break;
```
로그인 선택 시, 사용자는 사용자 이름과 비밀번호를 입력합니다.
입력된 사용자 이름이 이미 로그인 상태인지 확인한 후, 로그인되지 않았다면 비밀번호 확인을 진행합니다.
비밀번호 확인 후, 사용자가 존재하지 않으면 회원가입을 유도합니다.
##### 7.4 회원가입 처리
```
        } else if (choice == 2) {  // 회원가입
            system("clear");
            printf("회원가입할 이름을 입력하세요: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = '\0';

            // 이미 존재하는 사용자 확인
            if (check_user_exists(username)) {
                printf("이미 존재하는 사용자입니다. 다른 이름으로 시도하세요.\n");
                continue;
            }

            printf("비밀번호를 입력하세요: ");
            fgets(password, sizeof(password), stdin);
            password[strcspn(password, "\n")] = '\0';

            // 사용자 정보를 텍스트 파일에 저장
            save_user_info(username, password);

            printf("%s님, 회원가입에 성공했습니다!\n", username);
            break;
```
회원가입 선택 시, 사용자 이름과 비밀번호를 입력받아 save_user_info 함수를 통해 users.txt 파일에 정보를 저장합니다.

##### 7.5 클라이언트 전용 FIFO 생성
```
    // 클라이언트 전용 FIFO 생성
    sprintf(my_fifo_name, "/tmp/add_client_fifo%ld", (long)getpid());
    if (mkfifo(my_fifo_name, 0664) == -1) {
        perror("mkfifo");
        exit(1);
    }

    printf("%s님, 채팅에 오신 것을 환영합니다!\n", username);
```
클라이언트는 서버와 통신할 수 있도록 자신의 고유 FIFO 파일을 생성합니다.
mkfifo 함수를 통해 프로세스 ID를 기반으로 FIFO 파일을 /tmp 디렉토리에 생성합니다.

#### 7.6 서버로 클라이언트 정보 전송
```
    // 서버로 새 클라이언트 정보 전송
    char new_client_msg[128];
    sprintf(new_client_msg, "new %s %s", username, my_fifo_name);  // 클라이언트 이름과 FIFO 경로 전송
    
    if ((fd_server = open(SERVER_FIFO, O_WRONLY)) == -1) {
        perror("open server fifo");
        exit(1);
    }
    
    if (write(fd_server, new_client_msg, strlen(new_client_msg)) != strlen(new_client_msg)) {
        perror("write");
        exit(1);
    }
    
    if (close(fd_server) == -1) {
        perror("close server fifo");
        exit(1);
    }

    // .user 파일 생성 (로그인 처리)
    create_user_file(username);
```
클라이언트가 생성된 FIFO 정보를 서버에 전달합니다.
서버는 이 정보를 통해 새 클라이언트를 인식하게 됩니다.
.user 파일을 생성하여 로그인 상태를 기록합니다.

##### 7.7 프로세스 분기: 메시지 수신과 전송 처리
```
    if (fork() == 0) {  // 자식 프로세스: 서버로부터 메시지 수신
        while (1) {
            char received_msg[1024];
            if ((fd = open(my_fifo_name, O_RDONLY)) == -1) {
                perror("open client fifo");
                exit(1);
            }

            memset(received_msg, '\0', sizeof(received_msg));  // 수신 버퍼 초기화

            // 서버로부터 메시지 수신
            if (read(fd, received_msg, sizeof(received_msg)) > 0) {
                printf("[상대방 ID] %s\n", received_msg);
            }

            if (close(fd) == -1) {
                perror("close client fifo");
                exit(1);
            }
        }
    } else {  // 부모 프로세스: 서버로 메시지 전송
        while (1) {
            printf("\n 채팅 입력 : ");
            if (fgets(buf1, sizeof(buf1), stdin) == NULL) {
                break;
            }

            // 사용자가 "exit" 입력하면 로그아웃 처리
            if (strncmp(buf1, "exit", 4) == 0) {
                printf("로그아웃합니다.\n");
                delete_user_file(username);  // .user 파일 삭제
                close(fd_server);
                break;
            }

            // 메시지를 서버로 전송
            strcpy(buf2, my_fifo_name);
            strcat(buf2, " send ");
            strcat(buf2, buf1);
            
            if ((fd_server = open(SERVER_FIFO, O_WRONLY)) == -1) {
                perror("open server fifo");
                exit(1);
            }

            if (write(fd_server, buf2, strlen(buf2)) != strlen(buf2)) {
                perror("write");
                exit(1);
            }

            if (close(fd_server) == -1) {
                perror("close server fifo");
                exit(1);
            }
        }
    }
```

#### 클라이언트는 fork()를 사용하여 자식 프로세스와 부모 프로세스로 분기합니다.
- 자식 프로세스: 서버로부터 자신의 FIFO를 통해 메시지를 수신하고, 이를 출력합니다.
- 부모 프로세스: 사용자가 입력한 메시지를 서버로 전송합니다. "exit"를 입력하면 로그아웃 처리 후 종료됩니다.

#### 클라이언트 코드 요약
프로그램은 사용자가 로그인 또는 회원가입을 선택하도록 합니다.
로그인 후, 클라이언트는 자신의 FIFO를 생성하고 서버와 연결을 설정합니다.
클라이언트는 fork()를 통해 메시지 송수신을 처리하며, 자식 프로세스는 서버로부터 메시지를 수신하고, 부모 프로세스는 사용자가 입력한 메시지를 서버로 전송합니다.
클라이언트는 로그아웃 시 .user 파일을 삭제하여 로그인 상태를 해제합니다.


