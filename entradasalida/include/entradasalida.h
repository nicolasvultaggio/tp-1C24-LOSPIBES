#ifndef ENTRADASALIDA_ENTRADASALIDA_H
#define ENTRADASALIDA_ENTRADASALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>

int fd_conexion_kernel;//socket para enviar a kernel
int fd_conexion_memoria;//socket para enviar a memoria

char* ip_kernel;
char* puerto_kernel;
char* ip_memoria;
char* puerto_memoria;
int tiempo_unidad_trabajo;
char* mensaje_ok_memoria;
char* mensaje_ok_kernel;

char * nombre_de_interfaz;
char * path_configuracion;
char * tipo_de_interfaz;
int type_interfaz;

t_log* logger_obligatorio;
t_log* logger_io;
t_config* config_io;

void leer_configuraciones();
void terminar_programa();
bool iniciar_conexiones();

void atender_kernel();
void informar_nombre();
void atender_instruccion();
char * recibir_instruccion_de_kernel();

void atender_GENERICA();
void atender_STDIN();
void atender_STDOUT();
void atender_DIALFS();

void avisar_operacion_realizada_kernel();


//mover a protocolo asi kernel lo conoce tambien

#endif
