#include <../../kernel/include/kernel.h>

int main(int argc, char* argv[]) {
    
    decir_hola("KERNEL");

    logger_kernel = log_create("kernel_logs.log","kernel",1,LOG_LEVEL_INFO);
    logger_obligatorio = log_create("../../utils/logs_obligatorios.log","Kernel",1,LOG_LEVEL_INFO); // todos los modulos modifican este archivo de logger, por eso en utils
    config_kernel = config_create("./kernel.config");
    
    leer_configuraciones();
    inicializar_semaforos();
    
    if(!crear_conexiones()){
        log_info(logger_kernel, "No se pudo establecer correctamente alguna conexion.");
        terminar_programa();
        return EXIT_FAILURE;
    }

    iniciar_colas_de_estados();
    iniciar_escucha_io(); 
    

    pthread_t hilo_consola;
	pthread_create(&hilo_consola, NULL, (void*)iniciar_consola, NULL);
	pthread_join(hilo_consola, NULL);


    terminar_programa();
    return 0;
}

bool crear_conexiones(){

    fd_conexion_dispatch = crear_conexion(ip_cpu,puerto_cpu_dispatch, logger_kernel, "KERNEL");
    fd_conexion_interrupt = crear_conexion(ip_cpu, puerto_cpu_interrupt, logger_kernel, "KERNEL");
    fd_conexion_memoria = crear_conexion(ip_memoria, puerto_memoria, logger_kernel, "KERNEL");
    
    return fd_conexion_dispatch != -1 && fd_conexion_interrupt != -1 && fd_conexion_memoria != -1;
}

void inicializar_semaforos(){
    pthread_mutex_init(&mutex_lista_new, NULL);
    pthread_mutex_init(&mutex_lista_exec, NULL);
    pthread_mutex_init(&mutex_lista_exit, NULL);
    pthread_mutex_init(&mutex_lista_ready, NULL);
    pthread_mutex_init(&mutex_lista_interfaces, NULL);
    pthread_mutex_init(&mutex_debe_planificar, NULL);
    

    sem_init(&sem_procesos_new, 0, 0); // en principio nohay procesos en new, logicamente
	sem_init(&sem_procesos_ready, 0, 0); // en principio, no hay procesos en ready, logicamente
    sem_init(&sem_proceso_exec, 0, 1); //en principio, si, hay que ejecutar, porque es el primero proceso
}

void leer_configuraciones(){
    puerto_propio = config_get_string_value(config_kernel,"PUERTO_PROPIO");
    ip_memoria = config_get_string_value(config_kernel,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_kernel,"PUERTO_MEMORIA");
    ip_cpu= config_get_string_value(config_kernel,"IP_CPU");
    puerto_cpu_dispatch = config_get_string_value(config_kernel,"PUERTO_CPU_DISPATCH");
    puerto_cpu_interrupt = config_get_string_value(config_kernel,"PUERTO_CPU_INTERRUPT");
    quantum = config_get_string_value(config_kernel,"QUANTUM");
    gradoDeMultiprogramacion = config_get_string_value(config_kernel,"GRADO_MULTIPROGRAMACION");

}

void iniciar_colas_de_estados(){
    generador_pid = 1;
    cola_new = list_create();
    cola_ready = list_create();
    cola_exit = list_create();
    interfaces_conectadas= list_create();
};

void iniciar_proceso(char *arg1){
    char* path = arg1;

    pcb* proceso_nuevo = crear_pcb();
    
    enviar_datos_proceso(path, proceso_nuevo->PID, fd_conexion_memoria); // ENVIO PATH Y PID PARA QUE CUANDO CPU PIDA MANDE PID Y PC, Y AHI MEMORIA TENGA EL PID PARA IDENTIFICAR
    list_add(cola_new, proceso_nuevo);
    log_info(logger_obligatorio, "Se creo el proceso %d en NEW", proceso_nuevo -> PID);
    
    
    
}

pcb* buscar_proceso_para_finalizar(int pid_a_buscar){
    for (int i = 0; i<list_size(cola_exec); i++)
    {
        pcb* proceso = list_get(cola_exec,i);
        if(proceso->PID == pid_a_buscar){
            list_remove(cola_exec,i);
            return proceso;
        }
    }

    return NULL;
    
}

void finalizar_proceso(char* arg1){
    int pid_busado = atoi(arg1);
    pcb* pcb_buscado = buscar_proceso_para_finalizar(pid_busado);

    if(strcmp(string_de_estado(pcb_buscado->estado), "EXEC") == 0){
        enviar_interrupcion(EXIT_CONSOLA); 
    }else{
        pcb_buscado->motivo = EXIT_CONSOLA;
        push_con_mutex(cola_exit,pcb_buscado,&mutex_lista_exit);
        sem_post(&sem_procesos_exit);
    }
}

pcb *crear_pcb(){

    
    pcb *un_pcb;
    un_pcb->PID = asignar_pid();
    un_pcb->PC = 1; // porque todo proceso arranca en la instruccion 0
    un_pcb->QUANTUM = atoi(quantum);
    un_pcb->motivo = PROCESO_ACTIVO; 
    un_pcb->estado = NEW;

    un_pcb->registros.AX = 0;
    un_pcb->registros.BX = 0;
    un_pcb->registros.CX = 0;
    un_pcb->registros.DX = 0;
    un_pcb->registros.EAX = 0;
    un_pcb->registros.EBX = 0;
    un_pcb->registros.ECX = 0;
    un_pcb->registros.EDX = 0;
    un_pcb->registros.SI = 0;
    un_pcb->registros.DI = 0;

    //pcb->motivo_exit = PROCESO_ACTIVO;

    
    // ojo, cuando hagamos planificador a largo plazo puede haber semaforos


    return un_pcb;
}

// esto es mas de planificador a largo plazo, tener cuidado con los semaforos a implementar
void proceso_a_ready(){
    int cantidad_de_procesos_en_READY = list_size(cola_ready);
    int cantidad_de_procesos_en_NEW = list_size(cola_new);
    int gradoDeMulti = atoi(gradoDeMultiprogramacion);
    while(cantidad_de_procesos_en_NEW > 0 && cantidad_de_procesos_en_READY < gradoDeMulti){ //No tengo la mas puta idea si esto funciona o no, me base en las commons
        pcb *un_pcb = list_remove(cola_new,0); //ME PARECE QUE ESTO ^^^^^^^^^^^^^^^^^^^^^^^^^^^ CONVIENE ENCARARLO CON UN SEMAFORO
        list_add(cola_ready,un_pcb);
        un_pcb->estado = READY;
    }
    log_info(logger_kernel,"NO se pueden agregar mas procesos a READY");
}

int asignar_pid(){
    
    pthread_mutex_lock(&mutex_pid);
    generador_pid ++;
    pthread_mutex_unlock(&mutex_pid);

    return generador_pid;
}

void iniciar_consola(){
    char* leido;
    leido = readline(">");
    bool validacion_leido; 

    while (strcmp(leido , "\0") != 0)
    {
        validacion_leido = validacion_de_instrucciones(leido);
        if(!validacion_leido){
            log_error(logger_kernel,"Comando NO reconocido");
            free(leido);
            leido = readline(">");
            continue; // Con esto vuelve a la valdacion del while. 
        }

        atender_instruccion_valida(leido);
        free(leido);
        leido = readline(">");
    }

    free(leido);
    
}

bool validacion_de_instrucciones(char* leido){
    bool resultado_validacion = false;

    char** comando_consola = string_split(leido, " "); // retorna un ARRAY con cada palabra del string que sea separada por espacios

    if(strcmp(comando_consola[0], "EJECUTAR_SCRIPT") == 0){
        if(es_path(comando_consola[1])){ //falta validar que el tamaño del comando_consola sea de dos, porque debería estar el comando y el path nada mas, misma logica para el resto
            resultado_validacion = true;
        }
    }else if(strcmp(comando_consola[0], "INICIAR_PROCESO") == 0){
        if (comando_consola[1]){
            if (es_path(comando_consola[1])){
            resultado_validacion = true;
            log_info(logger_kernel,"Se ejecutara el proceso indicado");
        }
        }else{
            log_info(logger_kernel,"NO ingreso un PATH");
        }    
    }else if(strcmp(comando_consola[0], "FINALIZAR_PROCESO") == 0){
        resultado_validacion = true; //Aaca hatendriamos que verificar si el PID que manda existe, despues vemos como lo hacemos
    }else if(strcmp(comando_consola[0] , "DETENER_PLANIFICACION") == 0){
        resultado_validacion = true;
    }else if(strcmp(comando_consola[0] , "INICIAR_PLANIFICAION") == 0){
        resultado_validacion = true;
    }else if(strcmp(comando_consola[0] , "MULTIPROGRAMACION") == 0){
        resultado_validacion = true; //Hay que validar que el grado de multiprogramacion que manda es un int
    }else if(strcmp(comando_consola[0] , "PROCESO_ESTADO") == 0){
        resultado_validacion = true;
    }else{
        resultado_validacion = false;
    }
    string_array_destroy(comando_consola);
    return resultado_validacion;

}

void atender_instruccion_valida(char* leido){
    char** comando_consola = string_split(leido , " ");

    //A TODOS LES FALTA IMPLEMENTAR LA LOGICA, PERO DEJO EL ESQUELETO ARMADO
    if(strcmp(comando_consola[0] , "EJECUTAR_SCRIPT") == 0){

    }else if(strcmp(comando_consola[0], "INICIAR_PROCESO") == 0){
        iniciar_proceso(comando_consola[1]);
    }else if(strcmp(comando_consola[0], "FINALIZAR_PROCESO") == 0){
        finalizar_proceso(comando_consola[1]);
    }else if(strcmp(comando_consola[0] , "DETENER_PLANIFICACION") == 0){
        detener_planificacion();
    }else if(strcmp(comando_consola[0] , "INICIAR_PLANIFICAION") == 0){
        iniciar_planificacion();
    }else if(strcmp(comando_consola[0] , "MULTIPROGRAMACION") == 0){
    
    }else if(strcmp(comando_consola[0] , "PROCESO_ESTADO") == 0){

    }else{
        log_error(logger_kernel, "Nunca tendria que llegar aca por el filtro, pero si llega lo aviso por las dudas. FILTRO MAL HECHO");
        exit(EXIT_FAILURE);
    }
    
    string_array_destroy(comando_consola);
};

void iniciar_planificacion(){
    log_info(logger_kernel,"Planificaciones iniciadas");
    pthread_mutex_lock(debe_planificar);
    debe_planificar = true; // algunos hilos solo leen la variable, pero hay dos o tres que la escriben, entonces, tiene que haber un mutex, para leerla como para escribirla, mira leer_debe_planificar_con_mutex()
    pthread_mutex_unlock(debe_planificar);
    if(!esta_planificando){
        //planificar_largo_plazo();
        iniciar_planificacion_corto_plazo();
        esta_planificando = true;
    }
}

bool leer_debe_planificar_con_mutex(){
    bool buffer;
    pthread_mutex_lock(mutex_debe_planificar);
    buffer = debe_planificar;
    pthread_mutex_unlock(mutex_debe_planificar);
    return buffer
}

void detener_planificacion() {
	log_info(logger_kernel, "Pausar planificaciones");
    pthread_mutex_lock(mutex_debe_planificar);
	debe_planificar = false; // ponerle un mutex, porque muchos hilos deben leer esta variable
    pthread_mutex_unlock(mutex_debe_planificar);
}

//void planificar_largo_plazo(){

//}

void iniciar_planificacion_corto_plazo(){
    pthread_t hilo_exec;
	pthread_create(&hilo_exec, NULL, (void*) planificar_corto_plazo, NULL);
	pthread_detach(hilo_exec);
}

void planificar_corto_plazo(){
    char * algoritmo_de_planificacion = config_get_string_value(config_kernel,"ALGORITMO_PLANIFICACION");
    while(leer_debe_planificar_con_mutex()){
        sem_wait(&sem_procesos_ready);
		sem_wait(&sem_proceso_exec); //espera a que haya un proceso ejecutandose : la cola exec siempre tiene un solo proceso
        pcb * pcb_a_enviar = obtener_pcb_segun_algoritmo(algoritmo_de_planificacion); // obtiene un pcb de la cola de ready
        cambiar_estado(pcb_a_enviar,EXECUTE);
        push_con_mutex(cola_exec,pcb_a_enviar,&mutex_lista_exec); // uso con mutex porque posiblemente varios hilos agregen a exec
        despachar_pcb(pcb_a_enviar);
        //signal para indicar que se despacho? no se si hace falta
        if(!strcmp(algoritmo_de_planificacion,"RR")){
            manejar_quantum(pcb_a_enviar->PID);
        }
    }
}

pcb * obtener_pcb_segun_algoritmo(char * algoritmo){
    pcb * un_pcb;
    if(!strcmp(algoritmo, "FIFO")) {
        un_pcb = pop_con_mutex(cola_ready, &mutex_lista_ready ); 
		return un_pcb; //                                        tanto FIFO como RR elijen el primero de la cola ready
	}else if(!strcmp(algoritmo, "RR")){
        un_pcb = pop_con_mutex(cola_ready, &mutex_lista_ready); 
		return un_pcb;
    }else{ //en teoria no debería acceder aca pero bueno
        return NULL;
    }
}


void cambiar_estado(pcb * un_pcb , estadosDeLosProcesos estado){
    char* nuevo_estado = strdup(string_de_estado(estado)); // obtiene el string del numerito del estado, y STRDUP lo guarda en memoria dinamica
	char* estado_anterior = strdup(string_de_estado(un_pcb->estado)); // uso memoria dinamica porque a veces tira seg fault y no entiendo por que
	un_pcb->estado = estado;
	log_info(logger_obligatorio, "PID: %d - Estado Anterior: %s - Estado Actual: %s", un_pcb->PID, estado_anterior, nuevo_estado);
	free(nuevo_estado);
	free(estado_anterior);
}

char* string_de_estado(estadosDeLosProcesos estado){
	switch(estado){
		case NEW:
			return "NEW";
			break;
		case READY:
			return "READY";
			break;
		case EXECUTE:
			return "EXEC";
			break;
		case BLOCKED:
			return "BLOCKED";
			break;
		case EXIT:
			return "EXIT";
			break;
		default:
			return "NOT_VALID_STATUS";
		}
}

void manejar_quantum(int pid){
    //como debe realizarse paralelamente a la ejecucion del proceso, tenemos que delegarlo a un hilo
    pthread_t hilo_manejo_quantum;
	pthread_create(&hilo_manejo_quantum, NULL, (void*) reducir_quantum, (void*)(intptr_t) pid); //necesita PID para manejarle el quantum a ese proceso
	pthread_detach(hilo_manejo_quantum);//                                 (void*) porque si o si debe recibir ese tipo de dato
    //                                                                     (intptr_) para hacer que pid (un int) empiece a ocupar una cantidad de bytes necesaria para ser un puntero
}//                                                                        gracias CHATGPT                                 

void reducir_quantum(void *ppid){ // como todo hilo, es esa su especificacion
    int pid = (int)(intptr_t)ppid; // casteo de nuevo, puntero gen -> intptr_t -> int
    int quantum_como_int = atoi(quantum); // paso el numero quantum (char) a entero
    usleep(quantum_como_int); //probablemente no dure nada así
    if(!list_is_empty(cola_exec)){ // entra al if si SE SIGUE EJECTUANDO EL PROCESO
        pthread_mutex_lock(&mutex_lista_exec); // con mutex porque es caso de lectura y escritura
        pcb * pcb_retirado_de_exec = list_get(cola_exec,0); // obtiene el pcb, sin removerlo de exec
        pthread_mutex_lock(&mutex_lista_exec);
        if(pcb_retirado_de_exec->PID == pid){
            enviar_interrupcion(FIN_QUANTUM);
        }
    }
}

void enviar_interrupcion(motivo_desalojo motivo){
    t_paquete* paquete = crear_paquete(INTERRUPCION);
	agregar_a_paquete(paquete, &motivo, sizeof(motivo));
	enviar_paquete(paquete, fd_conexion_interrupt);
	eliminar_paquete(paquete);
}

//Verifico que sea path. Siempre los PATH tienen / o punto. Esta funcion verifica eso.
int es_path(char* path){
    int cantidadDeSlash = 0;
    int cantidadDePuntos = 0;

    while (*path){
        if(*path == '/'){
            cantidadDeSlash = 1;
        }else if(*path == '.'){
            cantidadDePuntos = 1;
        }
        path ++; //raro, que es esto?
    }
    return cantidadDeSlash || cantidadDePuntos;
}

void despachar_pcb(pcb * un_pcb){
    t_paquete * paquete_pcb = crear_paquete(CODE_PCB);

    agregar_a_paquete(paquete_pcb, &(un_pcb->PID), sizeof(int));
	agregar_a_paquete(paquete_pcb, &(un_pcb->PC), sizeof(int));
    agregar_a_paquete(paquete_pcb, &(un_pcb->QUANTUM), sizeof(int));
	agregar_a_paquete(paquete_pcb, &(un_pcb->estado), sizeof(estadosDeLosProcesos));

    agregar_a_paquete(paquete_pcb, &(un_pcb->registros.AX), sizeof(uint8_t));
	agregar_a_paquete(paquete_pcb, &(un_pcb->registros.BX), sizeof(uint8_t));
	agregar_a_paquete(paquete_pcb, &(un_pcb->registros.CX), sizeof(uint8_t));
	agregar_a_paquete(paquete_pcb, &(un_pcb->registros.DX), sizeof(uint8_t));
    agregar_a_paquete(paquete_pcb, &(un_pcb->registros.EAX), sizeof(uint32_t ));
	agregar_a_paquete(paquete_pcb, &(un_pcb->registros.EBX), sizeof(uint32_t ));
	agregar_a_paquete(paquete_pcb, &(un_pcb->registros.ECX), sizeof(uint32_t ));
	agregar_a_paquete(paquete_pcb, &(un_pcb->registros.EDX), sizeof(uint32_t ));
    agregar_a_paquete(paquete_pcb, &(un_pcb->registros.SI), sizeof(uint32_t ));
	agregar_a_paquete(paquete_pcb, &(un_pcb->registros.DI), sizeof(uint32_t ));
    
    enviar_paquete(paquete_pcb, fd_conexion_dispatch);

    eliminar_paquete(paquete_pcb);
    //DESPUES CPU VA A TENER QUE TENER UNA FUNCION QUE RECIBA UN PCB, ESTABLECIENDO TODAS LAS ESTRUCTURAS

}

void terminar_programa(){

    config_destroy(config_kernel);
    log_destroy(logger_kernel);   
    log_destroy(logger_obligatorio);
    liberar_conexion(fd_escucha_kernel);
}

/*
hay un hilo de kernel que esta escuchando en un while, conexiones, que se encarga de cuando se le conecta una interfaz, en un hilo detacheable le pregunta el nombre, ese hilo detach se queda bloqueado hasta que reciba el nombre de esa interfaz (por que se detachea? para que se puedan conectar otras interfaces mientras espera el nombre de la primera)
[NO] una vez recibido el nombre, ese hilo detacheado, de alguna forma, debe guardar, el nombre de la interfaz y el FD de su conexion (usar estructura de rta foro), para saber solicitarle cosas segun una instruccion, probablemente (no se) sea una lista de interfaces, que tiene que modificarse con un mutex, y ademas crear un hilo (while con semaforos) que se encargue de procesar las peticiones a esa interfaz paralelamente,(while PLANIFICACION ACTIVA) y semaforo es igual a la cantidad de procesos en la cola blocked (o sea, inicializado en 0)
[SI] una vez recibido el nombre, ese hilo detacheado, de alguna forma, debe guardar, el nombre de la interfaz y el FD de su conexion (usar estructura de rta foro), para saber solicitarle cosas segun una instruccion, probablemente (no se) sea una lista de interfaces, que tiene que modificarse con un mutex, despues quedarse en un while que espere que haya al menos uno en blocked para poder enviar instruccion a interfaz, esperar la respuesta y posteriormente, devolver a ready o cola que sea segun algoritmo
cuando el kernel, en planificacion a corto plazo (en el hilo que atiende respuesta de dispatch), recibe una peticion a una io, debe verificar si existe y si acepta la instruccion (si no cumple alguna lo manda a exit) (todo esto en el corto plazo de rta dispatch) si la cumple, manda el proceso a blocked de esa interfaz y aumenta el semaforo de cant de procesos en blocked
el while mencionado en el segundo renglon, por cada entrada por habilitacion de semaforo, agarra el primer pcb de la lista blocked, solicita  su instruccion a io, espera notificacion de esta y según el algorimo lo envía a su cola correspondiente
el hilo tiene que entender cuando se desconecte la interfaz para liberar sus estructuras de la lista
*/

void iniciar_escucha_io(){
    fd_escucha_kernel = iniciar_servidor(NULL,puerto_propio,logger_kernel,"Kernel"); // devuelve el fd_escucha para escuchar nuevas conexiones

    pthread_t hilo_io;
    pthread_create(&hilo_io,NULL,(void*)escuchar_interfaces,NULL);
    pthread_detach(hilo_io); 

}

void escuchar_interfaces(){
    int check=0;
    do{
        int * fd_conexion_io = malloc(sizeof(int)); //falta liberar, posiblemente cuando se desconecte interfaz, esta en memoria dinamica para que sea manipulable por varios hilos
        (*fd_conexion_io) = esperar_cliente(fd_escucha_kernel,logger_kernel,"IO");
        check = (*fd_conexion_io);
        //procesa conexion

        pthread_t hilo_proceso_conexion_interfaz; 
        pthread_create(&hilo_proceso_conexion_interfaz,NULL,(void*)procesar_conexion_interfaz,(void*)fd_conexion_io);
        pthread_detach(hilo_proceso_conexion_interfaz); //se detachea el protocolo de conexion para que pueda escuchar otras interfaces al mismo tiempo

    }while( check != (-1));
    
}


void procesar_conexion_interfaz(void * arg){
    int * fd_conexion_io = (int*)arg;
    element_interfaz * datos_interfaz = malloc(sizeof(element_interfaz));//falta liberar, posiblemente cuando se desconecte la interfaz
    datos_interfaz->fd_conexion_con_interfaz = fd_conexion_io;//otra a referencia al mismo malloc recibido por parametro, usar para liberar
    datos_interfaz->nombre = preguntar_nombre_interfaz((*fd_conexion_io)); //falta liberar, posiblemente cuando se desconecte interfaz, ¿Por qué es dinámico? porque en principio, no se sabe el tamaño que ocupara el nombre
    datos_interfaz->tipo = (io_type) preguntar_tipo_interfaz((*fd_conexion_io));
    datos_interfaz->cola_bloqueados=list_create();  //pcbs_bloqueados es una lista de pcb_block
    sem_init((datos_interfaz->sem_procesos_blocked),0,0); 
    pthread_mutex_init((datos_interfaz->mutex_procesos_blocked),NULL);
        
    push_con_mutex(interfaces_conectadas,datos_interfaz,mutex_lista_interfaces); //agrega al final de la lista de interfaces

    while(1){ //este es para que se pueda pausar y reanudar planificacion 
        while(leer_debe_planificar_con_mutex()){ // genera algo de espera activa cuando debe_planificar = 0;
            sem_wait(datos_interfaz->sem_procesos_blocked);
            //agarra el primero de la cola de blocked 
            //solicita la instruccion
            //espera la respuesta de la interfaz
            //segun algoritmo de planificacion, lo pone en la cola correspondiente
            //------------------------------------------------------------------------------------------------------------------------------- SEGUIR ACA
        }
    }
    
}


//PRIMERO RECIBIR CODIGO DE OPERACION, SE PUEDE ENVIAR UN CODIGO DE OPERACION SIN ENVIAR UN PAQUETE
char * preguntar_nombre_interfaz(int un_fd){
    int operacion = INFORMAR_NOMBRE;
    send(un_fd,&operacion,sizeof(int),0);
    if(recibir_operacion(un_fd,logger_kernel,"IO") == NOMBRE_INFORMADO){ //envio con codigo de operacion para poder usar funciones de commons, realmente no hace falta
    int size;
	char* buffer = recibir_buffer(&size, un_fd); 
	log_info(logger_kernel, "Nombre recibido: %s", buffer);
	return(buffer); //ojo, no se esta liberando el buffer de memoria dinamica
    }   
}

int preguntar_tipo_interfaz(int un_fd){
    int operacion = INFORMAR_TIPO;
    int tipo_recibido;
    send(un_fd,&operacion,sizeof(int),0);
    recv(un_fd,&tipo_recibido,sizeof(int),MSG_WAITALL);
    return tipo_recibido;
}


void liberar_datos_interfaz(){ // se invoca cuando el kernel detecte que la interfaz se desconecto, en el hilo este que va actualizando cola de bloqueados
//HACER FREE DE TODOS LOS ELEMENTOS DE ELEMENT_INTEFAZ QUE SEAN DINAMICOS, las funciones de commons me sirven al eliminar un elemento de una lista?NO
//con las commons obtengo la data del elemento, lo unico que libera son las estructuras para agregarlo a la lista
//despues tengo que liberar todos los datos dinamicos de ese element_interfaz (incluida la sublista)
}
