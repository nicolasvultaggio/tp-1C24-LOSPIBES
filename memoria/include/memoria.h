#ifndef MEMORIA_H_
#define MEMORIA_H_
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>

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
    IO_STDIN_READ,
    IO_STDIN_READ,
    IO_STDOUT_WRITE,
    IO_FS_CREATE,
    IO_FS_DELETE,
    IO_FS_TRUNCATE,
    IO_FS_WRITE,
    IO_FS_READ,
    EXIT,

}codigo_instrucciones;

typedef struct {
    codigo_instrucciones instruccion1;
    char* parametro1;
    char* parametro2;
    char* parametro3;
    char* parametro4;
    char* parametro5;

}t_instruccion;

typedef struct{
    int pid;
	char* path;
}t_datos_proceso;///lo que recibo de kernel
typedef struct {
	int pid;
	t_list* instrucciones;
} t_listaprincipal;//para armar mi lista de procesos en memoria
typedef struct
{
	int pid;
	int program_counter;
} t_solicitud_instruccion;//lo que uso para recibir los datos que me envia CPU

#endif
