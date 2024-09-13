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

/**
 * @brief 서버 프로그램을 데몬화하는 함수
 * @return void
 */
void daemonize() {
    pid_t pid;

    // 1. 새로운 프로세스를 생성하고 부모 프로세스를 종료
    pid = fork();

    if (pid < 0) {
        // fork 실패 시 오류 출력
        perror("fork 실패");
        exit(1);
    }

    if (pid > 0) {
        // 부모 프로세스 종료
        exit(0);
    }

    // 2. 새로운 세션 생성
    if (setsid() < 0) {
        // 새로운 세션 생성 실패 시 오류 출력
        perror("setsid 실패");
        exit(1);
    }

    // 3. 표준 입출력과 에러 출력을 닫음
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // 4. 루트 디렉터리로 변경
    if (chdir("/") < 0) {
        // 디렉터리 변경 실패 시 오류 출력
        perror("chdir 실패");
        exit(1);
    }
}

int main() {
    printf("Server Program...\n");
    
    daemonize();  // 서버를 데몬화
    

    int fd, fd_client, bytes_read;
    char fifo_name_set[20][100];  // 클라이언트의 FIFO를 저장하는 배열
    int counter = 0;  // 클라이언트 수를 저장하는 카운터
    char buf[4096];  // 수신한 메시지를 저장할 버퍼
    char server_msg[1024];  // 서버에서 보낼 메시지 버퍼

    // 부모-자식 간 파이프 생성
    int parent_to_child[2], child_to_parent[2];
    if (pipe(parent_to_child) == -1 || pipe(child_to_parent) == -1) {
        // 파이프 생성 실패 시 오류 출력
        perror("pipe");
        exit(1);
    }
    
    // 서버용 이름 지정된 파이프 생성
    if ((mkfifo(SERVER_FIFO, 0664) == -1) && (errno != EEXIST)) {
        // FIFO 생성 실패 시 오류 출력
        perror("mkfifo");
        exit(1);
    }

    // 서버용 FIFO 열기
    if ((fd = open(SERVER_FIFO, O_RDONLY)) == -1) {
        // FIFO 열기 실패 시 오류 출력
        perror("open server fifo");
        exit(1);
    }

    // 클라이언트 FIFO 배열 초기화
    for (int i = 0; i < 20; i++) {
        strcpy(fifo_name_set[i], "");
    }

    if (fork() == 0) {  // 자식 프로세스
        // 부모 -> 자식 쓰기용 파이프 닫기
        close(parent_to_child[1]);
        // 자식 -> 부모 읽기용 파이프 닫기
        close(child_to_parent[0]);

        // 자식 프로세스: 서버로부터 메시지 수신 및 클라이언트로 브로드캐스트
        while(1) {
            char received_msg[1024];  // 수신 메시지 저장 버퍼
            if (read(parent_to_child[0], received_msg, sizeof(received_msg)) > 0) {
                // 모든 클라이언트에게 메시지 전송
                for (int i = 0; i < counter; i++) {
                    if (strlen(fifo_name_set[i]) > 0) {
                        // 클라이언트 FIFO 열기
                        if ((fd_client = open(fifo_name_set[i], O_WRONLY)) == -1) {
                            perror("open client fifo");
                            continue;
                        }
                        // 클라이언트로 메시지 전송
                        if (write(fd_client, received_msg, strlen(received_msg)) != strlen(received_msg)) {
                            perror("write");
                        }
                        // 클라이언트 FIFO 닫기
                        if (close(fd_client) == -1) {
                            perror("close client fifo");
                        }
                    }
                }
            }
        }
    } else {  // 부모 프로세스
        // 부모 -> 자식 읽기용 파이프 닫기
        close(parent_to_child[0]);
        // 자식 -> 부모 쓰기용 파이프 닫기
        close(child_to_parent[1]);

        while (1) {
            memset(buf, '\0', sizeof(buf));  // 버퍼 초기화

            // 서버에서 메시지 읽기
            if ((bytes_read = read(fd, buf, sizeof(buf))) > 0) {
                char *token;  // 메시지 토큰 처리용
                char *rest = buf;
                char client_fifo[100];  // 클라이언트 FIFO 경로 저장
                char client_name[100];  // 클라이언트 이름 저장
                char msg[4096] = "";  // 최종 브로드캐스트 메시지
                int is_new_client = 0;  // 신규 클라이언트 여부 플래그
                int is_broadcast = 0;  // 브로드캐스트 여부 플래그

                // 받은 메시지 파싱
                token = strtok_r(rest, " ", &rest);
                if (token != NULL && strcmp(token, "new") == 0) {
                    // 새로운 클라이언트 등록 처리
                    token = strtok_r(rest, " ", &rest);  // 클라이언트 이름
                    if (token != NULL) {
                        strcpy(client_name, token);
                        strcpy(fifo_name_set[counter], rest);  // FIFO 경로 저장
                        strcpy(client_names[counter], client_name);  // 클라이언트 이름 저장
                        counter++;
                        is_new_client = 1;
                    }
                } else if (token != NULL) {
                    // 기존 클라이언트가 메시지를 보낸 경우
                    strcpy(client_fifo, token);  // 메시지 보낸 클라이언트 FIFO 저장
                    token = strtok_r(rest, " ", &rest);
                    if (token != NULL && strcmp(token, "send") == 0) {
                        // 메시지 전송 모드 처리
                        int client_index = -1;
                        for (int i = 0; i < counter; i++) {
                            if (strcmp(fifo_name_set[i], client_fifo) == 0) {
                                client_index = i;
                                break;
                            }
                        }
                        if (client_index != -1) {
                            // 메시지 작성 (보낸 클라이언트 이름 포함)
                            sprintf(msg, "%s: %s", client_names[client_index], rest);
                            is_broadcast = 1;  // 브로드캐스트 플래그 설정
                        }
                    }
                }

                // 메시지 브로드캐스트
                if (is_broadcast) {
                    for (int i = 0; i < counter; i++) {
                        // 메시지 보낸 클라이언트 제외하고 다른 클라이언트에 전송
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
