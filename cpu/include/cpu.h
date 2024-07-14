#ifndef MAIN_H_
#define MAIN_H

/* BIBLIOTECAS */
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>

/* VARIABLES DE LOG y CONFIG */
t_log* logger_cpu;
t_config* config_generales;
t_config* config_prueba;
pcb* PCB;

t_list * lista_interrupciones;

typedef struct{
	int pid;
	motivo_desalojo motivo;
}element_interrupcion;

/* TLB */
int MAX_TLB_ENTRY;
int TAM_PAGINA;
t_list* translation_lookaside_buffer;
pthread_mutex_t mutex_tlb;
pthread_mutex_t mutex_lista_interrupciones;
/* SEMAFOROS */
pthread_mutex_t mutex_motivo_x_consola; //what este para que es
sem_t sem_recibir_pcb;
sem_t sem_execute;
sem_t sem_interrupcion;


char* path_configuracion;
/* PARAMETROS CONFIG */
char* ip_propio;
char* puerto_cpu_dispatch; 
char* puerto_cpu_interrupt; 
char* ip_memoria;
char* puerto_memoria;
char* cantidad_entradas_tlb;
char* algoritmo_tlb;

/*Puntero para atender interrupciones*/
//motivo_desalojo* interrupcion_actual;

/* VARIABLES DE CONEXION */
int fd_conexion_memoria;
int fd_escucha_dispatch;
int fd_escucha_interrupt;
int fd_cpu_dispatch;
int fd_cpu_interrupt;

/*FLAGS VIEJOS PARA MANEJO DE INTERRUPCION*/

//todas estas flags tendran que actualizarse cada vez que se ejecuta una intstruccion
//bool es_exit; //siempre mofificar
//bool es_bloqueante; //modificar siempre que es_exit = false
//bool error_memoria; // modificar solo en mov in y mov out
//bool es_wait; //modificar si se pone a bloqueante = true
//bool cambio_proceso_wait; // modificar si se pone a es_wait = true
//bool es_resize; //modificar si se pone bloqueante = true
//bool resize_desalojo_outofmemory; //modificar si se pone es_resize = true
//bool hay_interrupcion_x_consola = false; //al inicio no tiene ninguna => false


bool hubo_desalojo;
bool wait_o_signal;
bool es_exit;

/* REGISTROS */
/*
typedef struct{
	uint32_t* pc;
    uint8_t*  ax;
	uint8_t*  bx;
	uint8_t*  cx;
	uint8_t*  dx;
	uint32_t* eax;
	uint32_t* ebx;
	uint32_t* ecx;
	uint32_t* edx;
	uint32_t* si;
	uint32_t* di;
} t_registros;
*/

/* FUNCIONES DE CPU */
void leer__configuraciones();
void inicializar_semaforos();
void dispatch();
void fetch ();

//PARA MANEJO DE DECODE
void decode (t_linea_instruccion* instruccion, pcb* PCB);
t_linea_instruccion* prox_instruccion(int pid, int program_counter);
void solicitar_proxima_instruccion( int pid, int program_counter);

/* EXECUTE Ejecutar instrucciones */
void ejecutar_set( char* registro, char* valor);
void ejecutar_mov_in(char* DATOS, char* DIRECCION);
void ejecutar_mov_out(char* DATOS, char* DIRECCION);
void ejecutar_sum( char* destinoregistro, char* origenregistro);
void ejecutar_sub( char* destinoregistro, char* origenregistro);
void ejecutar_jnz( char* registro, char* instruccion);
void ejecutar_resize(char* tamanio);
void ejecutar_copy_string( char* tamanio);
void ejecutar_wait(char* nombre_recurso);
void ejecutar_signal( char* nombre_recurso);
void ejecutar_io_gen_sleep( char* instruccion, char* interfaz, char* unidad_de_tiempo);
void ejecutar_io_stdin_read(char * nombre_interfaz, char * registro_direccion, char * registro_tamanio);
void ejecutar_io_stdout_write(char * nombre_interfaz, char * registro_direccion, char * registro_tamanio);
void ejecutar_io_fs_create(char * nombre_interfaz,char * nombre_archivo);
void ejecutar_io_fs_delete(char * nombre_interfaz,char * nombre_archivo);
void ejecutar_io_fs_truncate(char * nombre_interfaz,char * nombre_archivo,char * registro_tamanio);
void ejecutar_io_fs_write(char * nombre_interfaz,char * nombre_archivo,char * registro_direccion,char * registro_tamanio , char * registro_puntero_archivo);
void ejecutar_io_fs_read(char * nombre_interfaz,char * nombre_archivo,char * registro_direccion,char * registro_tamanio , char * registro_puntero_archivo);
void ejecutar_exit();
void ejecutar_error();

//MANEJO DE REGISTROS
size_t size_registro(enum_reg_cpu registro);
enum_reg_cpu registro_to_enum(char * registro);
void * capturar_registro(enum_reg_cpu registro);

//MANEJO DE INTERRUPCIONES
void interrupcion();
bool encontrar_interrupcion_por_fin_de_consola(void* elemento);
bool encontrar_interrupcion_por_fin_de_quantum(void* elemento);
element_interrupcion * recibir_motiv_desalojo(int fd_escucha_interrupt);
element_interrupcion * seleccionar_interrupcion();
void check_interrupt();

//MANEJO DE TRADUCCIONES
t_list * obtener_traducciones(uint32_t direccion_logica_i, uint32_t tamanio_a_leer );
//MMU
void inicializar_tlb();
nodo_tlb * administrar_tlb( int PID, int numero_pagina, int marco);
uint32_t MMU( uint32_t direccion_logica);
int solicitar_frame_memory(int numero_pagina);
bool es_entrada_TLB_de_PID(void * un_nodo_tlb );
void agregar_entrada_tlb(t_list* TLB, nodo_tlb * info_proceso_memoria, pthread_mutex_t* mutex, int PID, int numero_pagina, int marco);
void verificar_tamanio_tlb(t_list* TLB, pthread_mutex_t* mutex);
int consultar_tlb(int PID, int numero_pagina);

//PETICIONES A MEMORIA
void solicitar_tamanio_pagina();
int terminar_programa();
#endif
