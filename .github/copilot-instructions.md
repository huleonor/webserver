# Webserv — Contexto do Projeto

## O que é
Projeto da 42: servidor HTTP em C++ 98, testável com browser real.
O utilizador quer fazer o código ele próprio — Claude/Copilot guia, não escreve.

## Regras técnicas obrigatórias
- C++ 98 (`-std=c++98`, `-Wall -Wextra -Werror`)
- Non-blocking I/O em todos os sockets — nunca `read`/`write` sem passar pelo `poll`
- Um único `poll()` (ou `kqueue` no macOS) para todos os I/O
- `fork` apenas para CGI
- Sem bibliotecas externas nem Boost
- No macOS: `fcntl()` só com `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC`
- Proibido verificar `errno` após `read`/`write`

## Funcionalidades obrigatórias
- Config file (estilo NGINX): portas, error pages, body size, rotas
- Métodos: GET, POST, DELETE
- Servir site estático
- Upload de ficheiros
- CGI (pelo menos um: PHP ou Python)
- Múltiplas portas / servidores
- Default error pages

## Plano de desenvolvimento (fases)
1. [ ] Config file parser
2. [ ] Sockets + bind + listen (múltiplas portas)
3. [ ] Event loop com kqueue (macOS)
4. [ ] HTTP request parser
5. [ ] HTTP response + servir ficheiros estáticos
6. [ ] CGI (fork + execve)
7. [ ] File upload (POST multipart)
8. [ ] Testes & stress test

## Estado atual
- Fase 0: repositório criado, ficheiros vazios

## Notas de colaboração
- O utilizador faz o código, o assistente guia
- Perguntar sempre antes de implementar
- Preferência por macOS (`kqueue` em vez de `epoll`)
