Fluxo do programa:

1. Parsing do config file
    - ler e validar o ficheiro de configuração
    - guardar as configs de cada server e location

2. Setup dos servers
    - criar sockets para cada porta definida no config
    - bind + listen em cada socket

3. Event loop (poll)
    - único poll() para tudo
    - monitorizar e tratar por ordem:

        a) Novos pedidos de conexão
            - accept → cria novo socket para o cliente
				-> adicionar a lista de fds a checkar no poll()

        b) Receber request (read/recv)
            - ler os dados do cliente:
				-> pode ou nao chegar o request de uma vez

        c) Parsing do request
            - validar se cumpre o protocolo HTTP
            - extrair método (GET, POST, DELETE)
            - extrair path e query string
            - extrair headers
            - extrair body (se POST)
            - un-chunkar se for chunked encoding

        d) Processar o pedido
            - encontrar o location que casa com o path
            - verificar se o método é permitido
            - verificar tamanho do body
            - decidir o que fazer:
                - servir ficheiro estático (GET)
                - guardar ficheiro (POST upload)
                - apagar ficheiro (DELETE)
                - executar CGI (.php, .py)
                - redirecionar (return 301)
                - mostrar directory listing (autoindex)

        e) Executar CGI (se necessário)
            - fork + execve do script
            - pipes para stdin/stdout
            - variáveis de ambiente
            - ler resposta via EOF

        f) Enviar response (write/send)
            - status code correto
            - headers (Content-Type, Content-Length, Connection)
            - body (ficheiro, HTML gerado, output do CGI)

        g) Gestão da conexão
            - fechar se cliente enviou Connection: close
            - fechar se timeout (muito tempo sem atividade)
            - keep-alive se cliente pediu (opcional)

Módulos:

- Config Parser (Hugo)
    - lê o ficheiro de configuração
    - valida e guarda configs de cada server e location
    - devolve estrutura pronta a usar

- Server / Socket setup (Carlos)
    - cria sockets para cada porta do config
    - bind + listen em cada socket
    - event loop com poll()
    - accept de novas conexões
    - recebe dados do cliente (read/recv)
    - passa dados em bruto ao Hugo (HTTP Request Parser)

- HTTP Request Parser (Hugo)
    - recebe dados em bruto do Carlos
    - valida se é um request HTTP válido
    - extrai método, path, query string, headers, body
    - un-chunkar body se vier em chunks
    - devolve estrutura organizada ao Carlos

- Request Handler (Carlos)
    - recebe request parseado do Hugo
    - encontra o location que casa com o path
    - verifica se método é permitido
    - verifica tamanho do body
    - decide e executa:
        GET    → servir ficheiro estático
        POST   → guardar ficheiro (upload)
        DELETE → apagar ficheiro
        CGI    → executar script
        301    → redirecionar
        pasta  → autoindex ou 403

- Response Builder (Carlos)
    - constrói a response HTTP
    - define status code, headers e body
    - envia ao cliente (write/send)

- CGI Handler (ambos no fim)
    - fork + execve do script
    - pipes para stdin/stdout
    - variáveis de ambiente
    - lê resposta e passa ao Response Builder


Classes:
- ServerConfig: Serve para guardar todas as configuraçoes do server
{
	porta: um numero
		-> inicializar com um default
	host: uma string com ip (ex: 127.1.1.0)
		-> inicializar com um default
	root: uma string (ex: /var/www/)
		-> inicializar com um default
	client_max_body_size: unsigned long
	index: uma string (ex: index.html)
		-> inicializar com um default
	error pages: std::map(codigo erro + paginas)
	location: vetor da class Location
}

- Location: serve para guardar as locations de cada server
{
	root:string (ex: /var/www/html)
	path: string (ex: /)
}

- ServerManager: Serve para gerir todo o programa
{
	pollfds: um vetor de structs de pollfd
	clients: uma std::map -> clientfd, toda a info do client
		-> assim o programa sabe o estado de cada cliente, o que ja recebeu,
			qual o server associado para ver locations, regras, response a enviar, etc
	servers: um vetor de servers para no loop de accept() percorrer

}