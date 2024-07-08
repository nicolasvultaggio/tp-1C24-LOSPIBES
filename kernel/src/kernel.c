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
    pthread_mutex_init(&mutex_asignacion_recursos, NULL);
    pthread_mutex_init(&mutex_cola_block, NULL);

    sem_init(&sem_asignacion_recursos, 0, 0);
    sem_init(&sem_vuelta_recursos, 0, 0);
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
    recursos = config_get_array_value(config_kernel, "RECURSOS"); // char** porq es un array
	instancias = config_get_array_value(config_kernel, "INSTANCIAS_RECURSOS"); // char** porq es un array

}

void iniciar_colas_de_estados(){
    generador_pid = 1;
    cola_new = list_create();
    cola_ready = list_create();
    cola_ready_aux = list_create();
    cola_exit = list_create();
    interfaces_conectadas= list_create();
    cola_exit_liberados = list_create();
    cola_block = list_create();

    //ya se que la de recursos no tendria que ir aca pero fue, me sopla las bolas
    lista_recursos = inicializar_recursos();
};

void iniciar_proceso(char *pathPasadoPorConsola){
    int rta_memoria;
    char* path = pathPasadoPorConsola;

    pcb* proceso_nuevo = crear_pcb();
    
    pthread_mutex_lock(&mutex_envio_memoria); // MUTEX para que no se envie nada a memoria mientras q memoria este atendiendo otras cosas de KERNEL
    enviar_datos_proceso(path, proceso_nuevo->PID, fd_conexion_memoria); // ENVIO PATH Y PID PARA QUE CUANDO CPU PIDA MANDE PID Y PC, Y AHI MEMORIA TENGA EL PID PARA IDENTIFICAR
    recv(fd_conexion_memoria,&rta_memoria,sizeof(int),MSG_WAITALL); //(SE BLOQUEA EL HILO DE CONSOLA, HAY QUE VER QUE TAN BUENO ESTA ESO)
    pthread_mutex_unlock(&mutex_envio_memoria);
    
    push_con_mutex(cola_new,proceso_nuevo,&mutex_lista_new);
    sem_post(&sem_procesos_new);
    log_info(logger_obligatorio, "Se creo el proceso %d en NEW", proceso_nuevo -> PID);    
}


pcb* buscar_proceso_para_finalizar(int pid_a_buscar){ 
    pcb* pcbEncontrado;
    int posicionPCB = buscar_posicion_proceso(cola_new, pid_a_buscar);
    //creo que deberías parar todas las colas al mismo tiempo amigo
    if (posicionPCB != -1) {
        pcbEncontrado = remove_con_mutex(cola_new, &mutex_lista_new, posicionPCB);
    } else {
        posicionPCB = buscar_posicion_proceso(cola_ready, pid_a_buscar);
        if (posicionPCB != -1) {
            pcbEncontrado = remove_con_mutex(cola_ready, &mutex_lista_ready, posicionPCB);
        } else {
            posicionPCB = buscar_posicion_proceso(cola_ready_aux, pid_a_buscar);
            if (posicionPCB != -1) {
                pcbEncontrado = remove_con_mutex(cola_ready_aux, &mutex_lista_ready, posicionPCB);
            } else {
                posicionPCB = buscar_posicion_proceso(cola_exec, pid_a_buscar);
                if(posicionPCB != -1){
                    pcbEncontrado = remove_con_mutex(cola_exec, &mutex_lista_exec, posicionPCB);
                } else{
                posicionPCB = buscar_posicion_proceso(cola_block, pid_a_buscar);
                    if(posicionPCB != -1){
                        pcbEncontrado = remove_con_mutex(cola_block, &mutex_cola_block, posicionPCB);
                    }
                    else{
                        log_info(logger_kernel, "NO se encontro el proceso");
                    }
                }

            }
        }
    }

    return pcbEncontrado;
    
}



void finalizar_proceso(char* PID){
    int pid_busado = atoi(PID);
    pcb* pcb_buscado = buscar_proceso_para_finalizar(pid_busado);

    if(strcmp(string_de_estado(pcb_buscado->estado), "EXEC") == 0){ 
        enviar_interrupcion(EXIT_CONSOLA,pid_busado);
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
    un_pcb->PC = 1; 
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

    un_pcb->recursos_asignados = iniciar_recursos_en_proceso();

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

    if(strcmp(comando_consola[0] , "EJECUTAR_SCRIPT") == 0){
        ejecutar_script(comando_consola[1]);
    }else if(strcmp(comando_consola[0], "INICIAR_PROCESO") == 0){
        iniciar_proceso(comando_consola[1]);
    }else if(strcmp(comando_consola[0], "FINALIZAR_PROCESO") == 0){
        finalizar_proceso(comando_consola[1]);
    }else if(strcmp(comando_consola[0] , "DETENER_PLANIFICACION") == 0){
        detener_planificacion();
    }else if(strcmp(comando_consola[0] , "INICIAR_PLANIFICAION") == 0){
        iniciar_planificacion();
    }else if(strcmp(comando_consola[0] , "MULTIPROGRAMACION") == 0){
        cambiar_multiprogramacion(comando_consola[1]);
    }else if(strcmp(comando_consola[0] , "PROCESO_ESTADO") == 0){
        enlistar_procesos();
    }else{
        log_error(logger_kernel, "Nunca tendria que llegar aca por el filtro, pero si llega lo aviso por las dudas. FILTRO MAL HECHO");
        exit(EXIT_FAILURE);
    }
    
    string_array_destroy(comando_consola);
};

//ejecutar_script y atender_instruccion_valida son PRACTICAMENTE IGUALES pero uno lee de consola y el otro de un archivo
void ejecutar_script(char* pathPasadoPorConsola){
    FILE *f = fopen(pathPasadoPorConsola, "r"); // El PATH lo vamos a tener que completar
    if (f == NULL){
        log_info(logger_kernel,"Error al abrir el archivo de EJECUTAR_SCRIPT");
        return;
    }

    char buffer[256]; // Se pordia hacer con un malloc pero como se sabe que lo que se va a ingresar son los comandos por consola, nunca se va a superar este NUMERIN
    while(fgets(buffer, sizeof(buffer), f)){
        char** comandoSeparado = string_split(buffer," ");
        
        if(strcmp(comandoSeparado[0], "INICIAR_PROCESO") == 0){
            iniciar_proceso(comandoSeparado[1]);
        }else if(strcmp(comandoSeparado[0], "FINALIZAR_PROCESO") == 0){
            finalizar_proceso(comandoSeparado[1]);
        }else if(strcmp(comandoSeparado[0], "DETENER_PLANIFICACION") == 0){
            detener_planificacion();
        }else if(strcmp(comandoSeparado[0], "INICIAR_PLANIFICACION") == 0){
            iniciar_planificacion();
        }else if(strcmp(comandoSeparado[0], "MULTIPROGRAMACION") == 0){
            cambiar_multiprogramacion(comandoSeparado[1]);
        }else if(strcmp(comandoSeparado[0], "PROCESO_ESTADO") == 0){
            enlistar_procesos(comandoSeparado[1]);
        }else if(strcmp(comandoSeparado[0], "EJECUTAR_SCRIPT") == 0){ // NO creo q dentro del script venga un ejecutar script pero qcy todo puede pasar vite
            ejecutar_script(comandoSeparado[1]);
        }
        
        string_array_destroy(comandoSeparado);
    }
    fclose(f);
}

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

void cambiar_multiprogramacion(char* nuevoGrado){
    int nuevoGradoMultiprogramacion = atoi (nuevoGrado);
    int antiguoGradoMultiprogramacion = atoi(gradoDeMultiprogramacion);
    
    if(nuevoGradoMultiprogramacion  > antiguoGradoMultiprogramacion){
        for(int i=antiguoGradoMultiprogramacion ; i<nuevoGradoMultiprogramacion ; i++){
            sem_post(&sem_multiprogramacion);
        }
    }else{
        for(int i=nuevoGradoMultiprogramacion ; i<antiguoGradoMultiprogramacion; i++){
            sem_wait(&sem_multiprogramacion);
        }
    }
}

//LISTAS: EXIT
void enlistar_procesos(){
    t_list* lista_pids_new = obtener_lista_pid(cola_new);
    char* lista_new = de_lista_a_string(lista_pids_new);

    t_list* lista_pids_ready = obtener_lista_pid(cola_ready);
    char* lista_ready = de_lista_a_string(lista_pids_ready);
    t_list* lista_pids_ready_prioridad = obtener_lista_pid(cola_ready_aux);
    char* lista_ready_aux = de_lista_a_string(lista_pids_ready_prioridad);

    list_add_all(lista_ready, lista_ready_aux); //AGREGA TODOS LOS ELEMENTOS DE LA LISTA DE AUX A LA DE READY
    char* lista_ready_completa = de_lista_a_string(lista_ready); // Segun el chamaco CHAT GPT hay que volver a pasarla a strings

    t_list* lista_pids_exec = obtener_lista_pid(cola_exec);
    char* lista_exec = de_lista_a_string(lista_pids_exec);

    t_list* lista_pids_block = obtener_lista_pid(cola_block);
    char* lista_block = de_lista_a_string(lista_pids_block);

    t_list* lista_pids_exit = obtener_lista_pid(cola_exit);
    char* lista_exit = de_lista_a_string(lista_pids_exit);

    log_info(logger_obligatorio, "Estados en: \n -NEW: [%s] \n -READY: [%s] \n -EXEC: [%s] \n -BLOCK: [%s] \n -EXIT: [%s]", lista_new, lista_ready, lista_exec, lista_block, lista_exit);

    
    free(lista_new);
    free(lista_ready);
    free(lista_ready_aux);
    free(lista_exec);
    free(lista_block);
    free(lista_exit);

    list_destroy(lista_pids_new);
    list_destroy(lista_pids_ready);
    list_destroy(lista_pids_ready_prioridad);
    list_destroy(lista_pids_exec);
    list_destroy(lista_pids_block);
    list_destroy(lista_pids_exit);
}


void planificar_largo_plazo(){
    pthread_t hilo_ready;
    pthread_t hilo_exit;

    pthread_create(&hilo_ready,NULL,(void*) planificacion_procesos_ready,NULL);
    pthread_create(&hilo_exit,NULL,(void*) procesos_en_exit, NULL);

    pthread_detach(hilo_ready);
    pthread_detach(hilo_exit);
}

void procesos_en_exit(){
    pcb* pcbFinalizado;
    while(1){ // YA QUE CONSTANTEMENTE TIENE Q ESTAR VIENDO QUE LLEGUE UN PROCESO A EXIT
        while(leer_debe_planificar_con_mutex()){
        int rta_memoria;
        sem_wait(&sem_procesos_exit);
        pcbFinalizado = pop_con_mutex(cola_exit,&mutex_lista_exit);
        char* motivo_del_desalojo = motivo_a_string(pcbFinalizado->motivo);
        log_info(logger_obligatorio, "Finaliza el proceso: %d - Motivo: %s", pcbFinalizado->PID, motivo_del_desalojo);
        sem_post(&sem_multiprogramacion); // +1 a la multiprogramacion ya que hay 1 proceso menos en READY-EXEC-BLOCK
        pthread_mutex_lock(&mutex_envio_memoria);
        enviar_liberar_proceso(pcbFinalizado, fd_conexion_memoria); //OJO, si este es el unico hil
        recv(fd_conexion_memoria,&rta_memoria,sizeof(int),MSG_WAITALL);
        pthread_mutex_unlock(&mutex_envio_memoria);
        liberar_recursos(pcbFinalizado); // Esta es para que los procesos bloqueados en los recursos que tenia el pcbFinalizado se desbloqueen
        list_add(cola_exit_liberados,pcbFinalizado); //ESTAR ATENTO A SI EN UN FUTURO NECESITA MUTEX, -soy Nico: no creo, debería ser el unico que pushee a esta lista
        }
    }
}
void liberar_recursos(pcb* proceso){
    recurso* recurso_buscado = NULL;

    for(int i = 0; i<list_size(lista_recursos); i++){
        recurso_asignado* recurso_asignado = list_get(proceso->recursos_asignados, i);
        //Si el recurso asignado tiene instancias le aumenta la instancia dentro de la lista de recursos disponibles para todos los procesos
        if(recurso_asignado->instancias > 0){
            recurso_buscado = buscar_recurso(recurso_asignado->nombreRecurso);
            recurso_buscado->instancias ++;
            
            // Desbloqueo de procesos, como aumentamos 1 se desbloquea el proceso que estaba bloqueado
            if(recurso_buscado != NULL){
                pcb* pcb2 = pop_con_mutex(recurso_buscado->cola_block_asignada, &recurso_buscado->mutex_asignado);
                agregar_recurso(recurso_buscado->nombreRecurso, pcb2);
                procesar_vuelta_blocked_a_ready(pcb2, RECURSOVT);
                sem_post(&sem_procesos_ready);
            }
        }
    }
}

	


char* motivo_a_string(motivo_desalojo motivo){
    switch (motivo)
    {
    case EXITO:
        return "SUCCES";
        break;
    case EXIT_CONSOLA:
        return "INTERRUPTED_BY_USER";
        break;
    case RECURSO_INVALIDO:
        return "INVALID_RESOURCE";
        break;
    case INTERFAZ_INVALIDA:
        return "INVALID_INTERFACE";
        break;
    case SIN_MEMORIA:
        return "OUT_OF_MEMORY";
        break;


    


    default:
        return "NO CONOZCO ESE MOTIVO";
        break;
    }
};

void pcb_destroy(pcb* pcb){
    for (int i = 0; i < list_size(pcb->recursos_asignados); i++)
    {
        recurso* recurso_asignado = list_get(pcb->recursos_asignados, i);
        recurso_destroy(recurso_asignado);
    }
    list_destroy(pcb->recursos_asignados);
    free(pcb);
}
void recurso_destroy(recurso* recurso) {
    free(recurso->nombreRecurso);
    list_destroy_and_destroy_elements(recurso->cola_block_asignada, free);
    pthread_mutex_destroy(&recurso->mutex_asignado);
    free(recurso);
}


void planificacion_procesos_ready(){
    while (1)
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
    list_add(cola_ready, pcb);
    logger_cola_ready(); //Logger obligatorio para romper las pelotillas
    pthread_mutex_unlock(&mutex_lista_ready); 
    cambiar_estado(pcb,READY); // Cambiamos el estado dentro del PCB, no hace falta q este en seccion critica
}

//Ingreso a Ready: Cola Ready <COLA>: [<LISTA DE PIDS>]
void logger_cola_ready(){
    t_list* lista_pids = obtener_lista_pid(cola_ready);
    char* lista = de_lista_a_string(lista_pids);        //  ESTO DE ABAJO HAY QUE PASARLO AL LEER CONFIG, PERO ESTABA EN EL DESPACHADOR
    log_info(logger_obligatorio,"Cola Ready %s: [%s]", config_get_string_value(config_kernel,"ALGORITMO_PLANIFICACION"), lista);
    list_destroy(lista_pids);
    free(lista);
};

t_list* obtener_lista_pid(t_list* lista){
    t_list* lista_pids = list_create();
    	for (int i = 0; i < list_size(lista); i++){
		pcb* pcb = list_get(lista, i);
		list_add(lista_pids, &(pcb->PID));
	}

	return lista_pids;
}

//CHAT GPT a rolete, hay que pasarlo a STRING ya que no se puede concatenar una lista con un string
char *de_lista_a_string(t_list *lista) {
    char *string = string_new(); // Inicializa una nueva cadena vacía
    for (int i = 0; i < list_size(lista); i++) { // Itera sobre cada elemento de la lista
        int *numero = (int *)list_get(lista, i); // Obtiene el elemento en la posición i y lo convierte a puntero a int
        if (i < list_size(lista) - 1) { // Si no es el último elemento
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
            //int no_despachar=false; //habra casos en los que luego de atender el desalojo, no despachamos otro
            sem_wait(&sem_atender_rta);//esperar a que se haya despachado un pcb
            op_code codop= recibir_operacion(fd_conexion_dispatch,logger_kernel,"CPU");
            t_list * lista = recibir_paquete(fd_conexion_dispatch); 
            pcb* pcb_actualizado = guardar_datos_del_pcb(lista);
            int final_pcb = fin_pcb(lista); //Te devuelve el numero del elemento de la lista donde esta el ultimo recurso, osea del final del pcb
            switch(codop){
                case PCB_ACTUALIZADO:
		        switch(pcb_actualizado -> motivo){
		            case FIN_QUANTUM: //No sabemos el nombre pero me imagino que se va a llamar asi 
		        	    cambiar_estado(pcb_actualizado, READY);
                        pcb_actualizado->QUANTUM = atoi(quantum);//no importa el algoritmo, siempre si se interrumpe x fin de quantum el q del pcb = 0
		            	push_con_mutex(cola_ready, pcb_actualizado, &mutex_lista_ready);
                    	sem_post(&sem_procesos_ready);
                        break;
		            case EXITO:
                    	cambiar_estado(pcb_actualizado, EXITT);
                    	push_con_mutex(cola_exit, pcb_actualizado, &mutex_lista_exit);
                    	sem_post(&sem_procesos_exit);
                        break;
                    case SIN_MEMORIA: // Sergio tiene q poner este mismo motivo
                        cambiar_estado(pcb_actualizado, EXITT);
                        push_con_mutex(cola_exit, pcb_actualizado, &mutex_lista_exit);
                        sem_post(&sem_procesos_exit);
			    }
               	break;
                case RECURSO:
                switch(pcb_actualizado -> motivo){
                    int simulacion;
                    int rta_cpu;
                    case SOLICITAR_WAIT:
                        char * recurso_wait = list_get(lista , final_pcb+1);
                        int simulacion = simulacion_wait(pcb_actualizado,recurso_wait);
                        send(fd_conexion_dispatch,&simulacion,sizeof(int),NULL);
                        recv(fd_conexion_dispatch,&rta_cpu,sizeof(int),MSG_WAITALL);//aca recibimos que dice cpu despues de hacer check interrupt: esto nos determina si terminamos realizando la operacion o no
                        if(rta_cpu == EXIT_CONSOLA){ //aca descarto la operacion wait y signal
                            cambiar_estado(pcb_actualizado, EXIT_CONSOLA);
                            push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
		                    sem_post(&sem_procesos_exit);
                        }else{
                            manejar_wait(pcb_actualizado, recurso_wait);
                        }
				        free(recurso_wait);
                    break;
                    case SOLICITAR_SIGNAL:
                        char* recurso_signal = list_get(lista, final_pcb+1);
                        int simulacion = simulacion_signal(pcb_actualizado,recurso_signal);
                        send(fd_conexion_dispatch,&simulacion,sizeof(int),NULL);
                        recv(fd_conexion_dispatch,&rta_cpu,sizeof(int),MSG_WAITALL);//aca recibimos que dice cpu despues de hacer check interrupt: esto nos determina si terminamos realizando la operacion o no
                        if(rta_cpu == EXIT_CONSOLA){
                            cambiar_estado(pcb_actualizado, EXIT_CONSOLA);
                            push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
		                    sem_post(&sem_procesos_exit);
                        }else{
                            manejar_signal(pcb_actualizado, recurso_signal);
                        }  
                        free(recurso_signal);
				    break;
                }
                break;
                case INTERFAZ: //aca repito logica como loco pero sucede que me chupa la cabeza de la chota 
                //esperar respuesta de check interrupt
                if(tiempo_transcurrido_en_cpu < atoi(quantum)){ 
                    pcb_actualizado->QUANTUM = atoi(quantum) - tiempo_transcurrido_en_cpu;
                }
                //hacer lo de abajo si se lo permite CHECK_INTERRUPT
                switch(pcb_actualizado->motivo){
                    case SOLICITAR_INTERFAZ_GENERICA: 
                        char * instruccion_gen = list_get(lista,final_pcb+1); //devuelve el puntero al dato del elemento de la lista original 
                        char * nombre_interfaz_gen=list_get(lista,final_pcb+2); //devuelve el puntero al dato del elemento de la lista original
                        char * tiempo_a_esperar=list_get(lista,final_pcb+3); // falta liberar si es necesario, o va a haber que meter la info en un dato pcb_block, 
                        element_interfaz * interfaz_gen = interfaz_existe_y_esta_conectada(nombre_interfaz_gen); //puntero al elemento original de la lista, ojo
                        if(interfaz_gen){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                            if(generica_acepta_instruccion(instruccion_gen)){
                                free(instruccion_gen); //ya no me importa la instruccion, (ya se cual es)
                                free(nombre_interfaz_gen); //ya no me importa el nombre
                                //debemos mantener la referencia a tiempo_a_esperar
                                list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                pcb_block_gen * info_de_bloqueo = malloc(sizeof(pcb_block_gen));//falta liberar
                                info_de_bloqueo->el_pcb = pcb_actualizado ; //simplemente otra referencia 
                                info_de_bloqueo->unidad_de_tiempo= tiempo_a_esperar; //simplemente otra referencia
                                cambiar_estado(pcb_actualizado,BLOCKED);
                                push_con_mutex(interfaz_gen->cola_bloqueados,info_de_bloqueo,interfaz_gen->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                push_con_mutex(cola_block, pcb_actualizado, &mutex_cola_block); 
                                sem_post(interfaz_gen->sem_procesos_blocked); 
                                break;
                            }
                        }
                        //finalizar proceso
                        free(instruccion_gen); //ya no me importa la instruccion, no se pudo hacer la instruccion
                        free(nombre_interfaz_gen); //ya no me importa el nombre , no se pudo hacer la instruccion
                        free(tiempo_a_esperar); //ya no me importa el tiempo a esperar
                        list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                        cambiar_estado(pcb_actualizado,EXITT);
                        pcb_actualizado->motivo = INTERFAZ_INVALIDA; // Asi notificamos porq verga finalizo
                        push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                        sem_post(&sem_procesos_exit);
                        //OJO, tiene que haber un hilo del planificador a largo plazo que a los procesos de exit se encargue de pedirle a la memoria que libere las estructuras
                        break;
                    case SOLICITAR_STDIN:
                        char * instruccion_STDIN= list_get(lista,final_pcb+1);
                        char * nombre_interfaz_STDIN= list_get(lista,final_pcb+2);
                        uint32_t * tamanio_a_leer = list_get(lista,final_pcb+3);
                        t_list *traducciones_STDIN = desempaquetar_traducciones(lista,final_pcb+4);
                        element_interfaz * interfaz_STDIN = interfaz_existe_y_esta_conectada(nombre_interfaz_STDIN);
                         if(interfaz_STDIN){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                            if(STDIN_acepta_instruccion(instruccion_STDIN)){
                                free(instruccion_STDIN); //ya no me importa la instruccion, (ya se cual es)
                                free(nombre_interfaz_STDIN); //ya no me importa el nombre
                                list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                pcb_block_STDIN * info_de_bloqueo = malloc(sizeof(pcb_block_STDIN));//falta liberar
                                info_de_bloqueo->el_pcb = pcb_actualizado ;//simplemente otra referencia 
                                info_de_bloqueo->tamanio_lectura=*tamanio_a_leer;
                                info_de_bloqueo->traducciones = traducciones_STDIN;
                                free(tamanio_a_leer);//libero porque ya guarde su valor
                                cambiar_estado(pcb_actualizado,BLOCKED);
                                push_con_mutex(interfaz_STDIN->cola_bloqueados,info_de_bloqueo,interfaz_STDIN->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                push_con_mutex(cola_block, pcb_actualizado, &mutex_cola_block);
                                sem_post(interfaz_STDIN->sem_procesos_blocked); 
                                break;
                            }
                        }
                        //finalizar proceso
                        free(instruccion_STDIN); 
                        free(nombre_interfaz_STDIN); 
                        free(tamanio_a_leer);
                        list_destroy_and_destroy_elements(traducciones_STDIN,(void*) traduccion_destroyer);
                        list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                        cambiar_estado(pcb_actualizado,EXITT);
                        pcb_actualizado->motivo = INTERFAZ_INVALIDA; // Asi notificamos porq verga finalizo
                        push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                        sem_post(&sem_procesos_exit);
                        //OJO, tiene que haber un hilo del planificador a largo plazo que a los procesos de exit se encargue de pedirle a la memoria que libere las estructuras
                        break;
                    case SOLICITAR_STDOUT:
                        char * instruccion_STDOUT = list_get(lista,final_pcb+1); 
                        char * nombre_interfaz_STDOUT=list_get(lista,final_pcb+2); 
                        uint32_t * tamanio_de_escritura = list_get(lista,final_pcb+3); 
                        t_list *traducciones_STDOUT = desempaquetar_traducciones(lista,final_pcb+4);                        
                        element_interfaz * interfaz_STDOUT = interfaz_existe_y_esta_conectada(nombre_interfaz_STDOUT);
                         if(interfaz_STDOUT){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                            if(STDOUT_acepta_instruccion(instruccion_STDOUT)){
                                free(instruccion_STDOUT); //ya no me importa la instruccion, (ya se cual es)
                                free(nombre_interfaz_STDOUT); //ya no me importa el nombre
                                list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                pcb_block_STDOUT* info_de_bloqueo = malloc(sizeof(pcb_block_STDOUT));//falta liberar
                                info_de_bloqueo->el_pcb = pcb_actualizado ; //simplemente otra referencia 
                                info_de_bloqueo->traducciones=traducciones_STDOUT;
                                info_de_bloqueo->tamanio_escritura= *tamanio_de_escritura;
                                free(tamanio_de_escritura); //libero porque ya guarde su valor
                                cambiar_estado(pcb_actualizado,BLOCKED);
                                push_con_mutex(interfaz_STDOUT->cola_bloqueados,info_de_bloqueo,interfaz_STDOUT->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                push_con_mutex(cola_block, pcb_actualizado, &mutex_cola_block);
                                sem_post(interfaz_STDOUT->sem_procesos_blocked); 
                                break;
                            }
                        }
                        //finalizar proceso
                        free(instruccion_STDOUT); 
                        free(nombre_interfaz_STDOUT); 
                        free(tamanio_de_escritura);
                        list_destroy_and_destroy_elements(traducciones_STDOUT,(void*)traduccion_destroyer);
                        list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                        cambiar_estado(pcb_actualizado,EXITT);
                        pcb_actualizado->motivo = INTERFAZ_INVALIDA; // Asi notificamos porq verga finalizo
                        push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                        sem_post(&sem_procesos_exit);
                        //OJO, tiene que haber un hilo del planificador a largo plazo que a los procesos de exit se encargue de pedirle a la memoria que libere las estructuras
                        break;
                        case FS_CREATE: //le agrego a todo _fc al final para que compile bien
                            char * instruccion_fc = list_get(lista,final_pcb+1); //devuelve el puntero al dato del elemento de la lista original 
                            char * nombre_interfaz_fc=list_get(lista,final_pcb+2); //devuelve el puntero al dato del elemento de la lista original
                            char * nombre_archivo_fc=list_get(lista,final_pcb+3); // falta liberar si es necesario, o va a haber que meter la info en un dato pcb_block, 
                            element_interfaz * interfaz__fc = interfaz_existe_y_esta_conectada(nombre_interfaz_fc); //puntero al elemento original de la lista, ojo
                            if(interfaz__fc){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                                if(dialFS_acepta_instruccion(interfaz__fc)){
                                    free(instruccion_fc); //ya no me importa la instruccion, (ya se cual es)
                                    free(nombre_interfaz_fc); //ya no me importa el nombre
                                    //debemos mantener la referencia a tiempo_a_esperar
                                    list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                    pcb_block_dialFS* info_de_bloqueo = malloc(sizeof(pcb_block_dialFS));//falta liberar
                                    info_de_bloqueo->el_pcb = pcb_actualizado ; //simplemente otra referencia 
                                    info_de_bloqueo->instruccion_fs = FS_CREATE;
                                    info_de_bloqueo->tamanio_lectura_o_escritura_memoria=0; //no importa
                                    info_de_bloqueo->traducciones=NULL; //no importa
                                    info_de_bloqueo->parametros= list_create();
                                    list_add(info_de_bloqueo->parametros,nombre_archivo_fc);
                                    cambiar_estado(pcb_actualizado,BLOCKED);
                                    push_con_mutex(interfaz_fc->cola_bloqueados,info_de_bloqueo,interfaz_fc->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                    push_con_mutex(cola_block, pcb_actualizado, &mutex_cola_block); 
                                    sem_post(interfaz_fc->sem_procesos_blocked); 
                                    break;
                                }
                            }
                            //finalizar proceso
                            free(instruccion_fc); 
                            free(nombre_interfaz_fc); 
                            free(nombre_archivo_fc); 
                            list_destroy(lista); 
                            cambiar_estado(pcb_actualizado,EXITT);
                            pcb_actualizado->motivo = INTERFAZ_INVALIDA; // Asi notificamos porq verga finalizo
                            push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                            sem_post(&sem_procesos_exit);
                            break;
                        case FS_DELETE:
                            char * instruccion_fsd = list_get(lista,final_pcb+1); //devuelve el puntero al dato del elemento de la lista original 
                            char * nombre_interfaz_fsd=list_get(lista,final_pcb+2); //devuelve el puntero al dato del elemento de la lista original
                            char * nombre_archivo_fsd=list_get(lista,final_pcb+3); // falta liberar si es necesario, o va a haber que meter la info en un dato pcb_block, 
                            element_interfaz * interfaz_fsd = interfaz_existe_y_esta_conectada(nombre_interfaz_fsd); //puntero al elemento original de la lista, ojo
                            if(interfaz_fsd){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                                if(dialFS_acepta_instruccion(interfaz_fsd)){
                                    free(instruccion_fsd); //ya no me importa la instruccion, (ya se cual es)
                                    free(nombre_interfaz_fsd); //ya no me importa el nombre
                                    //debemos mantener la referencia a tiempo_a_esperar
                                    list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                    pcb_block_dialFS* info_de_bloqueo = malloc(sizeof(pcb_block_dialFS));//falta liberar
                                    info_de_bloqueo->el_pcb = pcb_actualizado ; //simplemente otra referencia 
                                    info_de_bloqueo->instruccion_fs = FS_DELETE;
                                    info_de_bloqueo->tamanio_lectura_o_escritura_memoria=0; //no importa
                                    info_de_bloqueo->traducciones=NULL; //no importa
                                    info_de_bloqueo->parametros= list_create();
                                    list_add(info_de_bloqueo->parametros,nombre_archivo_fsd);
                                    cambiar_estado(pcb_actualizado,BLOCKED);
                                    push_con_mutex(interfaz_fsd->cola_bloqueados,info_de_bloqueo,interfaz_fsd->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                    push_con_mutex(cola_block, pcb_actualizado, &mutex_cola_block); 
                                    sem_post(interfaz_fsd->sem_procesos_blocked); 
                                    break;
                                }
                            }
                            //finalizar proceso
                            free(instruccion_fsd); 
                            free(nombre_interfaz_fsd); 
                            free(nombre_archivo_fsd); 
                            list_destroy(lista); 
                            cambiar_estado(pcb_actualizado,EXITT);
                            pcb_actualizado->motivo = INTERFAZ_INVALIDA; // Asi notificamos porq verga finalizo
                            push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                            sem_post(&sem_procesos_exit);
                            break;
                        case FS_TRUNCATE:
                            char * instruccion_fst = list_get(lista,final_pcb+1); 
                            char * nombre_interfaz_fst=list_get(lista,final_pcb+2); 
                            char * nombre_archivo_fst=list_get(lista,final_pcb+3);
                            uint32_t * tamanio_fst = list_get(lista,final_pcb+4); 
                            element_interfaz * interfaz_fst = interfaz_existe_y_esta_conectada(nombre_interfaz_fst); //puntero al elemento original de la lista, ojo
                            if(interfaz_fst){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                                if(dialFS_acepta_instruccion(interfaz_fst)){
                                    free(instruccion_fst); //ya no me importa la instruccion, (ya se cual es)
                                    free(nombre_interfaz_fst); //ya no me importa el nombre
                                    list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                    pcb_block_dialFS* info_de_bloqueo = malloc(sizeof(pcb_block_dialFS));//falta liberar
                                    info_de_bloqueo->el_pcb = pcb_actualizado ; //simplemente otra referencia 
                                    info_de_bloqueo->instruccion_fs = FS_TRUNCATE;
                                    info_de_bloqueo->tamanio_lectura_o_escritura_memoria=0; //no importa
                                    info_de_bloqueo->traducciones=NULL; //no importa
                                    info_de_bloqueo->parametros= list_create();
                                    list_add(info_de_bloqueo->parametros,nombre_archivo_fst);
                                    list_add(info_de_bloqueo->parametros,tamanio_fst);
                                    cambiar_estado(pcb_actualizado,BLOCKED);
                                    push_con_mutex(interfaz_fst->cola_bloqueados,info_de_bloqueo,interfaz_fst->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                    push_con_mutex(cola_block, pcb_actualizado, &mutex_cola_block); 
                                    sem_post(interfaz_fst->sem_procesos_blocked); 
                                    break;
                                }
                            }
                            //finalizar proceso
                            free(instruccion_fst); 
                            free(nombre_interfaz_fst); 
                            free(nombre_archivo_fst); 
                            free(tamanio_fst); 
                            list_destroy(lista); 
                            cambiar_estado(pcb_actualizado,EXITT);
                            pcb_actualizado->motivo = INTERFAZ_INVALIDA; // Asi notificamos porq verga finalizo
                            push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                            sem_post(&sem_procesos_exit);
                            break;
                        case FS_WRITE:
                            char * instruccion_fsw = list_get(lista,final_pcb+1); 
                            char * nombre_interfaz_fsw=list_get(lista,final_pcb+2); 
                            char * nombre_archivo_fsw=list_get(lista,final_pcb+3);
                            uint32_t * tamanio_fsw = list_get(lista,final_pcb+4); 
                            uint32_t * puntero_archivo_fsw = list_get(lista,final_pcb+5); 
                            t_list * traducciones_fsw = desempaquetar_traducciones(lista,final_pcb+6);
                            element_interfaz * interfaz_fsw = interfaz_existe_y_esta_conectada(nombre_interfaz_fsw); //puntero al elemento original de la lista, ojo
                            if(interfaz_fsw){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                                if(dialFS_acepta_instruccion(interfaz_fsw)){
                                    free(instruccion_fsw); //ya no me importa la instruccion, (ya se cual es)
                                    free(nombre_interfaz_fsw); //ya no me importa el nombre
                                    list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                    pcb_block_dialFS* info_de_bloqueo = malloc(sizeof(pcb_block_dialFS));//falta liberar
                                    info_de_bloqueo->el_pcb = pcb_actualizado ; //simplemente otra referencia 
                                    info_de_bloqueo->instruccion_fs = FS_WRITE;
                                    info_de_bloqueo->tamanio_lectura_o_escritura_memoria= *tamanio_fsw; //no importa
                                    info_de_bloqueo->traducciones=traducciones_fsw; //no importa
                                    info_de_bloqueo->parametros= list_create();
                                    list_add(info_de_bloqueo->parametros,nombre_archivo_fsw);
                                    list_add(info_de_bloqueo->parametros,puntero_archivo_fsw);
                                    cambiar_estado(pcb_actualizado,BLOCKED);
                                    push_con_mutex(interfaz_fsw->cola_bloqueados,info_de_bloqueo,interfaz_fsw->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                    push_con_mutex(cola_block, pcb_actualizado, &mutex_cola_block); 
                                    sem_post(interfaz_fsw->sem_procesos_blocked); 
                                    break;
                                }
                            }
                            //finalizar proceso
                            free(instruccion_fsw); 
                            free(nombre_interfaz_fsw); 
                            free(nombre_archivo_fsw); 
                            free(tamanio_fsw); 
                            list_destroy(lista); 
                            cambiar_estado(pcb_actualizado,EXITT);
                            pcb_actualizado->motivo = INTERFAZ_INVALIDA; // Asi notificamos porq verga finalizo
                            push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                            sem_post(&sem_procesos_exit);
                            break;
                        case FS_READ:
                            char * instruccion_fsr = list_get(lista,final_pcb+1); 
                            char * nombre_interfaz_fsr=list_get(lista,final_pcb+2); 
                            char * nombre_archivo_fsr=list_get(lista,final_pcb+3);
                            uint32_t * tamanio_fsr = list_get(lista,final_pcb+4); 
                            uint32_t * puntero_archivo_fsr = list_get(lista,final_pcb+5); 
                            t_list * traducciones_fsr = desempaquetar_traducciones(lista,final_pcb+6);
                            element_interfaz * interfaz_fsr = interfaz_existe_y_esta_conectada(nombre_interfaz_fsr); //puntero al elemento original de la lista, ojo
                            if(interfaz_fsr){ //entra si no es un puntero nulo (casos: lista vacía (no hay interfaces conectadas), o no hay alguna interfaz con ese nombre)
                                if(dialFS_acepta_instruccion(interfaz_fsr)){
                                    free(instruccion_fsr); //ya no me importa la instruccion, (ya se cual es)
                                    free(nombre_interfaz_fsr); //ya no me importa el nombre
                                    list_destroy(lista); //ya no me interesa la lista, saque toda su informacion necesaria
                                    pcb_block_dialFS* info_de_bloqueo = malloc(sizeof(pcb_block_dialFS));//falta liberar
                                    info_de_bloqueo->el_pcb = pcb_actualizado ; //simplemente otra referencia 
                                    info_de_bloqueo->instruccion_fs = FS_READ;
                                    info_de_bloqueo->tamanio_lectura_o_escritura_memoria= *tamanio_fsr; //no importa
                                    info_de_bloqueo->traducciones=traducciones_fsr; //no importa
                                    info_de_bloqueo->parametros= list_create();
                                    list_add(info_de_bloqueo->parametros,nombre_archivo_fsr);
                                    list_add(info_de_bloqueo->parametros,puntero_archivo_fsr);
                                    cambiar_estado(pcb_actualizado,BLOCKED);
                                    push_con_mutex(interfaz_fsr->cola_bloqueados,info_de_bloqueo,interfaz_fsr->mutex_procesos_blocked); //si estaba en la lista de interfaces, tiene que tener los semaforos inicializados
                                    push_con_mutex(cola_block, pcb_actualizado, &mutex_cola_block); 
                                    sem_post(interfaz_fsr->sem_procesos_blocked); 
                                    break;
                                }
                            }
                            //finalizar proceso
                            free(instruccion_fsr); 
                            free(nombre_interfaz_fsr); 
                            free(nombre_archivo_fsr); 
                            free(tamanio_fsr); 
                            list_destroy(lista); 
                            cambiar_estado(pcb_actualizado,EXITT);
                            pcb_actualizado->motivo = INTERFAZ_INVALIDA; // Asi notificamos porq verga finalizo
                            push_con_mutex(cola_exit,pcb_actualizado,&mutex_lista_exit);
                            sem_post(&sem_procesos_exit);
                            break;
                }
                break;
            }
            sem_post(&sem_despachar);//una vez hecho todo, decirle a despachador() que puede planificar otro pcb
        }
    }
}



//Estos es la lista de recursos ASIGNADOS que es distintos a los SEMAFOROS QUE SE INICIALIZAN EN inicializar_recursos()
t_list* iniciar_recursos_en_proceso(){
	t_list* lista = list_create();
	int cantidad_recursos = string_array_size(recursos);
	for(int i = 0; i<cantidad_recursos; i++){
		char* nombre_obtenido = recursos[i]; // esta lista de recursos viene del logger
		recurso_asignado* recurso = malloc(sizeof(recurso_asignado)); 
		recurso->nombreRecurso = malloc(sizeof(char) * strlen(nombre_obtenido) + 1);
		strcpy(recurso->nombreRecurso, nombre_obtenido);
		recurso->instancias = instancias[i]; // esta lista de instancias viene del logger
		list_add(lista, recurso);
	}
	string_array_destroy(recursos);

	return lista;
}

int simulacion_wait(pcb* proceso, char* recurso_wait){
    int se_bloquearia;
    recurso* recurso_buscado = buscar_recurso(recurso_wait); // devuelve o el recurso encontrado o un recurso con ID = -1 que significa que NO EXISTE
	if(recurso_buscado->id == -1){
        se_bloquearia=true; //habría desalojo
        free(recurso_buscado);
	} else {
        int buffer;
		buffer = recurso_buscado->instancias -1;
		if(buffer < 0){ //Obviamente si es < 0 se bloquea
			se_bloquearia=true;
		} else {
            se_bloquearia=false; // 0 porque es FALSO que hubo desalojo
        }
	}
    return se_bloquearia;
}   
int simulacion_signal(pcb* proceso, char* recurso_signal){
    int se_bloquearia;
    recurso* recurso_buscado = buscar_recurso(recurso_signal);
	if(recurso_buscado->id == -1){
		se_bloquearia=true;
        free(recurso_buscado);
	} else {
       se_bloquearia=false;
	}
    return se_bloquearia;
}

void manejar_wait(pcb* proceso, char* recurso_wait){
    //int rta_a_cpu;
    recurso* recurso_buscado = buscar_recurso(recurso_wait); // devuelve o el recurso encontrado o un recurso con ID = -1 que significa que NO EXISTE
	if(recurso_buscado->id == -1){
        log_error(logger_kernel, "El recurso %s no existe", recurso_wait);
		proceso->motivo = RECURSO_INVALIDO; // antes era motivo_exit, en vez de motivo
		cambiar_estado(proceso, EXITT);
        push_con_mutex(cola_exit,proceso,&mutex_lista_exit); //cuando no existe hay que mandarlo a exit 
		sem_post(&sem_procesos_exit);
        //rta_a_cpu =1; //1 porque sí, hubo desalojo
        free(recurso_buscado);
        //sem_post(&sem_despachar); //Aviso que puede ejecutar otro proceso
	} else {
		recurso_buscado->instancias --;
		if(recurso_buscado->instancias < 0){ //Obviamente si es < 0 se bloquea 
			cambiar_estado(proceso, BLOCKED);
            //cada recurso tiene SU cola y SU mutex
			push_con_mutex(recurso_buscado->cola_block_asignada, proceso, &recurso_buscado->mutex_asignado);
            push_con_mutex(cola_block, proceso, &mutex_cola_block);
			//rta_a_cpu=1;
            //sem_post(&sem_despachar); //Aviso que puede ejecutar otro proceso
		} else {
			agregar_recurso(recurso_buscado->nombreRecurso, proceso); //Hay que asignarle el recurso usado al pcb AGREGAR LISTA DE RECURSOS A LA ESTRUCTURA PCB. Aca es donde me di cuenta que todo se iba a la mierda con el empaquetado
			push_con_mutex(cola_exec, proceso, &mutex_lista_exec);
			//enviar_pcb(proceso,fd_conexion_dispatch,PCBBITO,VACIO,NULL,NULL,NULL,NULL,NULL);
            //rta_a_cpu=0; // 0 porque es FALSO que hubo desalojo
        }
	}
    //send(fd_conexion_dispatch,&rta_a_cpu,sizeof(int),NULL);
}

void manejar_signal(pcb* proceso, char* recurso_signal){
	//int rta_a_cpu;
    recurso* recurso_buscado = buscar_recurso(recurso_signal);
	if(recurso_buscado->id == -1){
		log_error(logger_kernel, "El recurso %s no existe", recurso_signal);
		proceso->motivo = RECURSO_INVALIDO;
		cambiar_estado(proceso, EXITT);
		push_con_mutex(cola_exit,proceso,&mutex_lista_exit); //cuando no existe hay que mandarlo a exit 
		sem_post(&sem_procesos_exit);
        //rta_a_cpu=1;//hubo desalojo
        free(recurso_buscado);
        //sem_post(&sem_despachar); //Aviso que puede ejecutar otro proceso
	} else {
		recurso_buscado->instancias ++;
		quitar_recurso(recurso_buscado->nombreRecurso, proceso);
		if(recurso_buscado->instancias <= 0){
			pcb* proceso_desbloqueado = pop_con_mutex(recurso_buscado->cola_block_asignada, &recurso_buscado->mutex_asignado);
			agregar_recurso(recurso_buscado->nombreRecurso, proceso_desbloqueado);
			procesar_vuelta_blocked_a_ready(proceso_desbloqueado, RECURSOVT);
		}
		push_con_mutex(cola_exec, proceso, &mutex_lista_exec);
        //rta_a_cpu = 0; //no hubo desalojo
		//enviar_pcb(proceso,fd_conexion_dispatch,PCBBITO,VACIO,NULL,NULL,NULL,NULL,NULL);
	}
    //send(fd_conexion_dispatch,&rta_a_cpu,sizeof(int),NULL);
}

recurso *buscar_recurso(recurso* recurso_a_buscar){
    for (int i = 0; i < list_size(lista_recursos); i++) {
        recurso* recurso_buscado = list_get(lista_recursos, i);
        if (strcmp(recurso_buscado->nombreRecurso, recurso_buscado) == 0){
            return recurso_buscado;
        }
    }
    recurso* recurso_no_encontrado = malloc(sizeof(recurso)); // 
    recurso_no_encontrado->id = -1; 
    return recurso_no_encontrado;
}

void agregar_recurso(char* recurso, pcb* pcb){
	for(int i = 0; i<list_size(pcb->recursos_asignados); i++){
		recurso_asignado* recurso_asignado = list_get(pcb->recursos_asignados, i);
		if(strcmp(recurso_asignado->nombreRecurso, recurso) == 0){ //Busca dentro de los recursos del pcb para agregarle 1 instancia
			pthread_mutex_lock(&mutex_asignacion_recursos);
			recurso_asignado->instancias ++;
			pthread_mutex_unlock(&mutex_asignacion_recursos);
		}
	}
}

void quitar_recurso(char* recurso_a_sacar, pcb* pcb){
	for(int i = 0; i<list_size(pcb->recursos_asignados); i++){
		recurso_asignado* recurso_asignado = list_get(pcb->recursos_asignados, i);
		if(strcmp(recurso_asignado->nombreRecurso, recurso_a_sacar) == 0){
			pthread_mutex_lock(&mutex_asignacion_recursos);
			recurso_asignado->instancias --;
			pthread_mutex_unlock(&mutex_asignacion_recursos);
		}
	}
}

//Esta funcion nos devuelve la lista de recursos 
t_list* inicializar_recursos(){
	t_list* lista = list_create();
	int* instancias_recursos = arrayDeStrings_a_arrayDeInts(instancias);
	string_array_destroy(instancias);
	int cantidad_recursos = string_array_size(recursos);

	for(int i = 0; i < cantidad_recursos; i++){ // sobre cada recurso hace:
		char* nombreRecurso = recursos[i];  // obtiene el "nombre"
		recurso* recurso = malloc(sizeof(recurso)); // reserva memoria para la estructura
		recurso->nombreRecurso = malloc(sizeof(char) * strlen(nombreRecurso) + 1); // reserva la memoria para el nombre
		strcpy(recurso->nombreRecurso, recurso); // le encaja el nombre a la estructura en el espacio para el nombre
		t_list* cola_block = list_create(); // crea la lista de blockeados para futuros procesos 
		recurso->id = i; // le asigna un identificador
		recurso->instancias = instancias_recursos[i]; // Pone la cantiad de instancias q tiene disponible
		recurso->cola_block_asignada = cola_block; // le asigna la lista 
		pthread_mutex_init(&recurso->mutex_asignado, NULL); //le creamos un mutex para ese recurso
		list_add(lista, recurso); // agregamos a la lista de recursos este recurso
	}

	free(instancias_recursos);
	string_array_destroy(recursos);
	return lista;
}

//CHAT GPT total, pero se entiende lo que hace, por cada posicion le hace el atoi
int* arrayDeStrings_a_arrayDeInts(char** array_de_strings){
	int count = string_array_size(array_de_strings);
	int *numbers = malloc(sizeof(int) * count);
	for(int i = 0; i < count; i++){
		int num = atoi(array_de_strings[i]);
		numbers[i] = num;
	}

	return numbers;
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

bool generica_acepta_instruccion(char * instruccion){return !strcmp("IO_GEN_SLEEP",instruccion);}

bool STDIN_acepta_instruccion(char * instruccion){return !strcmp("IO_STDIN_READ",instruccion);}

bool STDOUT_acepta_instruccion(char * instruccion){return !strcmp("IO_STDOUT_WRITE",instruccion);}

bool dialFS_acepta_instruccion(char * instruccion){
    return !strcmp("IO_FS_CREATE",instruccion) || !strcmp("IO_FS_DELETE",instruccion) || !strcmp("IO_FS_TRUNCATE",instruccion) || !strcmp("IO_FS_WRITE",instruccion) || !strcmp("IO_FS_READ",instruccion) ;
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
            pcb_a_enviar = obtener_pcb_segun_algoritmo(algoritmo_de_planificacion); // obtiene un pcb de la cola de ready
            cambiar_estado(pcb_a_enviar,EXECUTE);
            push_con_mutex(cola_exec,pcb_a_enviar,&mutex_lista_exec); // uso con mutex porque posiblemente varios hilos agregen a exec
            enviar_pcb(pcb_a_enviar, fd_conexion_dispatch,CODE_PCB,VACIO,NULL,NULL,NULL,NULL,NULL);//falta motivo de desalojo
            sem_post(&sem_atender_rta);//signal para indicar que se despacho? no se si hace falta
            if(!strcmp(algoritmo_de_planificacion,"RR")){
                manejar_quantum_RR(pcb_a_enviar->PID);
            }else if(!strcmp(algoritmo_de_planificacion,"VRR")){
                manejar_quantum_VRR(pcb_a_enviar->PID);
            }
        }
    }
}


bool colaVacia(t_list* colaDeEstado) {
    return colaDeEstado->head == NULL;
}


pcb * obtener_pcb_segun_algoritmo(char * algoritmo){
    pcb * un_pcb;
    if((!strcmp(algoritmo, "FIFO")) || (!strcmp(algoritmo, "RR")) || ((!strcmp(algoritmo, "VRR")) && colaVacia(cola_ready_aux))) {
        un_pcb = pop_con_mutex(cola_ready, &mutex_lista_ready ); 
		return un_pcb; //                                        tanto FIFO, RR y VRR (si cola aux vacia) elijen el primero de la cola ready
    }else if(!strcmp(algoritmo, "VRR")){
        un_pcb = pop_con_mutex(cola_ready_aux, &mutex_lista_ready );
		return un_pcb; //
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
		case READY_AUX:
			return "READY_AUX";
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

void manejar_quantum_RR(int pid){
    //como debe realizarse paralelamente a la ejecucion del proceso, tenemos que delegarlo a un hilo
    pthread_t hilo_manejo_quantum_RR;
	pthread_create(&hilo_manejo_quantum_RR, NULL, (void*) reducir_quantum, (void*)(intptr_t) pid); //necesita PID para manejarle el quantum a ese proceso
	pthread_detach(hilo_manejo_quantum_RR);//                                 (void*) porque si o si debe recibir ese tipo de dato
    //                                                                     (intptr_) para hacer que pid (un int) empiece a ocupar una cantidad de bytes necesaria para ser un puntero
}//                                                                        gracias CHATGPT                                 

void reducir_quantum(void *ppid){ // como todo hilo, es esa su especificacion
    int pid = (int)(intptr_t)ppid; // casteo de nuevo, puntero gen -> intptr_t -> int
    int quantum_como_int = atoi(quantum); // paso el numero quantum (char) a entero
    usleep(quantum_como_int*1000);//microsegundo * 1000 = milisegundo
    if(!list_is_empty(cola_exec)){ // entra al if si SE SIGUE EJECTUANDO EL PROCESO
        pthread_mutex_lock(&mutex_lista_exec); // con mutex porque es caso de lectura y escritura
        pcb * pcb_retirado_de_exec = list_get(cola_exec,0); // obtiene el pcb, sin removerlo de exec
        pthread_mutex_lock(&mutex_lista_exec);
        if(pcb_retirado_de_exec->PID == pid){
            enviar_interrupcion(FIN_QUANTUM,pid);
        }
    }
}

void manejar_quantum_VRR(int pid){
    cronometro = temporal_create();
    manejar_quantum_RR(pid);
    temporal_stop(cronometro);
    tiempo_transcurrido_en_cpu = temporal_gettime(cronometro);
    temporal_destroy(cronometro);
}                             


void enviar_interrupcion(motivo_desalojo motivo, int pid){
    t_paquete* paquete = crear_paquete(INTERR);
	agregar_a_paquete(paquete, &motivo, sizeof(motivo_desalojo));
    agregar_a_paquete(paquete, &pid, sizeof(int));
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


void semaforos_destroy() {
	sem_destroy(&sem_multiprogramacion);//ojo, debería ser sem_destroy() creo 
	sem_destroy(&sem_procesos_new);//ojo, debería ser sem_destroy() 
	sem_destroy(&sem_procesos_ready);//ojo, debería ser sem_destroy() 
	sem_destroy(&sem_despachar);//ojo, debería ser sem_destroy() 
	sem_destroy(&sem_procesos_exit);//ojo, debería ser sem_destroy() 
    sem_destroy(&sem_atender_rta);//ojo, debería ser sem_destroy() 
}


void terminar_programa(){
    config_destroy(config_kernel);
    log_destroy(logger_kernel);   
    log_destroy(logger_obligatorio);
    liberar_conexion(fd_escucha_kernel);

    int i;
    for (i = list_size(cola_exit) - 1; i >= 0; i--){ 
        pcb* pcb_a_finalizar = remove_con_mutex(cola_exit, &mutex_cola_block, i);
        pcb_destroy(pcb_a_finalizar);
    }
    
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
    datos_interfaz->tipo = (vuelta_type) preguntar_tipo_interfaz((*fd_conexion_io));
    datos_interfaz->cola_bloqueados=list_create();  //pcbs_bloqueados es una lista de pcb_block
    sem_init((datos_interfaz->sem_procesos_blocked),0,0); 
    pthread_mutex_init((datos_interfaz->mutex_procesos_blocked),NULL);
        
    push_con_mutex(interfaces_conectadas,datos_interfaz,&mutex_lista_interfaces); //agrega al final de la lista de interfaces

    switch (datos_interfaz->tipo){
    case GENERICA:
        atender_interfaz_generica(datos_interfaz);
        break;
    case STDIN:
        atender_interfaz_STDIN(datos_interfaz);
        break;
    case STDOUT:
        atender_interfaz_STDOUT(datos_interfaz);
        break;
    default:
        atender_interfaz_dialFS(datos_interfaz);
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
                    procesar_vuelta_blocked_a_ready(proceso_a_atender,GENERICA);
                }else if(notificacion  == (-1) ){ //recibir operacion devuelve -1 en caso de que se haya desconectado la interfaz
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
                }
            }
            eliminar_paquete(paquete);
        }
    }
}

void atender_interfaz_STDIN(element_interfaz * datos_interfaz){
    while(1){ //este es para que se pueda pausar y reanudar planificacion 
        while(leer_debe_planificar_con_mutex()){ // genera algo de espera activa cuando debe_planificar = 0;
            sem_wait(datos_interfaz->sem_procesos_blocked);
            pcb_block_STDIN * proceso_a_atender = pop_con_mutex(datos_interfaz->cola_bloqueados,datos_interfaz->mutex_procesos_blocked);//agarra el primero de la cola de blocked 
            //contenido del paquete de instruccion
            t_paquete * paquete = crear_paquete(INSTRUCCION);//   codigo de operacion: INSTRUCCION
            agregar_a_paquete(paquete,proceso_a_atender->tamanio_lectura,sizeof(uint32_t));
            empaquetar_traducciones(paquete,proceso_a_atender->traducciones);
            if (enviar_paquete_io(paquete,*(datos_interfaz->fd_conexion_con_interfaz)) == (-1) ){ //devuelve -1 la interfaz había cerrado la conexion
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
            }else{ //Que devuelve la interfaz al realizar la operacion?
                int notificacion = recibir_operacion(*(datos_interfaz->fd_conexion_con_interfaz),logger_kernel,datos_interfaz->nombre);
                if(notificacion == 1 ){ 
                    procesar_vuelta_blocked_a_ready( proceso_a_atender,STDIN);
                }else if(notificacion  == (-1) ){ //recibir operacion devuelve -1 en caso de que se haya desconectado la interfaz
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
                }else if(!notificacion){
                   liberar_pcb_block_STDIN((void*)proceso_a_atender);
                }
            }
            eliminar_paquete(paquete);
        }
    }
}

void atender_interfaz_STDOUT(element_interfaz * datos_interfaz){
    while(1){ //este es para que se pueda pausar y reanudar planificacion 
        while(leer_debe_planificar_con_mutex()){ // genera algo de espera activa cuando debe_planificar = 0;
            sem_wait(datos_interfaz->sem_procesos_blocked);
            pcb_block_STDOUT * proceso_a_atender = pop_con_mutex(datos_interfaz->cola_bloqueados,datos_interfaz->mutex_procesos_blocked);//agarra el primero de la cola de blocked 
            //contenido del paquete de instruccion
            t_paquete * paquete = crear_paquete(INSTRUCCION);//   codigo de operacion: INSTRUCCION
            agregar_a_paquete(paquete,proceso_a_atender->tamanio_escritura,sizeof(uint32_t));
            empaquetar_traducciones(paquete,proceso_a_atender->traducciones);
            
            if (enviar_paquete_io(paquete,*(datos_interfaz->fd_conexion_con_interfaz)) == (-1) ){ //devuelve -1 la interfaz había cerrado la conexion
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
            }else{ //Que devuelve la interfaz al realizar la operacion?
                int notificacion = recibir_operacion(*(datos_interfaz->fd_conexion_con_interfaz),logger_kernel,datos_interfaz->nombre);
                if(notificacion == 1 ){ //caso exitoso
                    procesar_vuelta_blocked_a_ready( proceso_a_atender,STDOUT);
                }else if(notificacion  == (-1) ){ //recibir operacion devuelve -1 en caso de que se haya desconectado la interfaz
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
                }else if(!notificacion){ //ANALIZAR, no debería invocarse a liberar_pcb block stdout? analizar para stdin y generica tambien
                liberar_pcb_block_STDOUT((void*)proceso_a_atender);//total ya lo habiamos popeado
                }
            }
            eliminar_paquete(paquete);
        }
    }
}

void atender_interfaz_dialFS(element_interfaz * datos_interfaz){
    while(1){ //este es para que se pueda pausar y reanudar planificacion 
        while(leer_debe_planificar_con_mutex()){ // genera algo de espera activa cuando debe_planificar = 0;
            sem_wait(datos_interfaz->sem_procesos_blocked);
            pcb_block_dialFS * proceso_a_atender = pop_con_mutex(datos_interfaz->cola_bloqueados,datos_interfaz->mutex_procesos_blocked);//agarra el primero de la cola de blocked 
            //Ahora, contenido del paquete de instruccion
            
            //nombre del archivo estara en todos los paquetes
            char * p_nombre_archivo=list_get(proceso_a_atender->parametros,0); // lista parametros contiene el nombre del archivo en el primer elemento
            size_t tamanio_nombre_archivo = strlen(p_nombre_archivo);

            t_paquete * paquete = crear_paquete(INSTRUCCION);// paquet creado, ahora agregamos el contenido segund el tipo de instruccion
            agregar_a_paquete(paquete,&(proceso_a_atender->instruccion_fs),sizeof(int));
            agregar_a_paquete(paquete,p_nombre_archivo,tamanio_nombre_archivo+1); //sabemos que el nombre del archivo estará sí o sí
            
            switch(proceso_a_atender->instruccion_fs){
                case FS_CREATE:  //orden: NOMBRE_ARCHIVO 
                   //fs no necesita nada más
                    break;
                case FS_DELETE: //orden: NOMBRE_ARCHIVO 
                   //fs no necesita nada más
                    break;
                case FS_READ: //orden: NOMBRE_ARCHIVO / PUNTERO_ARCHIVO / TRADUCCIONES
                    agregar_a_paquete(paquete,&(proceso_a_atender->tamanio_lectura_o_escritura_memoria),sizeof(uint32_t));
                    agregar_a_paquete(paquete,list_get(proceso_a_atender->parametros,1),sizeof(uint32_t));
                    empaquetar_traducciones(paquete,proceso_a_atender->traducciones);
                    break;
                case FS_WRITE: //NOMBRE_ARCHIVO / PUNTERO_ARCHIVO / TRADUCCIONES
                    agregar_a_paquete(paquete,&(proceso_a_atender->tamanio_lectura_o_escritura_memoria),sizeof(uint32_t));
                    agregar_a_paquete(paquete,list_get(proceso_a_atender->parametros,1),sizeof(uint32_t));
                    empaquetar_traducciones(paquete,proceso_a_atender->traducciones);
                    break;
                case FS_TRUNCATE: //NOMBRE_ARCHIVO / NUEVO TAMANIO
                    agregar_a_paquete(paquete,list_get(proceso_a_atender->parametros,1),sizeof(uint32_t));
                    break;
            }

            //paquete ya esta armado
            if (enviar_paquete_io(paquete,*(datos_interfaz->fd_conexion_con_interfaz)) == (-1) ){ //devuelve -1 la interfaz había cerrado la conexion
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
            }else{ //Que devuelve la interfaz al realizar la operacion?
                int notificacion = recibir_operacion(*(datos_interfaz->fd_conexion_con_interfaz),logger_kernel,datos_interfaz->nombre);
                if(notificacion == 1 ){ 
                    procesar_vuelta_blocked_a_ready( proceso_a_atender,DIALFS);
                }else if(notificacion  == (-1) ){ //recibir operacion devuelve -1 en caso de que se haya desconectado la interfaz
                push_con_mutex(datos_interfaz->cola_bloqueados,proceso_a_atender,datos_interfaz->mutex_procesos_blocked);//lo vuelvo a meter en la cola de bloqueados para procesar la desconexion de la interfaz
                liberar_datos_interfaz(datos_interfaz);//debe liberar estructuras, poner pcbs en exit 
                }else if(!notificacion){ //la interfaz fs no cerro, pero fallo operacion, entonces termina proceso
                liberar_pcb_block_dialFS((void*)proceso_a_atender);//elimino solo este, no todo el resto de los procesos bloqueados
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

void procesar_vuelta_blocked_a_ready(void * proceso_a_atender, vuelta_type tipo){ //proceso_a_atender es un pcb_block gen, o pcb_block_stdin o pcb_block stdout, libera la estructura pcb_blocked, pero de los punteros dinamicos que contenga esta, nos encargamos antes
    char * algoritmo = config_get_string_value(config_kernel,"ALGORITMO_PLANIFICACION");
    switch(tipo){
        case GENERICA:
            pcb_block_gen * proceso_a_atender_gen = (pcb_block_gen *) proceso_a_atender;
            if((!strcmp(algoritmo, "FIFO")) || (!strcmp(algoritmo, "RR")) || ((!strcmp(algoritmo, "VRR")) && proceso_a_atender_gen->el_pcb->QUANTUM >= atoi(quantum)) ){//para dejar en claro que en el caso else el quantum del pcb es menor que el global
                cambiar_estado(proceso_a_atender_gen->el_pcb,READY);
                push_con_mutex(cola_ready,proceso_a_atender_gen->el_pcb,&mutex_lista_ready);
                sem_post(&sem_procesos_ready);
                free(proceso_a_atender_gen->unidad_de_tiempo);
                free(proceso_a_atender_gen);
            }else{
                cambiar_estado(proceso_a_atender_gen->el_pcb,READY_AUX);
                push_con_mutex(cola_ready_aux,proceso_a_atender_gen->el_pcb,&mutex_lista_ready);
                sem_post(&sem_procesos_ready);//el mismo semaforo de procesos porque en si, el proceso esta listo para ejecutarse
                free(proceso_a_atender_gen->unidad_de_tiempo);
                free(proceso_a_atender_gen);
            }
            break;
        case STDIN:
            pcb_block_STDIN * proceso_a_atender_STDIN = (pcb_block_STDIN *) proceso_a_atender;
            if((!strcmp(algoritmo, "FIFO")) || (!strcmp(algoritmo, "RR")) || ((!strcmp(algoritmo, "VRR")) && proceso_a_atender_gen->el_pcb->QUANTUM >= atoi(quantum)) ){//para dejar en claro que en el caso else el quantum del pcb es menor que el global
                cambiar_estado(proceso_a_atender_STDIN->el_pcb,READY);
                push_con_mutex(cola_ready,proceso_a_atender_STDIN->el_pcb,&mutex_lista_ready);
                sem_post(&sem_procesos_ready);
                list_destroy_and_destroy_elements(proceso_a_atender_STDIN->traducciones,(void*)traduccion_destroyer);
                free(proceso_a_atender_STDIN);  
            }else{
                cambiar_estado(proceso_a_atender_STDIN->el_pcb,READY_AUX);
                push_con_mutex(cola_ready_aux,proceso_a_atender_STDIN->el_pcb,&mutex_lista_ready);
                sem_post(&sem_procesos_ready);//el mismo semaforo de procesos porque en si, el proceso esta listo para ejecutarse
                list_destroy_and_destroy_elements(proceso_a_atender_STDIN->traducciones,(void*)traduccion_destroyer);
                free(proceso_a_atender_STDIN);  
            }
            break;
        case STDOUT:
            pcb_block_STDOUT * proceso_a_atender_STDOUT = (pcb_block_STDOUT *) proceso_a_atender;
            if((!strcmp(algoritmo, "FIFO")) || (!strcmp(algoritmo, "RR")) || ((!strcmp(algoritmo, "VRR")) && proceso_a_atender_gen->el_pcb->QUANTUM >= atoi(quantum)) ){//para dejar en claro que en el caso else el quantum del pcb es menor que el global
                cambiar_estado(proceso_a_atender_STDOUT->el_pcb,READY);
                push_con_mutex(cola_ready,proceso_a_atender_STDOUT->el_pcb,&mutex_lista_ready);
                sem_post(&sem_procesos_ready);
                list_destroy_and_destroy_elements(proceso_a_atender_STDOUT->traducciones,(void*)traduccion_destroyer);
                free(proceso_a_atender_STDOUT); 
            }else{
                cambiar_estado(proceso_a_atender_STDOUT->el_pcb,READY_AUX);
                push_con_mutex(cola_ready_aux,proceso_a_atender_STDOUT->el_pcb,&mutex_lista_ready);
                sem_post(&sem_procesos_ready);
                list_destroy_and_destroy_elements(proceso_a_atender_STDOUT->traducciones,(void*)traduccion_destroyer);
                free(proceso_a_atender_STDOUT); 
            }
            break;
        case RECURSOVT:
            pcb * pcb_a_ready = (pcb *) proceso_a_atender;
            if((!strcmp(algoritmo, "FIFO")) || (!strcmp(algoritmo, "RR")) || ((!strcmp(algoritmo, "VRR")) && proceso_a_atender_gen->el_pcb->QUANTUM >= atoi(quantum)) ){//para dejar en claro que en el caso else el quantum del pcb es menor que el global
                cambiar_estado(pcb_a_ready,READY);
                push_con_mutex(cola_ready_aux,pcb_a_ready,&mutex_lista_ready);
                sem_post(&sem_procesos_ready);
            }else{
                cambiar_estado(pcb_a_ready,READY_AUX);
                push_con_mutex(cola_ready_aux,pcb_a_ready,&mutex_lista_ready);
                sem_post(&sem_procesos_ready);
            }
            break;
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
    case STDIN:
        list_iterate(datos_interfaz->cola_bloqueados,(void*)liberar_pcb_block_STDIN);
        list_destroy(datos_interfaz->cola_bloqueados);
        break;
    case STDOUT:
        list_iterate(datos_interfaz->cola_bloqueados,(void*)liberar_pcb_block_STDOUT);
        list_destroy(datos_interfaz->cola_bloqueados);
        break;
    default:
        list_iterate(datos_interfaz->cola_bloqueados,(void*)liberar_pcb_block_dialFS);
        list_destroy(datos_interfaz->cola_bloqueados);
        break;
    }
    
    sem_destroy(datos_interfaz->sem_procesos_blocked);
    pthread_mutex_destroy(datos_interfaz->mutex_procesos_blocked);
    free(datos_interfaz->sem_procesos_blocked);
    free(datos_interfaz->mutex_procesos_blocked);
    free(datos_interfaz);

}

void liberar_pcb_block_gen(void * pcb_bloqueado){
    pcb_block_gen * pcb_bloqueado_c = (pcb_block_gen *) pcb_bloqueado;
    free(pcb_bloqueado_c->unidad_de_tiempo);
    cambiar_estado(pcb_bloqueado_c->el_pcb,EXITT);
    push_con_mutex(cola_exit,pcb_bloqueado_c->el_pcb,&mutex_lista_exit);
    sem_post(&sem_procesos_exit);
    //falta solicitar a memoria liberar las estructuras de ese proceso
    free(pcb_bloqueado_c); //nodo liberado, solo queda destruir la lista
}

void liberar_pcb_block_STDIN(void * pcb_bloqueado){
    pcb_block_STDIN * pcb_bloqueado_c = (pcb_block_STDIN *) pcb_bloqueado;
    list_destroy_and_destroy_elements(pcb_bloqueado_c->traducciones,(void*)traduccion_destroyer);
    cambiar_estado(pcb_bloqueado_c->el_pcb,EXITT);
    push_con_mutex(cola_exit,pcb_bloqueado_c->el_pcb,&mutex_lista_exit);
    sem_post(&sem_procesos_exit);
    //falta solicitar a memoria liberar las estructuras de ese proceso
    free(pcb_bloqueado_c); //nodo liberado, solo queda destruir la lista
}

void liberar_pcb_block_STDOUT(void * pcb_bloqueado){
    pcb_block_STDOUT * pcb_bloqueado_c = (pcb_block_STDOUT*) pcb_bloqueado;
    list_destroy_and_destroy_elements(pcb_bloqueado_c->traducciones,(void*)traduccion_destroyer);
    cambiar_estado(pcb_bloqueado_c->el_pcb,EXITT);
    push_con_mutex(cola_exit,pcb_bloqueado_c->el_pcb,&mutex_lista_exit);
    sem_post(&sem_procesos_exit);
    //falta solicitar a memoria liberar las estructuras de ese proceso
    free(pcb_bloqueado_c); //nodo liberado, solo queda destruir la lista
}

void liberar_pcb_block_dialFS(void * pcb_bloqueado){
    pcb_block_dialFS * pcb_bloqueado_c = (pcb_block_dialFS*) pcb_bloqueado;
    list_destroy_and_destroy_elements(pcb_bloqueado_c->traducciones,(void*)traduccion_destroyer);
    list_destroy_and_destroy_elements(pcb_bloqueado_c->parametros,(void*)free);
    cambiar_estado(pcb_bloqueado_c->el_pcb,EXITT);
    push_con_mutex(cola_exit,pcb_bloqueado_c->el_pcb,&mutex_lista_exit);
    sem_post(&sem_procesos_exit);
    //falta solicitar a memoria liberar las estructuras de ese proceso
    free(pcb_bloqueado_c); //nodo liberado, solo queda destruir la lista
}

