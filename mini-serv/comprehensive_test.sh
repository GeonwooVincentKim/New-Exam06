#!/bin/bash

# 시작 포트 설정 (여기서부터 찾기 시작합니다)
PORT=50000
SERVER="./mini_serv"
PASS_COUNT=0
FAIL_COUNT=0
SERVER_PID=""

# ------------------------------------------------------------------
# 핵심 수정: 빈 포트를 찾을 때까지 시도하는 스마트한 서버 시작 함수
# ------------------------------------------------------------------
start_server() {
    # 이전 프로세스 정리
    pkill -f "mini_serv" 2>/dev/null
    sleep 0.2

    # 최대 10번까지 포트를 바꿔가며 시도
    for i in {1..10}; do
        echo "Attempting to start server on port $PORT..."
        
        $SERVER $PORT > /dev/null 2>&1 &
        SERVER_PID=$!
        sleep 0.5

        # 서버가 살아있는지 확인
        if ps -p $SERVER_PID > /dev/null; then
            echo "--> Success! Server is running on port $PORT"
            return 0
        else
            # 서버가 죽었다면 (Bind Error), 포트를 1 올리고 다시 시도
            echo "--> Port $PORT is busy or in TIME_WAIT. Trying next port..."
            ((PORT++))
        fi
    done

    # 10번 다 실패하면 에러
    echo "ERROR: Could not find any free port after 10 attempts."
    exit 1
}

# 서버 종료 함수
stop_server() {
    if [ -n "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
    fi
    # 현재 사용된 포트에 붙은 nc 프로세스들만 정리
    pkill -f "nc 127.0.0.1 $PORT" 2>/dev/null
    rm -f /tmp/test_*.txt
}

cleanup() {
    stop_server
    echo ""
    echo "=== Final Summary ==="
    echo "Passed: $PASS_COUNT"
    echo "Failed: $FAIL_COUNT"
    exit 0
}

trap cleanup EXIT INT TERM

echo "=== Comprehensive Test Suite (Auto-Port Selection) ==="
echo "The script will automatically find a free port for each test."
echo ""

# ----------------------------------------------------------------
# Test 1: Two clients connection notification
# ----------------------------------------------------------------
start_server # <--- 여기서 알아서 포트를 결정함
echo "=== Test 1: Two clients - connection notifications (Port $PORT) ==="
# Client 0 (Observer) 접속
nc 127.0.0.1 $PORT > /tmp/test1_c1.txt 2>&1 &
CLIENT1_PID=$!
sleep 1
# Client 1 (Newcomer) 접속
nc 127.0.0.1 $PORT > /tmp/test1_c2.txt 2>&1 &
CLIENT2_PID=$!
sleep 2

if grep -q "server: client 1 just arrived" /tmp/test1_c1.txt 2>/dev/null; then
    echo "✓ PASS: Client 0 received notification about Client 1"
    ((PASS_COUNT++))
else
    echo "✗ FAIL: Connection notifications"
    echo "--- Client 0 output ---"
    cat /tmp/test1_c1.txt 2>/dev/null
    ((FAIL_COUNT++))
fi
stop_server
((PORT++)) # 다음 테스트를 위해 기본적으로 하나 증가시켜 둠
sleep 1

# ----------------------------------------------------------------
# Test 2: Message broadcasting
# ----------------------------------------------------------------
start_server
echo "=== Test 2: Message broadcasting (Port $PORT) ==="
# 1. Receiver (Client 0) 접속
nc 127.0.0.1 $PORT > /tmp/test2_receiver.txt 2>&1 &
RECEIVER_PID=$!
sleep 1

# 2. Sender (Client 1) 접속
(echo "Hello from sender"; sleep 1) | nc 127.0.0.1 $PORT > /tmp/test2_sender.txt 2>&1 &
SENDER_PID=$!
sleep 2

if grep -q "client 1: Hello from sender" /tmp/test2_receiver.txt 2>/dev/null; then
    echo "✓ PASS: Message broadcasting"
    ((PASS_COUNT++))
else
    echo "✗ FAIL: Message broadcasting"
    echo "Receiver content:"
    cat /tmp/test2_receiver.txt 2>/dev/null
    ((FAIL_COUNT++))
fi
stop_server
((PORT++))
sleep 1

# ----------------------------------------------------------------
# Test 3: Disconnect notification
# ----------------------------------------------------------------
start_server
echo "=== Test 3: Disconnect notification (Port $PORT) ==="
# Client 0
nc 127.0.0.1 $PORT > /tmp/test3_c1.txt 2>&1 &
CLIENT1_PID=$!
sleep 1
# Client 1
nc 127.0.0.1 $PORT > /tmp/test3_c2.txt 2>&1 &
CLIENT2_PID=$!
sleep 1

# Client 1 종료
kill $CLIENT2_PID 2>/dev/null
wait $CLIENT2_PID 2>/dev/null
sleep 2

if grep -q "server: client 1 just left" /tmp/test3_c1.txt 2>/dev/null; then
    echo "✓ PASS: Disconnect notification"
    ((PASS_COUNT++))
else
    echo "✗ FAIL: Disconnect notification"
    echo "Client 0 received:"
    cat /tmp/test3_c1.txt 2>/dev/null
    ((FAIL_COUNT++))
fi
stop_server
((PORT++))
sleep 1

# ----------------------------------------------------------------
# Test 4: Partial message
# ----------------------------------------------------------------
start_server
echo "=== Test 4: Partial message handling (Port $PORT) ==="
# Receiver
nc 127.0.0.1 $PORT > /tmp/test4_receiver.txt 2>&1 &
RECEIVER_PID=$!
sleep 1

# Sender (Split)
(
    echo -n "Hel"
    sleep 0.3
    echo "lo"
    sleep 1
) | nc 127.0.0.1 $PORT > /tmp/test4_sender.txt 2>&1 &
SENDER_PID=$!
sleep 2

if grep -q "client 1: Hello" /tmp/test4_receiver.txt 2>/dev/null; then
    echo "✓ PASS: Partial message handling"
    ((PASS_COUNT++))
else
    echo "✗ FAIL: Partial message handling"
    echo "Receiver received:"
    cat /tmp/test4_receiver.txt 2>/dev/null
    ((FAIL_COUNT++))
fi
stop_server
((PORT++))
sleep 1

# ----------------------------------------------------------------
# Test 5: Multiple lines
# ----------------------------------------------------------------
start_server
echo "=== Test 5: Multiple lines in one message (Port $PORT) ==="
# Receiver
nc 127.0.0.1 $PORT > /tmp/test5_receiver.txt 2>&1 &
RECEIVER_PID=$!
sleep 1

# Sender (Multi-line)
(printf "Line 1\nLine 2\nLine 3\n"; sleep 1) | nc 127.0.0.1 $PORT > /tmp/test5_sender.txt 2>&1 &
SENDER_PID=$!
sleep 2

LINES=$(grep -c "client 1: Line" /tmp/test5_receiver.txt 2>/dev/null || echo "0")
if [ "$LINES" -ge 3 ]; then
    echo "✓ PASS: Multiple lines handling ($LINES lines)"
    ((PASS_COUNT++))
else
    echo "✗ FAIL: Multiple lines handling (got $LINES lines, expected 3)"
    echo "Receiver received:"
    cat /tmp/test5_receiver.txt 2>/dev/null
    ((FAIL_COUNT++))
fi
stop_server