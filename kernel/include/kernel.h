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
    vuelta_type tipo;
    t_list * cola_bloqueados;
    sem_t sem_procesos_blocked; //es mas facil su manejo si su valor lo manejamos desde distintas referencias
    pthread_mutex_t mutex_procesos_blocked; // para mutua exclusion de la cola bloqueados
} element_interfaz;

typedef struct {
    pcb * el_pcb;
    char * unidad_de_tiempo; 
} pcb_block_gen ; // son elementos de las colas de pcb bloqueados

typedef struct{
    pcb * el_pcb;
    uint32_t tamanio_lectura;
    t_list * traducciones;
}pcb_block_STDIN ;


//no importa que sean exactamente iguales me importa que sean de un tipo distinto
typedef struct{
    pcb * el_pcb;
    uint32_t tamanio_escritura;
    t_list * traducciones;
}pcb_block_STDOUT;


typedef struct{
    pcb * el_pcb;
    int instruccion_fs;// CREATE/ DELETE /READ /WRITE / TRUNCATE
    uint32_t tamanio_lectura_o_escritura_memoria;
    t_list * traducciones;
    t_list * parametros;
}pcb_block_dialFS ;
typedef struct{
	char* nombreRecurso;
	int id;
	int instancias;
	t_list* cola_block_asignada;
	pthread_mutex_t mutex_asignado;
}recurso;


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
char ** recursos; 
char ** instancias;

t_list* cola_ready;
t_list* cola_ready_aux;
t_list* cola_exec;
t_list* cola_new;
t_list* cola_block_io;
t_list* cola_exit;
t_list* cola_exit_liberados;
t_list* lista_recursos;
t_list* cola_block;

int cantidad_de_recursos;
int generador_pid = 0;

sem_t sem_multiprogramacion; 
sem_t sem_procesos_new; // semaforo que marca procesos disponibles en new
sem_t sem_procesos_ready; // semaforo que marca procesos disponibles en ready (y ready_aux)
sem_t sem_despachar;
sem_t sem_procesos_exit;
sem_t sem_atender_rta;
sem_t sem_asignacion_recursos;
sem_t sem_vuelta_recursos;


pthread_mutex_t mutex_pid; 
pthread_mutex_t mutex_lista_ready;
pthread_mutex_t mutex_lista_new; 
pthread_mutex_t mutex_lista_exec; 
pthread_mutex_t mutex_lista_interfaces; 
pthread_mutex_t mutex_lista_exit;
pthread_mutex_t mutex_debe_planificar;
pthread_mutex_t mutex_envio_memoria;
pthread_mutex_t mutex_asignacion_recursos;
pthread_mutex_t mutex_cola_block; 

pcb * pcb_a_enviar ;

//MANEJO DE RECURSOS
t_list* inicializar_recursos();
int* arrayDeStrings_a_arrayDeInts(char** array_de_strings);
t_list* iniciar_recursos_en_proceso();
void manejar_wait(pcb* pcb, char* recurso_a_buscar);
recurso *buscar_recurso(recurso* recurso_a_buscar);
void agregar_recurso(char* recurso, pcb* pcb);
void manejar_signal(pcb* proceso, char* recurso_signal);
void recurso_destroy(recurso* recurso);
void liberar_recursos(pcb* proceso);
void quitar_recurso(char* recurso_a_sacar, pcb* pcb);

//no sabemos cuales haran falta todavía, pero por las dudas los declaro
t_log* logger_kernel;
t_config* config_generales;
t_config* config_prueba;
t_log * logger_obligatorio;
char* path_configuracion;

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
void finalizar_proceso(char* PID);
pcb* buscar_proceso_para_finalizar(int pid_a_buscar);

//CONSOLA
bool validacion_de_instrucciones(char* leido);
void atender_instruccion_valida(char* leido);
int es_path(char* path);
void iniciar_proceso(char* pathPasadoPorConsola);
void iniciar_planificacion();
void detener_planificacion();
void cambiar_multiprogramacion(char* nuevoGrado);
void enlistar_procesos();
void ejecutar_script(char* pathPasadoPorConsola);


void planificar_largo_plazo();
void planificacion_procesos_ready();
void semaforos_destroy();
void logger_cola_ready();
char *de_lista_a_string(t_list *list);
t_list* obtener_lista_pid(t_list* lista);
void procesos_en_exit();
char* motivo_a_string(motivo_desalojo motivo);

void iniciar_planificacion_corto_plazo();
void despachador();
void atender_vuelta_dispatch();
pcb * obtener_pcb_segun_algoritmo(char * algoritmo);
bool colaVacia(t_list* colaDeEstado);
void cambiar_estado(pcb * un_pcb , estadosDeLosProcesos estado);
char* string_de_estado(estadosDeLosProcesos estado);
void manejar_quantum_RR(int pid);
void manejar_quantum_VRR(int pid);
t_temporal* cronometro;
int tiempo_transcurrido_en_cpu;
void reducir_quantum(void *args);



void escuchar_interfaces();
void procesar_conexion_interfaz(void * arg);
void despachar_pcb(pcb * un_pcb);
void proceso_a_ready(pcb *pcb);
void enviar_interrupcion(motivo_desalojo motivo,int pid);
bool leer_debe_planificar_con_mutex();
element_interfaz * interfaz_existe_y_esta_conectada(char * un_nombre);
bool generica_acepta_instruccion(char * instruccion);
bool interfaz_con_nombre(void * una_interfaz);
char * preguntar_nombre_interfaz(int un_fd);
void atender_interfaz_generica(element_interfaz * datos_interfaz);
void atender_interfaz_STDIN(element_interfaz * datos_interfaz);
void atender_interfaz_STDOUT(element_interfaz * datos_interfaz);
void atender_interfaz_dialFS(element_interfaz * datos_interfaz);
void procesar_vuelta_blocked_a_ready(void * proceso_a_atender,vuelta_type tipo);
size_t enviar_paquete_io(t_paquete* paquete, int socket_cliente);
void liberar_pcb_block_gen(void * pcb_bloqueado);
void liberar_pcb_block_STDIN(void * pcb_bloqueado);
void liberar_pcb_block_STDOUT(void * pcb_bloqueado);
void liberar_pcb_block_dialFS(void * pcb_bloqueado);
void liberar_datos_interfaz(element_interfaz * datos_interfaz);

bool STDIN_acepta_instruccion(char * instruccion);
bool STDOUT_acepta_instruccion(char * instruccion);
bool dialFS_acepta_instruccion(char * instruccion);

int simulacion_wait(pcb* proceso, char* recurso_wait);//es para saber si se bloquearía o no el proceso al hacerle wait
int simulacion_signal(pcb* proceso, char* recurso_signal);


bool debe_planificar;
bool esta_planificando;


#endif
