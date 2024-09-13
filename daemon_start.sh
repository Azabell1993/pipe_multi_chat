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
