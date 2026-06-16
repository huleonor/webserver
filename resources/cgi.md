CGI objetivo -> pedir algo dinamico ao server, server executa um script e devolve uma response ao cliente

Fluxo do CGI:

1. Verificar se é CGI
	- location tem dados de cgi?
	- entre no cgi handler
		- tem ponto?
		- a extension existe no config file?
		- se tudo passou
			- vou buscar o path_cgi
			- substring do path até ao fim da extension -> novo path
			- verifico se para la da extension ainda ha dados -> PATH_INFO
			- verifico o stat se tudo é valido e tem permissoes

2. Setup do CGI
    - criar 2 pipes: pipe_body, pipe_output
        - fds do pai devem ficar non blocking -> fcntl 
            - fcntl o pipe_body[1] (escrita do pai)
            - fcntl o pipe_output[0] (leitura do pai)
        - se falhar o segundo pipe, fechar os fds do primeiro e abortar
    - definir variaveis ambiente (RFC 3875)
        - vetor de strings -> alocar char** envp (terminado em NULL)
    - definir o argv para execve (char** argv)
        - argv[0] = cgi_path
        - argv[1] = script_name
        - argv[2] = NULL
    - fork
        - se falhar: fechar os 4 fds, libertar envp/argv, retornar erro 500
        
        - PAI:
            - fecha o pipe_body[0]   (leitura do filho - o pai nao usa)
            - fecha o pipe_output[1]  (escrita do filho - o pai nao usa)
            
            - SE (metodo == POST && body_length > 0):
                - Adiciona APENAS pipe_body[1] ao poll_fds com events = POLLOUT
            - SENAO (metodo == GET ou POST com body vazio):
                - fecha o pipe_body[1] IMEDIATAMENTE (manda EOF para o script acordar)
                - Adiciona o pipe_output[0] ao poll_fds com events = POLLIN (para ler a resposta)

        - FILHO:
            - fecha o pipe_body[1]   (escrita do pai - o filho nao usa)
            - fecha o pipe_output[0]  (leitura do pai - o filho nao usa)
            
            - dup2(pipe_body[0], STDIN_FILENO)
            - dup2(pipe_output[1], STDOUT_FILENO)
            
            - fecha o pipe_body[0]   (ja guardado no stdin)
            - fecha o pipe_output[1]  (ja guardado no stdout)
            
            - execve(cgi_path, argv, envp)
            - exit(1) // Se o execve falhar, o filho morre aqui controlado