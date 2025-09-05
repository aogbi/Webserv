#!/bin/bash

# Create test directories and files
mkdir -p uploads
mkdir -p error_pages

# Create test HTML files
cat > docs/fusion_web/test.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Test Page</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { color: #333; border-bottom: 2px solid #333; }
        .test-section { margin: 20px 0; padding: 20px; border: 1px solid #ddd; }
        .success { color: green; }
        .error { color: red; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Webserv Test Page</h1>
        
        <div class="test-section">
            <h2>Static File Serving Test</h2>
            <p>If you can see this page, static file serving is working! ✅</p>
        </div>
        
        <div class="test-section">
            <h2>CGI Test</h2>
            <p><a href="/cgi-bin/time.py">Test Python CGI Script</a></p>
            <p><a href="/cgi-bin/info.sh">Test Shell CGI Script</a></p>
        </div>
        
        <div class="test-section">
            <h2>File Upload Test</h2>
            <form action="/upload" method="post" enctype="multipart/form-data">
                <input type="file" name="upload" required>
                <button type="submit">Upload File</button>
            </form>
        </div>
        
        <div class="test-section">
            <h2>Method Testing</h2>
            <p><a href="/tours">Directory with autoindex (GET)</a></p>
            <p><a href="/red">Redirect test</a></p>
        </div>
    </div>
</body>
</html>
EOF

# Create additional CGI script
cat > cgi-bin/info.sh << 'EOF'
#!/bin/bash

echo "Content-Type: text/html"
echo ""
echo "<!DOCTYPE html>"
echo "<html>"
echo "<head><title>Server Info</title></head>"
echo "<body>"
echo "<h1>Server Information</h1>"
echo "<p><strong>Date:</strong> $(date)</p>"
echo "<p><strong>Server:</strong> Webserv/1.0</p>"
echo "<p><strong>Request Method:</strong> $REQUEST_METHOD</p>"
echo "<p><strong>Request URI:</strong> $REQUEST_URI</p>"
echo "<p><strong>Server Protocol:</strong> $SERVER_PROTOCOL</p>"
echo "<h2>Environment Variables:</h2>"
echo "<ul>"
echo "<li>REQUEST_METHOD: $REQUEST_METHOD</li>"
echo "<li>REQUEST_URI: $REQUEST_URI</li>"
echo "<li>SERVER_PROTOCOL: $SERVER_PROTOCOL</li>"
echo "<li>HTTP_HOST: $HTTP_HOST</li>"
echo "<li>HTTP_USER_AGENT: $HTTP_USER_AGENT</li>"
echo "<li>CONTENT_TYPE: $CONTENT_TYPE</li>"
echo "<li>CONTENT_LENGTH: $CONTENT_LENGTH</li>"
echo "</ul>"
echo "<a href='/'>← Back to Home</a>"
echo "</body>"
echo "</html>"
EOF

chmod +x cgi-bin/info.sh
chmod +x cgi-bin/time.py

# Create custom error page
cat > error_pages/404.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>404 - Page Not Found</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
        .error { color: #d32f2f; }
        .container { max-width: 600px; margin: 0 auto; }
    </style>
</head>
<body>
    <div class="container">
        <h1 class="error">404 - Page Not Found</h1>
        <p>The requested page could not be found on this server.</p>
        <p>This is a custom error page served by Webserv.</p>
        <a href="/">Return to Homepage</a>
    </div>
</body>
</html>
EOF

# Create directory for autoindex test
mkdir -p docs/fusion_web/tours
echo "<h1>Tour 1</h1><p>Sample tour content.</p>" > docs/fusion_web/tours/tour1.html
echo "<h1>Tour 2</h1><p>Another tour page.</p>" > docs/fusion_web/tours/tour2.html
echo "<h1>Default Tours</h1><p>Default tours page.</p>" > docs/fusion_web/tours/tours1.html

echo "✅ Test setup completed!"
