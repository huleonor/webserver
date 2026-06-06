GET /index.html HTTP/1.1\r\n
Host: localhost\r\n
Connection: keep-alive\r\n
\r\n


POST /upload HTTP/1.1\r\n
Host: localhost\r\n
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryABC123\r\n
Content-Length: 256\r\n
Connection: keep-alive\r\n
\r\n
------WebKitFormBoundaryABC123\r\n
Content-Disposition: form-data; name="file"; filename="foto.jpg"\r\n
Content-Type: image/jpeg\r\n
\r\n
[bytes da imagem]\r\n
------WebKitFormBoundaryABC123--\r\n



DELETE /files/foto.jpg HTTP/1.1\r\n
Host: localhost\r\n
Connection: keep-alive\r\n
\r\n