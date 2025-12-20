#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

PORT=1111
DB_FILE=".save"

# Clean up function
cleanup() {
    pkill -f "./mini_db" 2>/dev/null
    rm -f "$DB_FILE"
}

# Run cleanup on script exit
trap cleanup EXIT

# Cleanup before starting
cleanup

# Start the server
./mini_db "$PORT" "$DB_FILE" &
SERVER_PID=$!
sleep 1  # Wait for server to start

# Check if server started successfully
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "${RED}✗${NC} Failed to start server"
    exit 1
fi

echo "Testing command sequence..."

# Function to send single command and get response
send_command() {
    local cmd="$1"
    echo "$cmd" | nc localhost "$PORT" | head -1
}

# Test each command individually
result1=$(send_command "POST A B")
result2=$(send_command "POST B C") 
result3=$(send_command "GET A")
result4=$(send_command "GET C")
result5=$(send_command "DELETE A")
result6=$(send_command "DELETE C") 
result7=$(send_command "UNKNOWN_COMMAND")

# Create test output
{
    echo "${result1}\$"
    echo "${result2}\$"  
    echo "${result3}\$"
    echo "${result4}\$"
    echo "${result5}\$"
    echo "${result6}\$"
    echo "${result7}\$"
} > test_output

expected="0\$
0\$
0 B\$
1\$
0\$
1\$
2\$"

if [ "$(cat test_output)" = "$expected" ]; then
    echo -e "${GREEN}✓${NC} All commands sequence passed"
else
    echo -e "${RED}✗${NC} Commands sequence failed"
    echo "Expected:"
    echo "$expected"
    echo "Got:"
    cat test_output
fi

rm -f test_output

echo "Testing persistence..."
# Kill server with SIGINT
kill -INT "$SERVER_PID" 2>/dev/null || pkill -SIGINT -f "./mini_db" 2>/dev/null
sleep 1

# Start server again and test persistence
./mini_db "$PORT" "$DB_FILE" &
SERVER_PID=$!
sleep 1

# Check if server started successfully
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo -e "${RED}✗${NC} Failed to restart server for persistence test"
    exit 1
fi

printf "GET B\n" | nc localhost "$PORT" | head -1 | cat -e > test_output
if [ "$(cat test_output)" = "0 C\$" ]; then
    echo -e "${GREEN}✓${NC} Persistence test passed"
else
    echo -e "${RED}✗${NC} Persistence test failed"
    echo "Expected: '0 C\$'"
    echo "Got: $(cat test_output)"
fi

rm -f test_output

# Final cleanup will be done by trap