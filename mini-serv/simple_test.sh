#!/bin/bash

PORT=1225
SERVER="./mini_serv"

cleanup() {
    pkill -f "mini_serv.*$PORT" 2>/dev/null
    exit 0
}

trap cleanup EXIT INT TERM

pkill -f "mini_serv.*$PORT" 2>/dev/null
sleep 1

echo "=== Simple Test ==="
echo "Starting server on port $PORT"
$SERVER $PORT &
SERVER_PID=$!
sleep 2

echo "Server PID: $SERVER_PID"
echo ""

echo "Test 1: Two clients connecting"
echo "Client 1 output:"
(echo "Hello"; sleep 3) | timeout 4 nc 127.0.0.1 $PORT &
CLIENT1_PID=$!
sleep 1

echo "Client 2 output:"
(sleep 1; echo "World"; sleep 2) | timeout 4 nc 127.0.0.1 $PORT

wait $CLIENT1_PID 2>/dev/null

echo ""
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
