#ifndef PROT#ifndef PROTOCOLO_H_
#define PROTOCOLO_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<commons/config.h>
#include<netdb.h>
#include<pthread.h>
#include<stdint.h>
#include<semaphore.h>
#include<unistd.h>
#include<stdbool.h>

/* DESALOJO */
typedef enum {
	EXITO, //finalizacion normal
	EXIT_CONSOLA, //Finalizacion por Error o por Consola
	INTERRUPCION, //Finalizacaion forzada por parte del Kernel
	FIN_QUANTUM, //Finalizacion por parte del Algoritmo
} motivo_desalojo;

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

} cod_instruccion;

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
    EXECUTE,
    BLOCKED,
    EXITT
}estadosDeLosProcesos;

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
} pcb;

/* OPERACIONES */
typedef enum {
	handshakeCPU,
	handshakeKERNEL,
	handshakeIO,
	handshakeMEMORIA,
	
	
	MENSAJE,
	PAQUETE,
	PCBBITO,
	CODE_PCB,
	DATOS_PROCESO,
	SOLICITAR_INSTRUCCION,
	PROXIMA_INSTRUCCION,
	SOLICITAR_IO_GEN_SLEEP,
	INTERR,
	ESPERA,
	SENIAL

}op_code;



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
void* pop_con_mutex(t_list* lista, pthread_mutex_t* mutex);
void push_con_mutex(t_list* lista, void * elemento,pthread_mutex_t* mutex);

/* PAQUETE */
typedef struct{
	int size;
	void* stream;
} t_buffer;

typedef struct{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

t_paquete* crear_paquete(void);
void crear_buffer(t_paquete* paquete);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void* serializar_paquete(t_paquete* paquete, int bytes);
void eliminar_paquete(t_paquete* paquete);

/* SENDS */
void enviar_mensaje(char* mensaje, int socket_cliente);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void enviar_mensaje_de_exito(int socket, char* mensaje);
void enviar_operacion(int socket_conexion, op_code numero);
void snd_handshake(int fd_socket_cliente);
void enviar_solicitud_de_instruccion(int fd, int pid, int program_counter);
void enviar_datos_proceso(char* path,int pid,int fd_conexion);
void enviar_solicitud_de_dormir_interfaz_generica(int fd_cpu_dispatch, char* interfaz, char* unidad_de_tiempo);
void enviar_pcb(pcb* PCB, int fd_escucha_dispatch);
void enviar_solicitud_de_cambio_de_estado(motivo_desalojo, int fd_escucha_dispatch);
void enviar_recurso_por_wait(char* recurso, int fd_escicha_dispatch, op_code OPERACION);
void enviar_recurso_por_signal(char* recurso, int fd_escicha_dispatch, op_code OPERACION);
void enviar_recurso(char* recurso, int fd_escicha_dispatch, op_code OPERACION);
void enviar_cambio_de_estado(motivo_desalojo motivo, int fd_escucha_dispatch);


/* RECVS */
void recibir_mensaje(t_log* loggerServidor, int socket_cliente);
t_list* recibir_paquete(int socket_cliente);
void* recibir_buffer(int* size, int socket_cliente);
int recibir_operacion(int socket_cliente, t_log * unLogger , char * elQueManda);
t_linea_instruccion* recibir_proxima_instruccion(int fd_conexion);
t_datos_proceso* recibir_datos_del_proceso(int fd_kernel);
pcb* recibir_pcb(int socket);
motivo_desalojo recibir_motiv_desalojo(int fd_escucha_dispatch);


#endif
