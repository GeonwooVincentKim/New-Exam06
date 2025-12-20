#!/bin/bash

PORT=1226
SERVER="./mini_serv"

echo "=== Manual Test Guide ==="
echo ""
echo "This script will start the server and provide instructions for manual testing."
echo ""
echo "Server will start on port $PORT"
echo ""

pkill -f "mini_serv.*$PORT" 2>/dev/null
sleep 1

$SERVER $PORT &
SERVER_PID=$!
sleep 2

echo "Server started with PID: $SERVER_PID"
echo ""
echo "=== Test Instructions ==="
echo ""
echo "1. Open a NEW terminal and run:"
echo "   nc 127.0.0.1 $PORT"
echo "   This will be Client 1 (ID: 0)"
echo ""
echo "2. Open ANOTHER terminal and run:"
echo "   nc 127.0.0.1 $PORT"
echo "   This will be Client 2 (ID: 1)"
echo "   Client 1 should see: 'server: client 1 just arrived'"
echo ""
echo "3. In Client 1 terminal, type: 'Hello' and press Enter"
echo "   Client 2 should see: 'client 0: Hello'"
echo ""
echo "4. In Client 2 terminal, type: 'World' and press Enter"
echo "   Client 1 should see: 'client 1: World'"
echo ""
echo "5. In Client 1 terminal, press Ctrl+C to disconnect"
echo "   Client 2 should see: 'server: client 0 just left'"
echo ""
echo "Press Enter when you're done testing..."
read

echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
echo "Server stopped."
