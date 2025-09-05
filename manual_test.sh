#!/bin/bash

# Manual testing script for specific Webserv features
echo "🧪 MANUAL WEBSERV TESTING"
echo "========================"

SERVER_URL="http://localhost:8002"

echo ""
echo "1. 📁 Testing static file serving:"
curl -s "$SERVER_URL/" | head -n 5
echo ""

echo "2. 🐍 Testing Python CGI:"
curl -s "$SERVER_URL/cgi-bin/time.py" | head -n 10
echo ""

echo "3. 🐚 Testing Shell CGI:"
curl -s "$SERVER_URL/cgi-bin/info.sh" | head -n 10
echo ""

echo "4. 📂 Testing directory listing:"
curl -s "$SERVER_URL/tours/" | head -n 10
echo ""

echo "5. 🔄 Testing redirect:"
curl -s -I "$SERVER_URL/red" | head -n 5
echo ""

echo "6. ❌ Testing 404 error:"
curl -s "$SERVER_URL/nonexistent" | head -n 5
echo ""

echo "7. 📊 Testing server status with various methods:"
echo "   GET status:"
curl -s -w "Status: %{http_code}\n" -o /dev/null "$SERVER_URL/"

echo "   POST status:"
curl -s -w "Status: %{http_code}\n" -o /dev/null -X POST "$SERVER_URL/"

echo "   DELETE status:"
curl -s -w "Status: %{http_code}\n" -o /dev/null -X DELETE "$SERVER_URL/"

echo "   HEAD status:"
curl -s -w "Status: %{http_code}\n" -o /dev/null -I "$SERVER_URL/"

echo ""
echo "8. 📤 Testing file upload (simulated):"
curl -s -X POST -F "upload=@test_webserv.sh" "$SERVER_URL/" | head -n 10

echo ""
echo "✅ Manual testing completed!"
