#ifndef ENTRADASALIDA_ENTRADASALIDA_H
#define ENTRADASALIDA_ENTRADASALIDA_H

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>


typedef struct{
    t_config * metadata;
    char* nombre_archivo;
    int bloque_inicial;
    int tamanio_archivo;
}fcb;


int fd_conexion_kernel;//socket para enviar a kernel
int fd_conexion_memoria;//socket para enviar a memoria

char* ip_kernel;
char* puerto_kernel;
char* ip_memoria;
char* puerto_memoria;
int tiempo_unidad_trabajo;
char* path_base_dialfs;
int block_size;
int block_count;
int retraso_compactacion;
char* path_bitmap;
char* path_bloques;

int tamanio_bitmap;
int tamanio_bloques;
void* buffer_bitmap;
void* buffer_bloques;
t_bitarray* bitmap; 

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
char * recibir_unidades_de_tiempo();

void atender_GENERICA();
void atender_STDIN();
void atender_STDOUT();
void atender_DIALFS();

void atender_DIALFS();
void inicializar_archivos();
void abrir_bitmap();
void abrir_archivo_bloques();
void escribir_metadata(char * name_file, int nro_bloque,int tamanio_archivo);
int leer_metadata(char * name_file, char *key);
void avisar_operacion_realizada_kernel();


void create_file(char * name_file);
void delete_file(char * name_file);
void truncate_file(char * name_file,uint32_t nuevo_tamanio);
void read_file(char* nombre_archivo,uint32_t tamanio_lectura,uint32_t puntero_archivo,t_list * traducciones);
void write_file(char* nombre_archivo, uint32_t tamanio_escritura,uint32_t puntero_archivo,t_list * traducciones);


int contar_digitos(int numero);
char * intTOString(int numero);
//mover a protocolo asi kernel lo conoce tambien


#endif
