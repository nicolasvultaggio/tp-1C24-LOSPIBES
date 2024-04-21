#ifndef MAIN_H_
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>

int fd_conexion_interrupt;
int fd_conexion_dispatch;
int fd_conexion_memoria;
int fd_escucha_dispatch;
int fd_escucha_interrupt;

char* ip_propio;
char* puerto_cpu_dispatch; 
char* puerto_cpu_interrupt; 
char* ip_memoria;
char* puerto_memoria;


t_log* logger_cpu;
t_config* config_cpu;

bool iniciar_conexiones();
void terminar_programa();
void leer__configuraciones();

#endif
