#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/temporal.h>
#include<string.h>
#include<commons/config.h>
#include<netdb.h>
#include<pthread.h>
#include<stdint.h>
#include<semaphore.h>
#include<unistd.h>
#include<stdbool.h>
#include<readline/readline.h>
#include<commons/bitarray.h>
#include<math.h>
/* DESALOJO */
typedef enum {
	EXITO, //finalizacion normal
	EXIT_CONSOLA, //Finalizacion por Error o por Consola
	INTERRUPCION, //Finalizacaion forzada por parte del Kernel
	FIN_QUANTUM, //Finalizacion por parte del Algoritmo
	PROCESO_ACTIVO,
	SOLICITAR_INTERFAZ_GENERICA,
	SOLICITAR_STDIN,
	SOLICITAR_STDOUT,
	SOLICITAR_WAIT,
	SOLICITAR_SIGNAL,
	RECURSO_INVALIDO,
	SIN_MEMORIA,
	INTERFAZ_INVALIDA,
	VACIO
} motivo_desalojo; //motivos de desalojo del proceso

/* INSTRUCCIONES */
typedef enum{

	SET,
	MOV_IN,
	MOV_OUT,
	SUM,
	SUB,
	JNZ,
	RESIZE,
	COPY_STRING,
	WAIT,
	SIGNAL,
	IO_GEN_SLEEP,
	IO_STDIN_READ,
	IO_STDOUT_WRITE,
	IO_FS_CREATE,
	IO_FS_DELETE,
	IO_FS_TRUNCATE,
	IO_FS_WRITE,
	IO_FS_READ,
	EXIT

} cod_instruccion; //son para entender instrucciones entre cpu y memoria

/* PCB */

typedef struct{
	uint8_t AX;
    uint8_t BX;
    uint8_t CX;
    uint8_t DX;
    uint32_t EAX;
    uint32_t EBX;
    uint32_t ECX;
    uint32_t EDX;
    uint32_t SI;
    uint32_t DI;
} registrosCPU;

typedef enum{
    NEW,
    READY,
	READY_AUX,
    EXECUTE,
    BLOCKED,
    EXITT
}estadosDeLosProcesos; //son para manejo del kernel, no deberían ser usados por ningún otro modulo

/* PBC PLAYS */
typedef struct {
	cod_instruccion instruccion;
	char* parametro1;
	char* parametro2;
	char* parametro3;
	char* parametro4;
	char* parametro5;
} t_linea_instruccion;

typedef struct {
	int PID; //numero UNICO para cada proceso
    uint32_t PC; // direccion de la siguiente instruccion a ejecutar
    int QUANTUM; // cantidad de tiempo que el proceso puede ejecutarse antes de que el so intervenga para cambiar el contexto
    motivo_desalojo motivo;
	estadosDeLosProcesos estado; //Estado en el que esta el proceso
    registrosCPU registros; //registros del cpu
	t_list* recursos_asignados; //recursos asignados
} pcb;

typedef struct {
	char* nombreRecurso;
	int instancias;
} recurso_asignado; 


/* OPERACIONES */
typedef enum {
	/* JANYEIK */
	handshakeCPU,
	handshakeKERNEL,
	handshakeIO,
	handshakeMEMORIA,

	/* TP0 */
	MENSAJE,
	PAQUETE,

	/* KERNEL envia PCB a CPU y CPU recibe */
	PCBBITO,

	// KERNEL envia a MEMORIA
	FINALIZAR_PROCESO,

	// MEMORIA a KERNEL
	OPERACION_TERMINADA,

	/* CPU envia PCB a KERNEL  */
	PCB_ACTUALIZADO,

	/************************/
	RECURSO, //cuando CPU envia PCB por wait o signal
	INTERFAZ, //cuando CPU envia PCB por interfaz generica

	CODE_PCB,
	DATOS_PROCESO,

	/* MEMORIA - CPU / CPU - MEMORIA */	
	SOLICITAR_INSTRUCCION,
	PROXIMA_INSTRUCCION,
	LECTURA_MEMORIA, //esta puede ser tambien de alguna interfaz, pero debe hacer lo mismo que si es de memoria
	ESCRITURA_MEMORIA, //esta puede ser tambien de alguna interfaz, pero debe hacer lo mismo que si es de memoria
	REAJUSTAR_TAMANIO_PROCESO,//puede ser reducir o ampliaR
	SIZE_PAGE, // memoria envia el tamaño de la pagina cargada en su respectivo config
	SOLICITUD_MARCO,
	MARCO,
	VALOR_LEIDO,
	OUTOFMEMORY,
	OK,
	COPY,

	INTERR //UNICO codigo de operacion de la conexion interrupt
}op_code; //codigos de operacion entre modulos, sirven para establecer que tipos de datos recibe el paquete



/* ESTRUCTURA QUE SE ENVIA KERNEL A MEMORIA */
typedef struct{
    char* path;
    int pid;
}t_datos_proceso;

/* ATENDIDO POR SUS CLIENTES */
typedef struct {
    int fd_conexion;
    t_log *logger_atendedor;
    char *cliente;
} args_atendedor;

void atender_conexion(void * args);
args_atendedor* crear_args(int un_fd, t_log * un_logger, char * un_cliente);

/*Operaciones de colas*/
void* pop_con_mutex(t_list* lista, pthread_mutex_t* mutex);
void push_con_mutex(t_list* lista, void * elemento,pthread_mutex_t* mutex);
int buscar_posicion_proceso(t_list* lista, int pid);
void * remove_con_mutex(t_list* lista, pthread_mutex_t* mutex, int posicion);

/* PAQUETE */
typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

/*ESTRUCTURAS INTERFACES */
typedef enum{
    GENERICA,
    STDIN,
    STDOUT,
	RECURSOVT, //agrego el VT al final porque ya hay un enum llamado "RECURSO"
    DIALFS
}vuelta_type;

typedef enum{
    INFORMAR_NOMBRE,
    INFORMAR_TIPO,
    INSTRUCCION
}op_kernel_a_io;

typedef enum{
    NOMBRE_INFORMADO,
    TIPO_INFORMADO,
    INSTRUCCION_REALIZADA
}op_io_a_kernel;

/* TLB */
typedef struct{
	int pid;
	int num_pag;
	int marco;
}nodo_tlb;

typedef struct
{
	int direccion_fisica; // recordar que: la direccion fisica ya incluye al offset
	int bytes;
}nodo_lectura_escritura; //para que sirve? es la unidad mas basica de escritura o lectura, no hace falta mas desgloce: leer "bytes" bytes desde la direccion fisica, o escribir "bytes" bytes desde la direccion fisica

/* ESTRUCTURA PARA EL ENVIO DE DIRECCIONES FISICAS DE cpu AL kernell*/

typedef struct {

	uint32_t direccion_fisica; // recordar que: la direccion fisica ya incluye al offset
	uint32_t bytes;

} nodo_lectura_escritura; //para que sirve? es la unidad mas basica de escritura o lectura, no hace falta mas desgloce: leer "bytes" bytes desde la direccion fisica, o escribir "bytes" bytes desde la direccion fisica

void empaquetar_pcb(t_paquete* paquete, pcb* PCB,motivo_desalojo MOTIVO);
int fin_pcb(t_list* lista);
void empaquetar_recursos(t_paquete* paquete, t_list* lista_de_recursos);
t_list* desempaquetar_recursos(t_list* paquete, int cantidad);
t_paquete* crear_paquete(op_code OPERACION);
void crear_buffer(t_paquete* paquete);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void* serializar_paquete(t_paquete* paquete, int bytes);
void eliminar_paquete(t_paquete* paquete);
void iterator(char* value, t_log *logger);
void traduccion_destroyer(void * traduccion);
t_list * desempaquetar_traducciones(t_list* paquete, int index_cantidad)
/* SENDS */
void enviar_mensaje(char* mensaje, int socket_cliente);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void enviar_mensaje_de_exito(int socket, char* mensaje);
void enviar_operacion(int socket_conexion, op_code numero);
void snd_handshake(int fd_socket_cliente);
void enviar_datos_proceso(char* path,int pid,int fd_conexion);
void enviar_pcb(pcb* PCB, int un_fd, op_code OPERACION, motivo_desalojo MOTIVO, char* parametro1, char* parametro2, char* parametro3, char* parametro4, char* parametro5);
void enviar_liberar_proceso(pcb* un_pcb,int fd);
void enviar_tamanio_pagina(int fd_cpu_dispatch, int tam_pag);
void enviar_solicitud_marco(int fd_conexion_memoria,int pid, int numero_pagina);
void enviar_marco (int fd_conexion_memoria, int marco);

/* RECVS */
void recibir_mensaje(t_log* loggerServidor, int socket_cliente);
t_list* recibir_paquete(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
int recibir_operacion(int socket_cliente, t_log * unLogger , char * elQueManda);
t_linea_instruccion* recibir_proxima_instruccion(int fd_conexion);
t_datos_proceso* recibir_datos_del_proceso(int fd_kernel);
pcb* recibir_pcb(int socket);
motivo_desalojo recibir_motiv_desalojo(int fd_escucha_dispatch);
pcb* recibir_liberar_proceso(int fd);
pcb* guardar_datos_del_pcb(t_list* paquete); //usar para cuando en un paquete, vienen los datos de un pcb y otras cosas mas
int recibir_tamanio_pagina(int fd_conexion_memoria);
int recibir_marco (int fd_conexion_memoria);
uint32_t recibir_valor_leido_memoria(int fd_memoria);
void empaquetar_traducciones(t_paquete* paquete, t_list* lista_de_traducciones);

#endif
