#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>

#define SERVER_FIFO "/tmp/addition_fifo_server"
#define USERS_FILE "users.txt"

/**
 * @brief 사용자 정보를 파일에 저장하는 함수
 * @param username 사용자 이름
 * @param password 사용자 비밀번호
 * @return void
 */
void save_user_info(const char *username, const char *password) {
    FILE *file = fopen(USERS_FILE, "a");  // append 모드로 파일 열기
    if (file == NULL) {
        perror("사용자 정보 파일 열기 실패");
        exit(1);
    }
    fprintf(file, "%s %s\n", username, password);  // 사용자 이름과 비밀번호 저장
    fclose(file);
}

/**
 * @brief 사용자가 이미 존재하는지 확인하는 함수
 * @param username 사용자 이름
 * @return true 사용자 존재함, false 사용자 없음
 */
bool check_user_exists(const char *username) {
    FILE *file = fopen(USERS_FILE, "r");  // 읽기 모드로 파일 열기
    if (file == NULL) {
        // 파일이 없을 때는 false 반환
        return false;
    }

    char file_username[100];
    char file_password[100];

    // 파일에서 사용자 이름 탐색
    while (fscanf(file, "%s %s", file_username, file_password) != EOF) {
        if (strcmp(file_username, username) == 0) {
            fclose(file);
            return true;
        }
    }

    fclose(file);
    return false;
}

/**
 * @brief 사용자가 이미 로그인 상태인지 확인하는 함수
 * @param username 사용자 이름
 * @return true 로그인 상태, false 비로그인 상태
 */
bool is_user_logged_in(const char *username) {
    char filename[128];
    sprintf(filename, ".%s.user", username);  // 로그인 여부 확인용 파일명 생성

    // 파일이 존재하면 이미 로그인된 상태
    if (access(filename, F_OK) == 0) {
        return true;
    }
    return false;
}

/**
 * @brief 로그인 시 사용자 상태 파일을 생성하는 함수
 * @param username 사용자 이름
 * @return void
 */
void create_user_file(const char *username) {
    char filename[128];
    sprintf(filename, ".%s.user", username);  // 로그인 상태 파일명 생성

    FILE *file = fopen(filename, "w");  // 쓰기 모드로 파일 생성
    if (file == NULL) {
        perror("파일 생성 실패");
        exit(1);
    }
    fprintf(file, "User: %s\n", username);  // 파일에 사용자 정보 기록
    fclose(file);
}

/**
 * @brief 로그아웃 시 사용자 상태 파일을 삭제하는 함수
 * @param username 사용자 이름
 * @return void
 */
void delete_user_file(const char *username) {
    char filename[128];
    sprintf(filename, ".%s.user", username);  // 삭제할 파일명 생성

    if (unlink(filename) == -1) {
        perror("파일 삭제 실패");
    } else {
        printf("%s님 로그아웃. 파일 삭제 완료.\n", username);
    }
}

int main(int argc, char *argv[]) {
    int fd_server, fd;
    char buf1[512], buf2[1024];
    char my_fifo_name[128];
    char username[100];
    char password[100];
    int choice;

    // 잘못된 명령행 인자가 전달된 경우
    if (argc != 2) {
        fprintf(stderr, "사용법: %s <server_ip>\n", argv[0]);
        exit(1);
    }

    // 로그인 또는 회원가입 선택
    while (1) {
        printf("1. 로그인\n");
        printf("2. 회원가입\n");
        printf("선택: ");
        scanf("%d", &choice);
        getchar();  // 입력 버퍼 비우기

        if (choice == 1) {  // 로그인
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
        } else {
            printf("잘못된 선택입니다. 다시 시도하세요.\n");
        }
    }

    // 클라이언트 전용 FIFO 생성
    sprintf(my_fifo_name, "/tmp/add_client_fifo%ld", (long)getpid());
    if (mkfifo(my_fifo_name, 0664) == -1) {
        perror("mkfifo");
        exit(1);
    }

    printf("%s님, 채팅에 오신 것을 환영합니다!\n", username);

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

    return 0;
}
