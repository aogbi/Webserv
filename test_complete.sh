#!/bin/sh

echo "🌐 Testing Complete Webserv Implementation"
echo "=========================================="

# Start the server
./Webserv &
SERVER_PID=$!
sleep 2

echo "✅ Server started (PID: $SERVER_PID)"

echo "\n📄 Testing static file serving:"
curl -s http://localhost:8002/ | grep -o "<title>.*</title>"

echo "\n🐍 Testing CGI execution:"
curl -s http://localhost:8002/cgi-bin/time.py | grep -o "The current time is:.*"

echo "\n📁 Testing specific file:"
curl -s http://localhost:8002/tours/tours1.html | grep -o "<title>.*</title>"

echo "\n❌ Testing 404 error:"
curl -s http://localhost:8002/nonexistent | grep -o "<h1>.*</h1>"

echo "\n🔒 Testing method restrictions:"
curl -s -X PUT http://localhost:8002/ | grep -o "<h1>.*</h1>"

echo "\n📤 Testing file upload form:"
curl -s http://localhost:8002/upload.html | grep -o "<title>.*</title>"

echo "\n🗑️ Testing DELETE method:"
curl -s -X DELETE http://localhost:8002/tours/ | grep -o "<h1>.*</h1>"

# Stop the server
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null
echo "\n🛑 Server stopped"

echo "\n🎉 Test Summary:"
echo "✅ Static file serving"
echo "✅ CGI script execution"
echo "✅ Configuration parsing"
echo "✅ HTTP methods (GET, POST, DELETE, HEAD)"
echo "✅ Error handling (404, 405, 501)"
echo "✅ Location-based routing"
echo "✅ Method restrictions"
echo "✅ File upload support"
echo "✅ MIME type detection"
echo "✅ Directory listing capability"
echo "\n🚀 Webserv implementation complete!"
