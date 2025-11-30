#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

// Configurações gerais do Spa

#define NUM_RECEPCIONISTAS 2
#define NUM_ESTETICISTAS 3
#define TAMANHO_FILA_ESPERA 5 // Capacidade da sala de espera

// Estrutura de cada cliente

typedef struct {
  int id;
  int tipo_servico; // 0: Massagem, 1: Limpeza de Pele, 2: Manicure
  float valor;
} Cliente;

// Variáveis globais (Share entre as THREADS)

Cliente buffer[TAMANHO_FILA_ESPERA]; // A sala de espera (Memória Compartilhada simulada entre threads)
int in = 0;  // Índice para inserir na fila
int out = 0; // Índice para retirar da fila

// Sincronização

sem_t sem_vagas;      // Conta quantas cadeiras vazias restam
sem_t sem_clientes;   // Conta quantos clientes estão esperando
pthread_mutex_t mutex_buffer; // O trancamento para ninguém mexer na fila ao mesmo tempo

// IPC - a comunicação entre os processo

int pipe_financeiro[2]; // Aqui o Pipe é: [0] para leitura, [1] para escrita

// Geração de cliente aleatório

Cliente gerar_cliente(int id) {
  Cliente c;
  c.id = id;
  c.tipo_servico = rand() % 3;

  if (c.tipo_servico == 0) { c.valor = 120.00; } // Massagem
  else if (c.tipo_servico == 1) { c.valor = 150.00; } // Limpeza
  else { c.valor = 60.00; } // Manicure
  return c;
}

// Primeira Thread: Recepcionista (produção)
void *recepcionista(void *arg) {
  int id_thread = *(int*)arg;
  int contador_clientes = 0;

  while (1) {
      //Produz um cliente (gasta um tempinho atendendo o telefone)
      sleep(rand() % 3 + 1);
      Cliente novo_cliente = gerar_cliente(id_thread * 1000 + contador_clientes++);

      //Protocolo de entrada na Região Crítica
      sem_wait(&sem_vagas); // Espera ter vaga na sala. Se tiver cheia, TRAVA aqui.
      pthread_mutex_lock(&mutex_buffer); // Tranca a porta para mexer na fila

      //Área Crítica (Insere na fila)
      buffer[in] = novo_cliente;
      printf(" Uhuuh [Recepção %d] Cliente %d chegou (Serviço %d - R$ %.2f). Fila na posição %d.\n",
             id_thread, novo_cliente.id, novo_cliente.tipo_servico, novo_cliente.valor, in);
      in = (in + 1) % TAMANHO_FILA_ESPERA; // Lógica de fila circular

      //Saída da Região Crítica
      pthread_mutex_unlock(&mutex_buffer); // Destranca a porta
      sem_post(&sem_clientes); // Avisa: "Hey po, tem mais um cliente esperando!"
    }
}

// Segunda Thread: Esteticista (consumo)
void *esteticista(void *arg) {
  int id_thread = *(int*)arg;

  while (1) {
     Cliente cliente_atendido;

     //Protocolo de espera por trabalho
     printf("  [Esteticista %d] Livre e aguardando...\n", id_thread);
     sem_wait(&sem_clientes); // Espera ter cliente. Se fila vazia, TRAVA aqui.
     pthread_mutex_lock(&mutex_buffer); // Tranca a fila para retirar

     //Área Crítica (Retira da fila)
     cliente_atendido = buffer[out];
     out = (out + 1) % TAMANHO_FILA_ESPERA;

     //Saída da Região Crítica
     pthread_mutex_unlock(&mutex_buffer);
     sem_post(&sem_vagas); // Avisa: "Liberei uma cadeira na espera!"

     //Processamento (Simula o serviço demorado)
     printf(" [Esteticista %d] Atendendo Cliente %d...\n", id_thread, cliente_atendido.id);
     sleep(rand() % 5 + 3); // Trabalha por 3 a 7 segundos

     //IPC: Envia o valor para o processo Financeiro via PIPE
     //Importante: A thread escreve no pipe que conecta ao processo filho
     write(pipe_financeiro[1], &cliente_atendido.valor, sizeof(float));
     printf(" [Esteticista %d] Cliente %d finalizado!\n", id_thread, cliente_atendido.id);
    }
}

// Main

int main() {
    srand(time(NULL));

    //Inicializa Sincronização
    sem_init(&sem_vagas, 0, TAMANHO_FILA_ESPERA); // Começa com 5 vagas livres
    sem_init(&sem_clientes, 0, 0); // Começa com 0 clientes
    pthread_mutex_init(&mutex_buffer, NULL);

    //Cria o PIPE (IPC)
    if (pipe(pipe_financeiro) == -1) {
        perror("Erro ao criar pipe");
        exit(1);
    }

    //Criação de Processos (fork)
    pid_t pid_financeiro = fork();

    if (pid_financeiro == 0) {
        //Processo filho: gerenciar o caixa
        close(pipe_financeiro[1]); //Fecha lado de escrita, pois ele só lê
        float valor_recebido;
        float saldo_total = 0.0;

        printf(" [Financeiro] Sistema de caixa iniciado.\n");

        //Loop infinito lendo do pipe
        while (read(pipe_financeiro[0], &valor_recebido, sizeof(float)) > 0) {
            saldo_total += valor_recebido;
            printf("\n--- CAIXA ATUALIZADO: +R$ %.2f | TOTAL: R$ %.2f ---\n\n", valor_recebido, saldo_total);
        }
        exit(0);
    }
    else {

        //Processo pai: gerenciar o SPA
        close(pipe_financeiro[0]); //Fecha lado de leitura, as threads vão escrever no [1]

        pthread_t rec[NUM_RECEPCIONISTAS];
        pthread_t est[NUM_ESTETICISTAS];
        int ids_rec[NUM_RECEPCIONISTAS];
        int ids_est[NUM_ESTETICISTAS];

        //Cria as Threads
        for (int i = 0; i < NUM_RECEPCIONISTAS; i++) {
            ids_rec[i] = i + 1;
            pthread_create(&rec[i], NULL, recepcionista, &ids_rec[i]);
        }

        for (int i = 0; i < NUM_ESTETICISTAS; i++) {
            ids_est[i] = i + 1;
            pthread_create(&est[i], NULL, esteticista, &ids_est[i]);
        }

        //O pai espera infinitamente (Ctrl+C para parar)
        for (int i = 0; i < NUM_RECEPCIONISTAS; i++) pthread_join(rec[i], NULL);
        for (int i = 0; i < NUM_ESTETICISTAS; i++) pthread_join(est[i], NULL);

        //Limpeza (nunca chega aqui no loop infinito, mas é boa prática)
        sem_destroy(&sem_vagas);
        sem_destroy(&sem_clientes);
        pthread_mutex_destroy(&mutex_buffer);
    }

    return 0;
}
