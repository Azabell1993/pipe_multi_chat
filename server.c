#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#define SERVER_FIFO "/tmp/addition_fifo_server"
char client_names[20][100];  // 클라이언트 이름 저장 배열

// 데몬화 함수
void daemonize() {
    pid_t pid;

    // 1. 새로운 프로세스를 생성하고 부모 프로세스 종료
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

    // 3. 표준 입출력과 에러 출력 연결 해제
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 4. 루트 디렉토리로 변경
    if (chdir("/") < 0) {
        perror("chdir 실패");
        exit(1);
    }
}

int main() {
    printf("Server Program...\n");
    
    daemonize();  // 서버를 데몬화
    

    int fd, fd_client, bytes_read;
    char fifo_name_set[20][100];  // 클라이언트의 FIFO를 저장하는 배열
    int counter = 0;
    char buf[4096];
    char server_msg[1024];

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

    // 클라이언트 FIFO 배열 초기화
    for (int i = 0; i < 20; i++) {
        strcpy(fifo_name_set[i], "");
    }

    if (fork() == 0) {  // 자식 프로세스
        close(parent_to_child[1]); // 부모 -> 자식 쓰기용 파이프 닫기
        close(child_to_parent[0]); // 자식 -> 부모 읽기용 파이프 닫기

        // 자식 프로세스: 서버로부터 메시지 수신 및 브로드캐스트
        while(1) {
            char received_msg[1024];
            if (read(parent_to_child[0], received_msg, sizeof(received_msg)) > 0) {
                // 메시지 각 클라이언트에 전송
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
    } else {  // 부모 프로세스
        close(parent_to_child[0]); // 부모 -> 자식 읽기용 파이프 닫기
        close(child_to_parent[1]); // 자식 -> 부모 쓰기용 파이프 닫기

        while (1) {
            memset(buf, '\0', sizeof(buf));

            // 서버에서 메시지 읽기
            if ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                char *token;
                char *rest = buf;
                char client_fifo[100];
                char client_name[100];
                char msg[4096] = "";
                int is_new_client = 0;
                int is_broadcast = 0;

                // 받은 메시지 파싱
                token = strtok_r(rest, " ", &rest);
                if (token != NULL && strcmp(token, "new") == 0) {
                    // 새 클라이언트 등록
                    token = strtok_r(rest, " ", &rest);  // 클라이언트 이름
                    if (token != NULL) {
                        strcpy(client_name, token);
                        strcpy(fifo_name_set[counter], rest);  // FIFO 경로 저장
                        strcpy(client_names[counter], client_name);  // 클라이언트 이름 저장
                        counter++;
                        is_new_client = 1;
                    }
                } else if (token != NULL) {
                    // 메시지 보낸 클라이언트의 FIFO 이름 추출
                    strcpy(client_fifo, token);
                    token = strtok_r(rest, " ", &rest);
                    if (token != NULL && strcmp(token, "send") == 0) {
                        // 메시지 전송 모드
                        int client_index = -1;
                        for (int i = 0; i < counter; i++) {
                            if (strcmp(fifo_name_set[i], client_fifo) == 0) {
                                client_index = i;
                                break;
                            }
                        }
                        if (client_index != -1) {
                            sprintf(msg, "%s: %s", client_names[client_index], rest);  // 클라이언트 이름으로 메시지 작성
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
