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
    t_config * metadata; //para usar funciones de las commons sobre el metadata, MODIFICAR SIEMPRE DESPUES DE MODIFICAR LOS DE ABAJO
    char * nombre_archivo; //para buscar archivo por nombre
    int bloque_inicial; //para facilitar escritura y lectura; SOLO MODIFICAR EN TRUNCATE
    int tamanio_archivo; 
    //COSAS IMPORTANTES SOBRE TAMANIO ARCHIVO
    //SIEMPRE son los bytes asignados desde el ultimo truncate. NO necesariamente son la cantidad bytes que escribio el proceso. Podemos hacer truncate y despues no escribir nada.
    //modificar tamanio_archivo en truncate solamente, no cuando escribimos.
    //siempre que hagamos truncate se modifica tamanio_archivo
}fcb;

t_list * lista_fcbs;

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
void escribir_metadata(t_config * metadata,char * key, int valor);
int leer_metadata(t_config * metadata, char* key);
void avisar_operacion_realizada_kernel();


void create_file(char * name_file);
void delete_file(char * name_file);
void truncate_file(char * name_file,uint32_t nuevo_tamanio);
fcb * buscar_archivo(char * name_file); //esta raro porque usa funciones anidadas
void read_file(char* nombre_archivo,uint32_t tamanio_lectura,uint32_t puntero_archivo,t_list * traducciones);
bool bytes_pertenecen_a_archivo(fcb* archivo, uint32_t posicion, uint32_t tamanio_operacion);
void write_file(char* nombre_archivo, uint32_t tamanio_escritura,uint32_t puntero_archivo,t_list * traducciones);
void escribir_archivo(fcb* archivo, uint32_t posicion_a_escribir, char* buffer);
<<<<<<< HEAD
bool agrandar(fcb* fcb_file,uint32_t nuevo_tamanio,int nueva_cant_bloques,int cant_bloques_actual);
bool achicar(fcb* fcb_file,uint32_t nuevo_tamanio,int nueva_cant_bloques, int cant_bloques_actual);
=======
off_t buscar_primer_bloque_libre();
>>>>>>> e564c11405b44df04c5e51d3e9cde218f59f8814
int contar_digitos(int numero);
char * intTOString(int numero);
//mover a protocolo asi kernel lo conoce tambien


#endif
