#!/bin/bash

PORT=1222
SERVER="./mini_serv"
TEST_FILE="test_input.txt"

# クリーンアップ関数
cleanup() {
    echo "Cleaning up..."
    pkill -f "mini_serv.*$PORT" 2>/dev/null
    rm -f "$TEST_FILE"
    exit 0
}

trap cleanup EXIT INT TERM

# 既存のプロセスを終了
pkill -f "mini_serv.*$PORT" 2>/dev/null
sleep 1

# テストファイルを作成
echo -e "Hello from client 0\nLine 2\nLine 3" > "$TEST_FILE"

echo "=== Starting server on port $PORT ==="
$SERVER $PORT &
SERVER_PID=$!
sleep 2

# サーバーが起動しているか確認
if ! ps -p $SERVER_PID > /dev/null; then
    echo "ERROR: Server failed to start"
    exit 1
fi

echo "Server started with PID: $SERVER_PID"
echo ""

# テスト1: クライアント1を接続（バックグラウンド）
echo "=== Test 1: Client 1 connecting ==="
(echo "test message 1"; sleep 1) | nc -w 1 127.0.0.1 $PORT &
CLIENT1_PID=$!
sleep 1

# テスト2: クライアント2を接続してメッセージを送信
echo "=== Test 2: Client 2 connecting and sending message ==="
(echo "Hello from client 2"; sleep 1) | nc -w 1 127.0.0.1 $PORT &
CLIENT2_PID=$!
sleep 1

# テスト3: ファイルから送信
echo "=== Test 3: Sending from file ==="
nc -w 1 127.0.0.1 $PORT < "$TEST_FILE" &
CLIENT3_PID=$!
sleep 1

# すべてのプロセスが終了するまで待つ
wait $CLIENT1_PID 2>/dev/null
wait $CLIENT2_PID 2>/dev/null
wait $CLIENT3_PID 2>/dev/null

echo ""
echo "=== Tests completed ==="
sleep 1

# サーバーを終了
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo "Server stopped"
