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
    char ** elementos_de_instruccion; //char doble porque voy a recibir los elementos de la instruccion separados
} pcb_block ; // son elementos de las colas de pcb bloqueados

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
sem_t sem_proceso_exec;
sem_t sem_procesos_exit;

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

bool iniciar_conexiones();
void terminar_programa();
void leer_configuraciones();
void iniciar_consola();
bool validacion_de_instrucciones(char* leido);
void atender_instruccion_valida(char* leido);
void iniciar_planificacion();
void detener_planificacion();
void planificar_largo_plazo();
void iniciar_planificacion_corto_plazo();
void planificar_corto_plazo();
pcb * obtener_pcb_segun_algoritmo(char * algoritmo);
void cambiar_estado(pcb * un_pcb , estadosDeLosProcesos estado);
char* string_de_estado(estadosDeLosProcesos estado);

void iniciar_proceso(char* arg1);
void finalizar_proceso(char* arg1);
pcb* buscar_proceso_para_finalizar(int pid_a_buscar);
pcb *crear_pcb();
void despachar_pcb(pcb * un_pcb);
void iniciar_colas_de_estados();
void proceso_a_ready();
int asignar_pid();
void inicializar_semaforos();

void manejar_quantum(int pid);
void reducir_quantum(void *args);

void enviar_interrupcion(motivo_desalojo motivo);

bool crear_conexiones();

bool leer_debe_planificar_con_mutex();

bool debe_planificar;
bool esta_planificando;


#endif
