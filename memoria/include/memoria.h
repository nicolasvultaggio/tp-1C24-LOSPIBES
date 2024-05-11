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
*************
char* memoria;
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
    char* path;
    int pid;
}t_datos_proceso;
typedef struct {
	int pid;
	t_list* instrucciones;
} t_proceso_instrucciones;
#endif
