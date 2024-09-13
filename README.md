## PIPE 기반 멀티 채팅 서버

### 1. 개요
이 프로젝트는 C 언어로 구현된 서버-클라이언트 기반 채팅 시스템입니다. 이 시스템은 서버와 클라이언트 간의 메시지 송수신을 **FIFO(이름 있는 파이프)**를 통해 처리하며, 서버는 여러 클라이언트와 통신할 수 있는 멀티플렉싱 기능을 수행합니다. 서버는 데몬화되어 백그라운드에서 지속적으로 실행되며, 클라이언트는 로그인 및 회원가입 기능을 통해 서버에 연결됩니다.

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
메시지가 **"new"**라는 키워드로 시작하면, 새로운 클라이언트를 등록하고 해당 클라이언트의 FIFO 경로를 저장합니다.
**"send"**라는 키워드로 메시지가 시작되면, 클라이언트가 보낸 메시지를 자식 프로세스에게 전달하여 브로드캐스트를 처리합니다.

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
