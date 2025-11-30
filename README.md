# Simulador de SPA "Mãos de Fada" (Concorrência e IPC)

Este projeto é uma simulação de um sistema de atendimento de um SPA, desenvolvido em C para demonstrar conceitos avançados de Sistemas Operacionais, como **Threads**, **Sincronização** e **Comunicação entre Processos (IPC)**.

## Objetivo
Simular o fluxo de clientes (Produtores), fila de espera (Buffer Circular) e esteticistas (Consumidores), gerenciando recursos compartilhados e contabilizando o faturamento em um processo separado.

## Tecnologias e Conceitos Utilizados

O projeto utilizou as seguintes ferramentas:

1.  **Programação Concorrente (Threads POSIX):**
    * `pthread_create`: Para criar múltiplas recepcionistas e esteticistas rodando simultaneamente.
2.  **Sincronização (Semáforos e Mutex):**
    * `sem_wait` / `sem_post`: Para controlar o acesso às vagas na sala de espera (Recurso Limitado) e sinalizar novos clientes.
    * `pthread_mutex_lock`: Para garantir Exclusão Mútua ao acessar a fila de espera (Região Crítica), evitando condições de corrida.
3.  **Comunicação Entre Processos (IPC):**
    * **Pipes (`pipe()`):** Utilizado para enviar dados financeiros das threads (Processo Pai) para o gerenciador de caixa (Processo Filho).
    * **Fork (`fork()`):** Criação de um processo independente para o financeiro.

## Como Compilar e Rodar

Este projeto foi desenvolvido para rodar em ambientes Linux (Ubuntu/WSL).

### Pré-requisitos
* Compilador GCC
* Bibliotecas: `pthread` e `rt`

### Passo a Passo

1.  **Clone o repositório:**
    ```bash
    git clone https://github.com/acastro-s/Simulador-SPA-SO.git
    cd nome-do-repo
    ```

2.  **Compile o código:**
    ```bash
    gcc spa_simulador_maos_de_fada.c -o spa -lpthread -lrt
    ```

3.  **Execute:**
    ```bash
    ./spa
    ```

4.  **Para encerrar:**
    Pressione `Ctrl + C` no terminal.

## Estrutura do Código

* **Recepcionistas (Produtores):** Geram clientes aleatórios e os colocam no buffer compartilhado. Se a fila estiver cheia, aguardam (bloqueio por semáforo).
* **Esteticistas (Consumidores):** Retiram clientes da fila e processam o serviço (simulado com `sleep`). Se a fila estiver vazia, aguardam.
* **Processo Financeiro:** Processo isolado que lê continuamente de um *Pipe* e atualiza o saldo total do dia sempre que um serviço é concluído.

## Autores
Desenvolvido pela equipe de alunos: Annyele Barbosa de Castro (Matrícula: 2215300009); Edmar Couto da Silva (Mátricula: 2315300009); Jonathan Magalhães Duarte (Matrícula: 1915300013); 
Disciplina: Sistemas Operacionais Embarcados do professor Fábio Cardoso.
