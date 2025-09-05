#!/bin/sh

echo "🌐 Testing Webserv HTTP Server"
echo "================================"

# Start the server in background
./Webserv &
SERVER_PID=$!
sleep 1

echo "✅ Server started (PID: $SERVER_PID)"

echo "\n📄 Testing homepage:"
curl -s http://localhost:8002/ | grep -o "<title>.*</title>"

echo "\n📄 Testing specific file:"
curl -s http://localhost:8002/tours/tours1.html | grep -o "<title>.*</title>"

echo "\n❌ Testing 404 error:"
curl -s http://localhost:8002/nonexistent | grep -o "<h1>.*</h1>"

echo "\n🔒 Testing method restriction (POST to root):"
curl -s -X POST http://localhost:8002/ | grep -o "<h1>.*</h1>"

echo "\n🚀 Testing DELETE method:"
curl -s -X DELETE http://localhost:8002/tours/ | grep -o "<h1>.*</h1>"

echo "\n⛔ Testing unsupported method:"
curl -s -X PATCH http://localhost:8002/ | grep -o "<h1>.*</h1>"

# Stop the server
kill $SERVER_PID
echo "\n🛑 Server stopped"
echo "\nTest completed! 🎉"
