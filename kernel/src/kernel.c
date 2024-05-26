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
	pthread_create(&hilo_consola, NULL, (void*)iniciar_consola, NULL); //si termina siendo solo esto lo que hace, hace falta que sea un hilo a parte?
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
    
    sem_init(&sem_multiprogramacion, 0, gradoDeMultiprogramacion);
    sem_init(&sem_procesos_new, 0, 0); // en principio nohay procesos en new, logicamente
	sem_init(&sem_procesos_ready, 0, 0); // en principio, no hay procesos en ready, logicamente
    sem_init(&sem_despachar, 0, 1); // en 1 porque primero se despacha
    sem_init(&sem_atender_rta, 0, 0); // en 0 porque segundo se atiende la rta
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

void iniciar_proceso(char *pathPasadoPorConsola){
    char* path = pathPasadoPorConsola;

    pcb* proceso_nuevo = crear_pcb();
    
    enviar_datos_proceso(path, proceso_nuevo->PID, fd_conexion_memoria); // ENVIO PATH Y PID PARA QUE CUANDO CPU PIDA MANDE PID Y PC, Y AHI MEMORIA TENGA EL PID PARA IDENTIFICAR
    push_con_mutex(cola_new,proceso_nuevo,&mutex_lista_new);
    log_info(logger_obligatorio, "Se creo el proceso %d en NEW", proceso_nuevo -> PID);    
    
    
    
}

pcb* buscar_proceso_para_finalizar(int pid_a_buscar){ 
    for (int i = 0; i<list_size(cola_exec); i++)       
    {
        pcb* proceso = list_get(cola_exec,i);
        if(proceso->PID == pid_a_buscar){
            list_remove(cola_exec,i);//ojo, capaz necesite mutex
            return proceso;
        }
    }

    return NULL;
    
}

//Esta funcion va a tener que ser revisada cuando hagamos largo plazo, ya que ahora no tiene mucho sentido seguir desarrollandola porq muy probablemente sea una abstraccion de logica de varias cosas. NO SE COMITEO
void finalizar_proceso(char* PID){
    int pid_busado = atoi(PID);
    pcb* pcb_buscado = buscar_proceso_para_finalizar(pid_busado);

    if(strcmp(string_de_estado(pcb_buscado->estado), "EXEC") == 0){
        enviar_interrupcion(EXIT_CONSOLA); //ojo, capaz necesite mutex, los fd son compartidos
    }else{
        pcb_buscado->motivo = EXIT_CONSOLA;
        cambiar_estado(pcb_buscado,EXITT);
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
        // falta
    }else if(strcmp(comando_consola[0], "INICIAR_PROCESO") == 0){
        iniciar_proceso(comando_consola[1]);
    }else if(strcmp(comando_consola[0], "FINALIZAR_PROCESO") == 0){
        finalizar_proceso(comando_consola[1]);
    }else if(strcmp(comando_consola[0] , "DETENER_PLANIFICACION") == 0){
        detener_planificacion();
    }else if(strcmp(comando_consola[0] , "INICIAR_PLANIFICAION") == 0){
        iniciar_planificacion();
    }else if(strcmp(comando_consola[0] , "MULTIPROGRAMACION") == 0){
        // falta
    }else if(strcmp(comando_consola[0] , "PROCESO_ESTADO") == 0){
        // falta
    }else{
        log_error(logger_kernel, "Nunca tendria que llegar aca por el filtro, pero si llega lo aviso por las dudas. FILTRO MAL HECHO");
        exit(EXIT_FAILURE);
    }
    
    string_array_destroy(comando_consola);
};

void iniciar_planificacion(){
    log_info(logger_kernel,"Planificaciones iniciadas");
    pthread_mutex_lock(&mutex_debe_planificar);
    debe_planificar = true; // algunos hilos solo leen la variable, pero hay dos o tres que la escriben, entonces, tiene que haber un mutex, para leerla como para escribirla, mira leer_debe_planificar_con_mutex()
    pthread_mutex_unlock(&mutex_debe_planificar);
    if(!esta_planificando){
        planificar_largo_plazo();
        iniciar_planificacion_corto_plazo();
        esta_planificando = true;
    }
}

bool leer_debe_planificar_con_mutex(){
    bool buffer;
    pthread_mutex_lock(&mutex_debe_planificar);
    buffer = debe_planificar;
    pthread_mutex_unlock(&mutex_debe_planificar);
    return buffer;
}

void detener_planificacion() {
	log_info(logger_kernel, "Pausar planificaciones");
    pthread_mutex_lock(&mutex_debe_planificar);
	debe_planificar = false; // ponerle un mutex, porque muchos hilos deben leer esta variable
    pthread_mutex_unlock(&mutex_debe_planificar);

    semaforos_destroy(); // Cierra todos los semaforos
}

void planificar_largo_plazo(){
    pthread_t hilo_ready;

    pthread_create(&hilo_ready,NULL,(void*) planificacion_procesos_ready,NULL);
    pthread_detach(hilo_ready);
}

void planificacion_procesos_ready(){
    while (esta_planificando)
    {
        sem_wait(&sem_procesos_new); // Cantidad de proceso en NEW
        pcb* pcb = pop_con_mutex(cola_new, &mutex_lista_new); //Agarramos el primero de la lista de NEW
        sem_wait(&sem_multiprogramacion); // Todavia podemos mandar procesos a ready?
        proceso_a_ready(pcb); // Mandamos el proceso a ready
        sem_post(&sem_procesos_ready); // +1 a la cantidad de procesos en ready 
    }
    
}

void proceso_a_ready(pcb* pcb){
    pthread_mutex_lock(&mutex_lista_ready);
    push_con_mutex(cola_ready, pcb, &mutex_lista_ready); //Agregamos el proceso a ready
    logger_cola_ready(); //Logger obligatorio para romper las pelotillas
    pthread_mutex_unlock(&mutex_lista_ready); 
    cambiar_estado(pcb,READY); // Cambiamos el estado dentro del PCB, no hace falta q este en seccion critica
}

//Ingreso a Ready: Cola Ready <COLA>: [<LISTA DE PIDS>]
void logger_cola_ready(){
    t_list* lista_pids = obtener_pids(cola_ready);
    char* lista = de_lista_a_string(lista_pids);        //  ESTO DE ABAJO HAY QUE PASARLO AL LEER CONFIG, PERO ESTABA EN EL DESPACHADOR
    log_info(logger_obligatorio,"Cola Ready %s: [%s]", config_get_string_value(config_kernel,"ALGORITMO_PLANIFICACION"), lista);
    list_destroy(lista_pids);
    free(lista);
};

t_list* obtener_lista_pid(t_list* lista){
    t_list* lista_pids = list_create();
    	for (int i = 0; i < list_size(lista); i++){
		pcb* pcb = list_get(lista, i);
		list_add(lista_pids, &(pcb->pid));
	}

	return lista_pids;
}

//CHAT GPT, hay que pasarlo a STRING ya que no se puede concatenar una lista con un string
char *de_lista_a_string(t_list *lista) {
    char *string = string_new(); // Inicializa una nueva cadena vacía
    for (int i = 0; i < list_size(lista); i++) { // Itera sobre cada elemento de la lista
        int *numero = (int *)list_get(lista, i); // Obtiene el elemento en la posición i y lo convierte a puntero a int
        if (i < list_size(list) - 1) { // Si no es el último elemento
            string_append_with_format(&string, "%d,", *numero); // Añade el número seguido de una coma
        } else { // Si es el último elemento
            string_append_with_format(&string, "%d", *numero); // Añade solo el número
        }
    }
    return string; // Devuelve la cadena resultante
}


void iniciar_planificacion_corto_plazo(){
    pthread_t hilo_exec;
	pthread_create(&hilo_exec, NULL, (void*) despachador, NULL);
	pthread_detach(hilo_exec);

    pthread_t hilo_vuelta_dispatch;
    pthread_create(&hilo_vuelta_dispatch,NULL,(void*) atender_vuelta_dispatch,NULL);
    pthread_detach(hilo_vuelta_dispatch);
}

void atender_vuelta_dispatch(){
    while(1){
        while(leer_debe_planificar_con_mutex()){
            sem_wait(&sem_atender_rta);//esperar a que se haya despachado un pcb
            op_code codop= recibir_operacion(fd_conexion_dispatch,logger_kernel,"CPU");//ponerse a escuchar el fd_dispatch
            t_list * lista = recibir_paquete(fd_conexion_dispatch); //Esto me parece que tendria que ir arriba del SWITCH ya que en todos los casos vamos a tomar un pcb y actualizarlo.
            pcb* pcb_actualizado = guardar_datos_del_pcb(lista);
            switch(codop){
                case PCB_ACTUALIZADO:
		        switch(pcb_actualizado -> motivo){
		            case FIN_QUANTUM: //No sabemos el nombre pero me imagino que se va a llamar asi 
		        	cambiar_estado(pcb_actualizado, READY); // -> No se si esta funcion esta creada o no, me tengo q fijar. SI esta creada pero solo cambia el estado dentro del PCB
		            	push_con_mutex(cola_ready, pcb_actualizado, &mutex_lista_ready);// No importa si es RR o VRR ya que ambos actuan igual ante el FIN DE QUANTUM, solo encolan el proceso en READY. Lo que cambia es cuando va a blockeado, en VRR hay q fijarse cuanto q le quedo
                    		sem_post(&sem_procesos_ready);
                            break;
		            case EXITO:
                    		cambiar_estado(pcb_actualizado, EXITT);
                    		push_con_mutex(cola_exit, pcb_actualizado, &mutex_lista_exit);
                    		sem_post(&sem_procesos_exit);
                            break;
			    }
               	break;
                case RECURSO:
                //recibir pcb y manejar motivo de desalojo
                break;
                case INTERFAZ:
                switch(pcb_actualizado->motivo){
                    case SOLICITAR_INTERFAZ_GENERICA: // OJO, no va a ser IO_GEN_SLEEP, va a ser el motivo de desalojo que elija sergio para estos casos, pongo esto para ir haciendo por ahora
                        char * instruccion = list_get(lista,14); //devuelve el puntero al dato del elemento de la lista original
                        char * nombre_interfaz=list_get(lista,15); //devuelve el puntero al dato del elemento de la lista original
                        char * tiempo_a_esperar=list_get(lista,16); // falta liberar si es necesario, o va a haber que meter la info en un dato pcb_block, 
                        element_interfaz * interfaz = interfaz_existe_y_esta_conectada(nombre_interfaz); //puntero al elemento original de la lista, ojo
                        if(interfaz){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                            if(generica_acepta_instruccion(instruccion)){
                                free(instruccion); //ya no me importa la instruccion, (ya se cual es)
                                free(nombre_interfaz); //ya no me importa el nombre
                                //debemos mantener la referencia a tiempo_a_esperar
                                list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                pcb_block_gen * info_de_bloqueo = malloc(sizeof(pcb_block_gen));//falta liberar
                                info_de_bloqueo->el_pcb = pcb_actualizado ; //simplemente otra referencia 
                                info_de_bloqueo->unidad_de_tiempo= tiempo_a_esperar; //simplemente otra referencia
                                cambiar_estado(pcb_actualizado,BLOCKED);
                                push_con_mutex(interfaz->cola_bloqueados,info_de_bloqueo,interfaz->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                sem_post(interfaz->sem_procesos_blocked); 
                                break;
                            }
                        }
                        //finalizar proceso
                        free(instruccion); //ya no me importa la instruccion, no se pudo hacer la instruccion
                        free(nombre_interfaz); //ya no me importa el nombre , no se pudo hacer la instruccion
                        free(tiempo_a_esperar); //ya no me importa el tiempo a esperar
                        list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                        cambiar_estado(pcb_actualizado,EXITT);
                        push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                        //OJO, tiene que haber un hilo del planificador a largo plazo que a los procesos de exit se encargue de pedirle a la memoria que libere las estructuras
                        break;
                }
                
                
                break;
            }
            sem_post(&sem_despachar);//una vez hecho todo, decirle a despachador() que puede planificar otro pcb
        }
    }
}

element_interfaz * interfaz_existe_y_esta_conectada(char * un_nombre){
    bool interfaz_con_nombre(void * una_interfaz){
	element_interfaz * una_interfaz_casteada = (element_interfaz*) una_interfaz; //no importa que este en blanco, es solo por un tema del editor de texto, en teoría debería compilarlo bien
        return (!strcmp(una_interfaz_casteada->nombre,un_nombre)); //no importa que este en blanco, es solo por un tema del editor de texto, en teoría debería compilarlo bien
    };
    pthread_mutex_lock(&mutex_lista_interfaces); 
    element_interfaz * interfaz  = list_find(interfaces_conectadas,(void*)interfaz_con_nombre); 
    pthread_mutex_unlock(&mutex_lista_interfaces);
    return interfaz;
}

bool generica_acepta_instruccion(char * instruccion){
    return !strcmp("IO_GEN_SLEEP",instruccion);
}


// ¿Como es esta coordinacion?
// Suponete que A sea "despachar" y B sea "recibir respuesta"
// queremos que la ejecucion sea A-B-A-B-A-B-A-B 
// esto, en los ejercicios de sincronizacion, era con dos semaforos, una pavada

void despachador(){
    char * algoritmo_de_planificacion = config_get_string_value(config_kernel,"ALGORITMO_PLANIFICACION");
    while(1){
        while(leer_debe_planificar_con_mutex()){
            sem_wait(&sem_despachar); //espera a que NO haya un proceso ejecutandose : la cola exec siempre tiene un solo proceso
            sem_wait(&sem_procesos_ready); //espera a que haya al menos un proceso en ready para ponerse a planificar
            pcb * pcb_a_enviar = obtener_pcb_segun_algoritmo(algoritmo_de_planificacion); // obtiene un pcb de la cola de ready
            cambiar_estado(pcb_a_enviar,EXECUTE);
            push_con_mutex(cola_exec,pcb_a_enviar,&mutex_lista_exec); // uso con mutex porque posiblemente varios hilos agregen a exec
            despachar_pcb(pcb_a_enviar);
            sem_post(&sem_atender_rta);//signal para indicar que se despacho? no se si hace falta
            if(!strcmp(algoritmo_de_planificacion,"RR")){
                manejar_quantum(pcb_a_enviar->PID);
            }
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
		case EXITT:
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
    t_paquete* paquete = crear_paquete(INTERR);
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
        path ++; //raro, que es esto?. *path es un puntero, no el path. Entonces el ++ funciona
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

void semaforos_destroy() {
	sem_close(&sem_multiprogramacion);
	sem_close(&sem_procesos_new);
	sem_close(&sem_procesos_ready);
	sem_close(&sem_despachar);
	sem_close(&sem_procesos_exit);
    sem_close(&sem_atender_rta)
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
        int * fd_conexion_io = malloc(sizeof(int)); //falta liberar, posiblemente cuando se desconecte interfaz, esta en memoria dinamica para que se pueda pasar por parametro, liberado
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
    datos_interfaz->fd_conexion_con_interfaz = fd_conexion_io;//otra a referencia al mismo malloc recibido por parametro, usar para liberar / liberado
    datos_interfaz->nombre = preguntar_nombre_interfaz((*fd_conexion_io)); //falta liberar, posiblemente cuando se desconecte interfaz, ¿Por qué es dinámico? porque en principio, no se sabe el tamaño que ocupara el nombre / liberado
    datos_interfaz->tipo = (io_type) preguntar_tipo_interfaz((*fd_conexion_io));
    datos_interfaz->cola_bloqueados=list_create();  //pcbs_bloqueados es una lista de pcb_block
    sem_init((datos_interfaz->sem_procesos_blocked),0,0); 
    pthread_mutex_init((datos_interfaz->mutex_procesos_blocked),NULL);
        
    push_con_mutex(interfaces_conectadas,datos_interfaz,&mutex_lista_interfaces); //agrega al final de la lista de interfaces

    switch (datos_interfaz->tipo){
    case GENERICA:
        atender_interfaz_generica(datos_interfaz);
        break;
    default:
        break;
    }
    
}

void atender_interfaz_generica(element_interfaz * datos_interfaz){
    while(1){ //este es para que se pueda pausar y reanudar planificacion 
        while(leer_debe_planificar_con_mutex()){ // genera algo de espera activa cuando debe_planificar = 0;
            sem_wait(datos_interfaz->sem_procesos_blocked);
            pcb_block_gen * proceso_a_atender = pop_con_mutex(datos_interfaz->cola_bloqueados,datos_interfaz->mutex_procesos_blocked);//agarra el primero de la cola de blocked 
            //contenido del paquete de instruccion
            t_paquete * paquete = crear_paquete(INSTRUCCION);//   codigo de operacion: INSTRUCCION
            agregar_a_paquete(paquete,proceso_a_atender->unidad_de_tiempo,strlen(proceso_a_atender->unidad_de_tiempo)+1);//unidad de tiempo
            if (enviar_paquete_io(paquete,*(datos_interfaz->fd_conexion_con_interfaz)) == (-1) ){ //devuelve -1 la interfaz había cerrado la conexion
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
            }else{
                int notificacion = recibir_operacion(*(datos_interfaz->fd_conexion_con_interfaz),logger_kernel,datos_interfaz->nombre);
                if(notificacion == 1 ){ // la interfaz devolvio el numero 1, entonces la operacion salio bien papa
                    procesar_vuelta_blocked_a_ready( proceso_a_atender);
                }else if(notificacion  == (-1) ){ //recibir operacion devuelve -1 en caso de que se haya desconectado la interfaz
                //INTERFAZ SE DESCONECTO: NO SE PUDO REALIZAR LA OPERACION
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
                }
            }
            eliminar_paquete(paquete);
        }
    }
}
size_t enviar_paquete_io(t_paquete* paquete, int socket_cliente) //modificacion de enviar_paquete para detectar si no se pudo enviar el paquete porque se desconecto la interfaz
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	size_t err = send(socket_cliente, a_enviar, bytes, MSG_NOSIGNAL);//MSG_NOSIGNAL PARA QUE NO CORTE LA EJECUCION DE KERNEL SI LA INTERFAZ SE DESCONECTÓ (BROKEN PIPE)

	free(a_enviar);

    return err;
}

void procesar_vuelta_blocked_a_ready(pcb_block_gen * proceso_a_atender){ //libera la estructura pcb_blocked, pero de los punteros dinamicos que contenga esta, nos encargamos antes
    char * algoritmo = config_get_string_value(config_kernel,"ALGORITMO_PLANIFICACION");
    if(!strcmp(algoritmo, "FIFO")) {
    cambiar_estado(proceso_a_atender->el_pcb,READY);
    push_con_mutex(cola_ready,proceso_a_atender->el_pcb,&mutex_lista_ready);
    free(proceso_a_atender->unidad_de_tiempo);
    free(proceso_a_atender);
	}else if(!strcmp(algoritmo, "RR")){
    cambiar_estado(proceso_a_atender->el_pcb,READY);
    push_con_mutex(cola_ready,proceso_a_atender->el_pcb,&mutex_lista_ready);
    free(proceso_a_atender->unidad_de_tiempo);
    free(proceso_a_atender);
    }else{ //todavía no analizamos VRR
                        
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


void liberar_datos_interfaz(element_interfaz * datos_interfaz){ // se invoca cuando el kernel detecte que la interfaz se desconecto, en el hilo este que va actualizando cola de bloqueados
    pthread_mutex_lock(&mutex_lista_interfaces);
    list_remove_element(interfaces_conectadas,datos_interfaz); //removimos la interfaz de la lista de interfaces conectadas
    pthread_mutex_unlock(&mutex_lista_interfaces);
    //ahora hay que liberar su informacion
    free(datos_interfaz->fd_conexion_con_interfaz);//si se esta haciendo esto, ya se le hizo close
    free(datos_interfaz->nombre);

    switch (datos_interfaz->tipo)
    {
    case GENERICA:
        list_iterate(datos_interfaz->cola_bloqueados,(void*)liberar_pcb_block_gen);
        list_destroy(datos_interfaz->cola_bloqueados);
        break;
    default:
        break;
    }
    
}

void liberar_pcb_block_gen(void * pcb_bloqueado){
    pcb_block_gen * pcb_bloqueado_c = (pcb_block_gen *) pcb_bloqueado;
    free(pcb_bloqueado_c->unidad_de_tiempo);
    cambiar_estado(pcb_bloqueado_c->el_pcb,EXITT);
    push_con_mutex(cola_exit,pcb_bloqueado_c->el_pcb,&mutex_lista_exit);
    //falta solicitar a memoria liberar las estructuras de ese proceso
    free(pcb_bloqueado_c); //nodo liberado, solo queda destruir la lista
}
