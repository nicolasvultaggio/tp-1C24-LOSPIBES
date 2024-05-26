#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdlib.h>
#include <stdio.h>
#include <../../utils/include/hello.h>
#include <../../utils/include/socket.h>
#include <../../utils/include/protocolo.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <readline/readline.h>

typedef struct {
    int * fd_conexion_con_interfaz; // puntero a ese fd (otra referencia a ese fd)
    char * nombre;
    io_type tipo;
    t_list * cola_bloqueados;
    sem_t  * sem_procesos_blocked; //es mas facil su manejo si su valor lo manejamos desde distintas referencias
    pthread_mutex_t * mutex_procesos_blocked; // para mutua exclusion de la cola bloqueados
} element_interfaz;

typedef struct {
    pcb * el_pcb;
    char * unidad_de_tiempo; 
} pcb_block_gen ; // son elementos de las colas de pcb bloqueados

t_list * interfaces_conectadas;
int fd_escucha_kernel;
int fd_conexion_dispatch;
int fd_conexion_interrupt;
int fd_conexion_memoria;

char * puerto_propio; 
char * puerto_memoria;
char * ip_cpu; 
char * ip_memoria;
char * puerto_cpu_dispatch;
char * puerto_cpu_interrupt;
char * quantum;
char * gradoDeMultiprogramacion;

t_list* cola_ready;
t_list* cola_exec;
t_list* cola_new;
t_list* cola_block_io;
t_list* cola_exit;

int generador_pid = 0;

sem_t sem_procesos_new; // semaforo que marca procesos disponibles en new
sem_t sem_procesos_ready; // semaforo que marca procesos disponibles en ready
sem_t sem_despachar;
sem_t sem_procesos_exit;
sem_t sem_atender_rta;

pthread_mutex_t mutex_pid; 
pthread_mutex_t mutex_lista_ready; 
pthread_mutex_t mutex_lista_new; 
pthread_mutex_t mutex_lista_exec; 
pthread_mutex_t mutex_lista_interfaces; 
pthread_mutex_t mutex_lista_exit;
pthread_mutex_t mutex_debe_planificar;

//no sabemos cuales haran falta todavía, pero por las dudas los declaro
t_log* logger_kernel;
t_config* config_kernel;
t_log * logger_obligatorio;

//FUNCIONES DENTRO DEL MAIN
void leer_configuraciones();
void inicializar_semaforos();
bool crear_conexiones();
void terminar_programa();
void iniciar_colas_de_estados();
void iniciar_escucha_io();
void iniciar_consola();

//CREACION DEL PROCESO
pcb *crear_pcb();
int asignar_pid();

//FIN DEL PROCESO (FALTAN REVISARLAS Y TERMINARLAS)
void finalizar_proceso(char* arg1);
pcb* buscar_proceso_para_finalizar(int pid_a_buscar);

//CONSOLA
bool validacion_de_instrucciones(char* leido);
void atender_instruccion_valida(char* leido);
int es_path(char* path);
void iniciar_proceso(char* pathPasadoPorConsola);



bool iniciar_conexiones();
void iniciar_planificacion();
void detener_planificacion();
void planificar_largo_plazo();
void iniciar_planificacion_corto_plazo();
void despachador();
void atender_vuelta_dispatch();
pcb * obtener_pcb_segun_algoritmo(char * algoritmo);
void cambiar_estado(pcb * un_pcb , estadosDeLosProcesos estado);
char* string_de_estado(estadosDeLosProcesos estado);
void escuchar_interfaces();
void procesar_conexion_interfaz(void * arg);
void despachar_pcb(pcb * un_pcb);
void proceso_a_ready();
void manejar_quantum(int pid);
void reducir_quantum(void *args);
void enviar_interrupcion(motivo_desalojo motivo);
bool leer_debe_planificar_con_mutex();
element_interfaz * interfaz_existe_y_esta_conectada(char * un_nombre);
bool generica_acepta_instruccion(char * instruccion);
bool interfaz_con_nombre(void * una_interfaz);
char * preguntar_nombre_interfaz(int un_fd);
void atender_interfaz_generica(element_interfaz * datos_interfaz);
void procesar_vuelta_blocked_a_ready(pcb_block_gen * proceso_a_atender);
size_t enviar_paquete_io(t_paquete* paquete, int socket_cliente);
void liberar_pcb_block_gen(void * pcb_bloqueado);
void liberar_datos_interfaz(element_interfaz * datos_interfaz);

bool debe_planificar;
bool esta_planificando;


#endif
