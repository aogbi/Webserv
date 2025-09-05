#!/bin/bash

echo "🚀 WEBSERV STRESS TEST"
echo "====================="

SERVER_URL="http://localhost:8002"

echo "Starting stress tests..."

echo ""
echo "1. 📈 Testing concurrent connections (50 requests):"
start_time=$(date +%s.%N)
for i in {1..50}; do
    curl -s "$SERVER_URL/" > /dev/null &
done
wait
end_time=$(date +%s.%N)
duration=$(echo "$end_time - $start_time" | bc)
echo "   ✅ Completed 50 concurrent requests in ${duration}s"

echo ""
echo "2. 🔄 Testing rapid sequential requests (100 requests):"
start_time=$(date +%s.%N)
for i in {1..100}; do
    curl -s "$SERVER_URL/" > /dev/null
done
end_time=$(date +%s.%N)
duration=$(echo "$end_time - $start_time" | bc)
echo "   ✅ Completed 100 sequential requests in ${duration}s"

echo ""
echo "3. 🧪 Testing mixed request types:"
for i in {1..20}; do
    curl -s "$SERVER_URL/" > /dev/null &
    curl -s "$SERVER_URL/cgi-bin/time.py" > /dev/null &
    curl -s "$SERVER_URL/tours/" > /dev/null &
    curl -s -X POST "$SERVER_URL/" > /dev/null &
done
wait
echo "   ✅ Completed mixed request test"

echo ""
echo "4. 📊 Testing error handling under load:"
for i in {1..30}; do
    curl -s "$SERVER_URL/nonexistent$i" > /dev/null &
done
wait
echo "   ✅ Completed error handling stress test"

echo ""
echo "✅ All stress tests completed successfully!"
