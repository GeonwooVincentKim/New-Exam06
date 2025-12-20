#!/bin/bash

PORT=1223
SERVER="./mini_serv"

cleanup() {
    pkill -f "mini_serv.*$PORT" 2>/dev/null
    rm -f /tmp/test_*.txt
    exit 0
}

trap cleanup EXIT INT TERM

pkill -f "mini_serv.*$PORT" 2>/dev/null
sleep 1

echo "=== Detailed Test Suite ==="
echo "Starting server on port $PORT"
$SERVER $PORT &
SERVER_PID=$!
sleep 2

if ! ps -p $SERVER_PID > /dev/null; then
    echo "ERROR: Server failed to start"
    exit 1
fi

echo "Server PID: $SERVER_PID"
echo ""

# テスト1: クライアント1が接続し、クライアント2が接続したときに通知を受信
echo "Test 1: Client 1 should receive notification when Client 2 connects"
echo "test1" | timeout 3 nc 127.0.0.1 $PORT > /tmp/test_client1.txt &
CLIENT1_PID=$!
sleep 1

echo "test2" | timeout 3 nc 127.0.0.1 $PORT > /tmp/test_client2.txt &
CLIENT2_PID=$!
sleep 2

if grep -q "server: client 1 just arrived" /tmp/test_client1.txt 2>/dev/null; then
    echo "✓ PASS: Client 1 received arrival notification"
else
    echo "✗ FAIL: Client 1 did not receive arrival notification"
fi

wait $CLIENT1_PID 2>/dev/null
wait $CLIENT2_PID 2>/dev/null

# テスト2: クライアント間のメッセージ送信
echo ""
echo "Test 2: Message broadcasting between clients"
(echo "Hello from client 0"; sleep 2) | timeout 5 nc 127.0.0.1 $PORT > /tmp/test_msg1.txt &
CLIENT1_PID=$!
sleep 1

(echo "test"; sleep 2) | timeout 5 nc 127.0.0.1 $PORT > /tmp/test_msg2.txt &
CLIENT2_PID=$!
sleep 3

if grep -q "client 0: Hello from client 0" /tmp/test_msg2.txt 2>/dev/null; then
    echo "✓ PASS: Client 2 received message from Client 1"
else
    echo "✗ FAIL: Client 2 did not receive message from Client 1"
    echo "Client 2 output:"
    cat /tmp/test_msg2.txt 2>/dev/null || echo "(empty)"
fi

wait $CLIENT1_PID 2>/dev/null
wait $CLIENT2_PID 2>/dev/null

# テスト3: 切断通知
echo ""
echo "Test 3: Disconnect notification"
(echo "test"; sleep 1) | timeout 3 nc 127.0.0.1 $PORT > /tmp/test_disc1.txt &
CLIENT1_PID=$!
sleep 1

(echo "test"; sleep 3) | timeout 5 nc 127.0.0.1 $PORT > /tmp/test_disc2.txt &
CLIENT2_PID=$!
sleep 2

wait $CLIENT1_PID 2>/dev/null
sleep 1

if grep -q "server: client 0 just left" /tmp/test_disc2.txt 2>/dev/null; then
    echo "✓ PASS: Client 2 received disconnect notification"
else
    echo "✗ FAIL: Client 2 did not receive disconnect notification"
    echo "Client 2 output:"
    cat /tmp/test_disc2.txt 2>/dev/null || echo "(empty)"
fi

wait $CLIENT2_PID 2>/dev/null

echo ""
echo "=== Test Summary ==="
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
