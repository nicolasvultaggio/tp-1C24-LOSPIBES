#ifndef MEMORIA_H_
#define MEMORIA_H_
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>

typedef struct {
	int pid;
	t_list* tabla_de_paginas;// SER UNA TABLA t_pagina LINEA 28
	t_list* instrucciones;
} t_proceso;//para armar mi lista de procesos en memoria

typedef struct
{
	int pid;
	int program_counter;
} t_solicitud_instruccion;//lo que uso para recibir los datos que me envia CPU
typedef struct
{
	int pid;//                      soy nico, no entiendo, se usa este o el de abajo? me gusta mas el de abajo
	int numero_pagina;
} pid_y_pag_de_cpu;//lo que uso para recibir los datos  para leer el PID Y NRO DE PAGINA cuando solicitan NUMERO DE MARCO
typedef struct
{
	//int pid; //este no hace falta, porque una si pertenece a una tabla de paginas definida, ya conocemos su pid
	int numpag;
	int marco;
}t_pagina;//tabla de pagina de un proceso. tiene ID del proceso, numero pagina  y el numero marco asociado a esa pagina
//LA lista t_list ya esta creada ahora cuando se asignen marcos a los procesos en con resize se debera usar esta estructura como lista de tablas

 
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
bool es_proceso_con_pid(void * un_pid);
t_proceso* buscar_proceso_en_lista(int un_pid);
int server_escuchar();
static void procesar_clientes(void* void_args);
cod_instruccion instruccion_to_enum(char* instruccion);
void iniciar_proceso_a_pedido_de_Kernel(char* path, int pid, int socket_kernel);
t_solicitud_instruccion* recv_solicitar_instruccion(int fd);
t_linea_instruccion* buscar_instruccion(int pid, int program_counter);
void send_proxima_instruccion(int filedescriptor, t_linea_instruccion *instruccion);
void procesar_pedido_instruccion(int socket_cpu);
void instruccion_destroyer(t_linea_instruccion* instruccion);
int nro_de_marco_libre();
int bitsToBytes(int bits);
void procesar_escritura_en_memoria(int cliente_socket);
void procesar_lectura_en_memoria(int cliente_socket);
void send_marco (int fd, int marco);
void procesar_reajuste_de_memoria(int un_fd);
int divide_and_ceil(int numerator, int denominator);
//te devuelve el puntero al proceso en la lista de procesos
void acortar_tamanio_proceso(int un_fd,t_proceso * proceso_reajustado,int diferencia_de_paginas);
void aumentar_tamanio_proceso(int un_fd,t_proceso * proceso_reajustado,int diferencia_de_paginas);

int cant_marcos;
int iniciarMemoria();
int iniciarPaginacion();
int idGlobal;
char * data;
t_bitarray* frames_array;
char * asignarMemoriaBits(int bits);
t_list tablas_de_paginas;
#endif
