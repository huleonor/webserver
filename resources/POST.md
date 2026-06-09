POST
	- recebe o request
	- check se path é valido e safe
	- check se existe location
	- check se metodo é valido nessa location
	- check se path existe


Códigos de resposta POST:
	- 200 OK                    → processado com sucesso (resposta com corpo)
	- 201 Created               → recurso criado com sucesso
	- 204 No Content            → sucesso sem corpo de resposta
	- 400 Bad Request           → corpo/cabeçalhos malformados
	- 401 Unauthorized          → autenticação necessária
	- 403 Forbidden             → sem permissão
	- 404 Not Found             → endpoint não existe
	- 405 Method Not Allowed    → POST não permitido nesta location
	- 409 Conflict              → conflito com estado atual do recurso
	- 411 Length Required       → Content-Length em falta
	- 413 Payload Too Large     → corpo excede client_max_body_size
	- 415 Unsupported Media Type→ Content-Type não suportado
	- 500 Internal Server Error → erro interno do servidor



/upload

/upload/file