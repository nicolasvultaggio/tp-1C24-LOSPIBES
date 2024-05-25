#ifndef MEMORIA_H_
#define MEMORIA_H_
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>

typedef struct {
	int pid;
	t_list* instrucciones;
} t_listaprincipal;//para armar mi lista de procesos en memoria
typedef struct
{
	int pid;
	int program_counter;
} t_solicitud_instruccion;//lo que uso para recibir los datos que me envia CPU

int fd_escucha_memoria;
int fd_conexion_server;

char* puerto_propio;

t_log* logger_memoria;
t_config* config_memoria;


int fd_conexion_kernel;
int fd_conexion_cpu;
int fd_conexion_io;

void terminar_programa();
void leer_configuraciones();

//MUTEX
void inicializar_semaforos();
pthread_mutex_t mutex_lista_instrucciones; 



//2do checkpoint
int puerto_escucha;
int tam_memoria;
int tam_pagina;
char* path_instrucciones;
int retardo_respuesta;
/**********/
char* memoria;
t_list* proceso_instrucciones;// esta sera una lista de procesos, entonces cada nodo es un proceso(con un pid y una sublista de instrucciones)
char* server_name = "SOY UN CLIENTE";
t_list * leer_pseudocodigo(char* ruta);
int server_escuchar();
static void procesar_clientes(void* void_args);
cod_instruccion instruccion_to_enum(char* instruccion);
void iniciar_memoria_apedidodeKernel(char* path, int pid, int socket_kernel);
t_solicitud_instruccion* recv_solicitar_instruccion(int fd);
t_linea_instruccion* buscar_instruccion(int pid, int program_counter, t_list* proceso_instrucciones);
void send_proxima_instruccion(int filedescriptor, t_linea_instruccion *instruccion);
void procesar_pedido_instruuccion(int socket_cpu, t_list* proceso_instrucciones);
void instruccion_destroyer(t_linea_instruccion* instruccion);

#endif
