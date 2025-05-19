Mini HTTP Server (Advanced)
💡 What You'll Learn: Sockets, Threads, Handling HTTP Requests

Build a basic HTTP server that serves static pages.
Use socket programming to handle incoming requests.
Implement multithreading to process multiple requests.
🔗 Key Concepts Used:
✅ Sockets (Networking)
✅ Multithreading
✅ File Handling

Uses sockets for networking (accepting incoming TCP connections)
Uses threads to handle multiple client requests concurrently
Reads static files from disk to serve as HTTP responses

Listen on a specified port (e.g., 8080)
Accept incoming connections
Parse basic HTTP GET requests
Serve static files from a directory (e.g., ./www/)
Use a thread per connection to handle multiple clients

    