#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>

int fd_escucha_kernel;
int fd_conexion_dispatch;
int fd_conexion_interrupt;
int fd_conexion_io;
int fd_conexion_memoria;

char * puerto_propio; 
char * puerto_memoria;
char * ip_cpu; 
char * ip_memoria;
char * puerto_cpu_dispatch ;
char * puerto_cpu_interrupt ;

t_log* logger_kernel;
t_config* config_kernel;

bool iniciar_conexiones();
void terminar_programa();
void leer_configuraciones();


#endif