#ifndef MEMORIA_H_
#define MEMORIA_H_
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>

typedef struct{
	int nro_pagina; //pasarlo a memoria como size_t
	int nro_marco; //pasarlo a memoria como size_t
}fila_tabla_de_paginas; //las tablas de paginas no son tablas, son listas, es mas facil de manejar
typedef struct {
	int pid;
	t_list* tabla_de_paginas;
	t_list* instrucciones;
} t_proceso;//para armar mi lista de procesos en memoria

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
//void iniciar_memoria_contigua();

//MUTEX
void inicializar_semaforos();
pthread_mutex_t mutex_lista_procesos; 
pthread_mutex_t mutex_frames_array; 

/*
char * user_space;
t_bitarray * frames_array;
char * bitmap;
*/

//2do checkpoint
int puerto_escucha;
int tam_memoria;
int tam_pagina; 
char* path_instrucciones;
int retardo_respuesta;
/**********/
char* memoriaPrincipal;
t_list* lista_de_procesos;// esta sera una lista de procesos, entonces cada nodo es un proceso(con un pid y una sublista de instrucciones)
char* server_name = "SOY UN CLIENTE";
t_list * leer_pseudocodigo(char* ruta);
int server_escuchar();
static void procesar_clientes(void* void_args);
cod_instruccion instruccion_to_enum(char* instruccion);
void iniciar_proceso_a_pedido_de_Kernel(char* path, int pid, int socket_kernel);
t_solicitud_instruccion* recv_solicitar_instruccion(int fd);
t_linea_instruccion* buscar_instruccion(int pid, int program_counter);
void send_proxima_instruccion(int filedescriptor, t_linea_instruccion *instruccion);
void procesar_pedido_instruccion(int socket_cpu);
void instruccion_destroyer(t_linea_instruccion* instruccion);
int dividir_y_redondear_hacia_arriba(int a,int b);
int nro_de_marco_libre();
bool es_proceso_con_pid(void * un_pid);
int bitsToBytes(int bits);

int cant_marcos;
int iniciarMemoria();
int iniciarPaginacion();
int idGlobal;
char * data;
char * frames_array;
char * asignarMemoriaBits(int bits);
#endif
