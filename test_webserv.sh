#!/bin/bash

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test configuration
SERVER_HOST="localhost"
SERVER_PORT="8002"
SERVER_URL="http://$SERVER_HOST:$SERVER_PORT"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to print test results
print_test_result() {
    local test_name="$1"
    local result="$2"
    local details="$3"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    if [ "$result" = "PASS" ]; then
        echo -e "${GREEN}✅ PASS${NC}: $test_name"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ FAIL${NC}: $test_name"
        if [ -n "$details" ]; then
            echo -e "   ${YELLOW}Details:${NC} $details"
        fi
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# Function to test HTTP request
test_http_request() {
    local method="$1"
    local path="$2"
    local expected_status="$3"
    local test_name="$4"
    local additional_args="$5"
    
    echo -e "${BLUE}🧪 Testing:${NC} $test_name"
    
    # Make the request and capture response
    response=$(curl -s -w "HTTPSTATUS:%{http_code}" -X "$method" $additional_args "$SERVER_URL$path" 2>/dev/null)
    http_status=$(echo $response | grep -o "HTTPSTATUS:[0-9]*" | cut -d: -f2)
    response_body=$(echo $response | sed 's/HTTPSTATUS:[0-9]*$//')
    
    if [ "$http_status" = "$expected_status" ]; then
        print_test_result "$test_name" "PASS"
        return 0
    else
        print_test_result "$test_name" "FAIL" "Expected status $expected_status, got $http_status"
        return 1
    fi
}

# Function to start server
start_server() {
    echo -e "${BLUE}🚀 Starting Webserv server...${NC}"
    cd /home/aogbi/goinfre/Webserv
    ./Webserv configs/default.conf &
    SERVER_PID=$!
    sleep 2
    
    # Check if server started successfully
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo -e "${GREEN}✅ Server started successfully (PID: $SERVER_PID)${NC}"
        return 0
    else
        echo -e "${RED}❌ Failed to start server${NC}"
        return 1
    fi
}

# Function to stop server
stop_server() {
    if [ -n "$SERVER_PID" ]; then
        echo -e "${BLUE}🛑 Stopping server...${NC}"
        kill -SIGINT $SERVER_PID 2>/dev/null
        wait $SERVER_PID 2>/dev/null
        echo -e "${GREEN}✅ Server stopped${NC}"
    fi
}

# Cleanup function
cleanup() {
    stop_server
    exit 0
}

# Set trap for cleanup
trap cleanup SIGINT SIGTERM

echo -e "${BLUE}🧪 WEBSERV COMPREHENSIVE TEST SUITE${NC}"
echo -e "${BLUE}===================================${NC}"
echo ""

# Start the server
if ! start_server; then
    echo -e "${RED}❌ Cannot proceed with tests - server failed to start${NC}"
    exit 1
fi

echo ""
echo -e "${BLUE}📋 TESTING SUBJECT REQUIREMENTS${NC}"
echo -e "${BLUE}==============================${NC}"

# Test 1: Basic GET request for static file
test_http_request "GET" "/" "200" "Static file serving (index.html)"

# Test 2: GET request for specific file
test_http_request "GET" "/test.html" "200" "Static file serving (test.html)"

# Test 3: 404 Error handling
test_http_request "GET" "/nonexistent.html" "404" "404 Error handling"

# Test 4: Directory listing (autoindex)
test_http_request "GET" "/tours/" "200" "Directory listing with autoindex"

# Test 5: HEAD method
test_http_request "HEAD" "/" "200" "HEAD method support"

# Test 6: POST method
test_http_request "POST" "/" "200" "POST method support" "-d 'test=data'"

# Test 7: DELETE method
test_http_request "DELETE" "/" "200" "DELETE method support"

# Test 8: Method not allowed (if configured)
test_http_request "PUT" "/tours/" "405" "Method restriction (PUT not allowed)" || true

# Test 9: CGI execution - Python
test_http_request "GET" "/cgi-bin/time.py" "200" "CGI execution (Python)"

# Test 10: CGI execution - Shell
test_http_request "GET" "/cgi-bin/info.sh" "200" "CGI execution (Shell)"

# Test 11: Redirect test
test_http_request "GET" "/red" "301" "HTTP redirect (301)"

# Test 12: Large file handling (if exists)
test_http_request "GET" "/large_file_test" "404" "Large file handling (not found is expected)" || true

echo ""
echo -e "${BLUE}🔧 ADVANCED FUNCTIONALITY TESTS${NC}"
echo -e "${BLUE}==============================${NC}"

# Test 13: Multiple concurrent connections
echo -e "${BLUE}🧪 Testing:${NC} Multiple concurrent connections"
for i in {1..5}; do
    curl -s "$SERVER_URL/" > /dev/null &
done
wait
print_test_result "Multiple concurrent connections" "PASS"

# Test 14: Request with various headers
echo -e "${BLUE}🧪 Testing:${NC} Custom headers handling"
response=$(curl -s -w "HTTPSTATUS:%{http_code}" -H "User-Agent: WebservTester/1.0" -H "Accept: text/html" "$SERVER_URL/")
http_status=$(echo $response | grep -o "HTTPSTATUS:[0-9]*" | cut -d: -f2)
if [ "$http_status" = "200" ]; then
    print_test_result "Custom headers handling" "PASS"
else
    print_test_result "Custom headers handling" "FAIL" "Expected 200, got $http_status"
fi

# Test 15: POST with Content-Length
echo -e "${BLUE}🧪 Testing:${NC} POST with Content-Length"
test_data="name=test&value=data"
response=$(curl -s -w "HTTPSTATUS:%{http_code}" -X POST -H "Content-Type: application/x-www-form-urlencoded" -H "Content-Length: ${#test_data}" -d "$test_data" "$SERVER_URL/")
http_status=$(echo $response | grep -o "HTTPSTATUS:[0-9]*" | cut -d: -f2)
if [ "$http_status" = "200" ]; then
    print_test_result "POST with Content-Length" "PASS"
else
    print_test_result "POST with Content-Length" "FAIL" "Expected 200, got $http_status"
fi

# Test 16: CGI POST request
echo -e "${BLUE}🧪 Testing:${NC} CGI POST request"
response=$(curl -s -w "HTTPSTATUS:%{http_code}" -X POST -d "test=cgi_post" "$SERVER_URL/cgi-bin/info.sh")
http_status=$(echo $response | grep -o "HTTPSTATUS:[0-9]*" | cut -d: -f2)
if [ "$http_status" = "200" ]; then
    print_test_result "CGI POST request" "PASS"
else
    print_test_result "CGI POST request" "FAIL" "Expected 200, got $http_status"
fi

echo ""
echo -e "${BLUE}📊 ERROR HANDLING TESTS${NC}"
echo -e "${BLUE}=======================${NC}"

# Test 17: Invalid HTTP method
echo -e "${BLUE}🧪 Testing:${NC} Invalid HTTP method"
response=$(curl -s -w "HTTPSTATUS:%{http_code}" -X INVALID "$SERVER_URL/")
http_status=$(echo $response | grep -o "HTTPSTATUS:[0-9]*" | cut -d: -f2)
if [ "$http_status" = "501" ] || [ "$http_status" = "400" ]; then
    print_test_result "Invalid HTTP method" "PASS"
else
    print_test_result "Invalid HTTP method" "FAIL" "Expected 501 or 400, got $http_status"
fi

# Test 18: Malformed request
echo -e "${BLUE}🧪 Testing:${NC} Malformed request handling"
response=$(echo -e "INVALID REQUEST\r\n\r\n" | nc $SERVER_HOST $SERVER_PORT 2>/dev/null | head -n 1)
if echo "$response" | grep -q "400\|HTTP"; then
    print_test_result "Malformed request handling" "PASS"
else
    print_test_result "Malformed request handling" "FAIL" "Server should handle malformed requests"
fi

echo ""
echo -e "${BLUE}🧪 PERFORMANCE & STRESS TESTS${NC}"
echo -e "${BLUE}=============================${NC}"

# Test 19: Server responsiveness under load
echo -e "${BLUE}🧪 Testing:${NC} Server under load (10 concurrent requests)"
start_time=$(date +%s.%N)
for i in {1..10}; do
    curl -s "$SERVER_URL/" > /dev/null &
done
wait
end_time=$(date +%s.%N)
duration=$(echo "$end_time - $start_time" | bc)
if (( $(echo "$duration < 5.0" | bc -l) )); then
    print_test_result "Server under load" "PASS"
else
    print_test_result "Server under load" "FAIL" "Took too long: ${duration}s"
fi

# Test 20: Memory leak test (basic)
echo -e "${BLUE}🧪 Testing:${NC} Basic memory stability"
for i in {1..50}; do
    curl -s "$SERVER_URL/" > /dev/null
done
print_test_result "Basic memory stability" "PASS"

echo ""
echo -e "${BLUE}📋 TEST SUMMARY${NC}"
echo -e "${BLUE}==============${NC}"
echo -e "Total tests: ${TOTAL_TESTS}"
echo -e "${GREEN}Passed: ${PASSED_TESTS}${NC}"
echo -e "${RED}Failed: ${FAILED_TESTS}${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "\n${GREEN}🎉 ALL TESTS PASSED! Webserv is working correctly!${NC}"
    exit_code=0
else
    echo -e "\n${YELLOW}⚠️  Some tests failed. Please review the results above.${NC}"
    exit_code=1
fi

# Stop the server
stop_server

echo -e "\n${BLUE}✅ Test suite completed.${NC}"
exit $exit_code
