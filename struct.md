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
                -> adicionar á lista de fds a checkar no poll()

        b) Receber request (read/recv)
            - ler os dados do cliente:
                -> pode ou nao chegar o request de uma vez

        c) Parsing do request
            - validar se cumpre o protocolo HTTP
            - extrair método (GET, POST, DELETE)
            - extrair path e query string
            - extrair headers
			- verificar tamanho do body
            - extrair body (se POST)
            - un-chunkar se for chunked encoding

        d) Processar o pedido
            - encontrar o location que liga com o path
            - verificar se o método é permitido
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
            - variáveis de ambiente (passar a query_string extraída no passo c)
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
    - recebe dados do cliente (read/recv):
		-> FASE 1: recebe ate detetar fim do header
			-> Hugo faz parsing do header
		-> FASE 2: vai recebendo o body e verificando o tamanho. Se ultrapassa, para
		-> A ter atençao:
			- esta a muito tempo sem dizer nada
			- desconectou

- HTTP Request Parser (Hugo)
	FASE1:
		- recebe dados em bruto do Carlos (objeto Client)
    	- valida se é um request HTTP válido
		- verifica o header Content-Length ou Chunked
    	- extrai método, path, query string, headers
	FASE2:
		- extrai body
    	- un-chunkar body se vier em chunks
    	- devolve estrutura organizada ao Carlos (dentro do objeto Client)

- Request Handler (Carlos)
    - recebe request parseado do Hugo
    - encontra o location que liga com o path
    - verifica se método é permitido
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
    - variáveis de ambiente (QUERY_STRING)
    - lê resposta e passa ao Response Builder


Classes:

- ServerConfig: Serve para guardar todas as configuraçoes do server
{
    porta: um numero
        -> inicializar com um default
    host: uma string com ip (ex: 127.1.1.0)
        -> inicializar com um default (0.0.0.0)
    root: uma string (ex: /var/www/)
        -> inicializar com um default (./www)
    client_max_body_size: unsigned long
        -> inicializar com um default (1048576 = 1MB)
    index: uma string (ex: index.html)
        -> inicializar com um default (index.html)
    error_pages: std::map(codigo erro + caminho da pagina)
    locations: vetor da classe Location
}

- Location: Serve para guardar as regras de cada rota específica
{
    path: string (ex: / ou /uploads)
    root: string (ex: /var/www/html)
    allowed_methods: vetor de strings (GET, POST, DELETE)
    autoindex: bool (true ou false)
    cgi_ext: string (ex: .py)
    cgi_path: string (ex: /usr/bin/python3)
}

- Client: Serve para guardar a ficha de cada cliente ligado
{
    struct HttpRequest {
        method: string                       
        path: string                           
        query_string: string                 
        headers: std::map(string, string) 
        body: string                        
        headers_parsed: bool
    	body_parsed: bool     
    }

    client_fd: numero (o socket do cliente)
    request_buffer: string (tudo o que vai chegando do recv)
    response_buffer: string (a resposta que vai saindo para o send)
    state: um numero ou enum (READING_HEADER, READING_BODY, WRITING)
    server: um ponteiro para a classe ServerConfig (ligação direta ao seu servidor)
		-> ajuda a saber tudo sobre o server que esta ligado
    request: HttpRequest (a variável que guarda os dados limpos do Hugo)
}

- ServerManager: Serve para gerir todo o programa e o loop do poll
{
    pollfds: um vetor de structs de pollfd
    servers: um std::vector -> ServerConfig
        -> O vetor com todas as configurações lidas pelo Hugo 
    clients: uma std::map -> client_fd, toda a info do cliente (classe Client)
        -> sabe o estado de cada cliente e as regras do seu respetivo server
}

Ação:
- ServerConfig
    -> Hugo, lê o ficheiro, cria objetos para cada server e coloca todos no vetor 'servers' em ServerManager.

- ServerManager
    -> Configura e liga todos os servidores do vetor (abre os sockets de escuta).
    -> Preenche o socket_fd de cada server no vetor servers
    -> Conforme o poll deteta novas conexões nos sockets dos servidores, fazer accept(), cria o objeto Client e adiciona ao map de clients.
		Ou seja, para cada fd gerado no accept associa um objeto Client.
    -> Recebe o request em bruto diretamente para o 'request_buffer' do cliente correspondente.

- Hugo, vai fazer parsing em duas fase:
	-> FASE1: parsing do header
	-> FASE2: parsing do body
		-> pode vir em chunks
    
- Carlos, com os dados da struct validados, dá o handle do pedido (ficheiro estático ou CGI).