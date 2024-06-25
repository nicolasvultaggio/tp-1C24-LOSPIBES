#include <../include/entradasalida.h>

int main(int argc, char* argv[]) {

    // IMPORTANTE: ANTES DE LAS PRUEBAS: NO OLVIDAR ()
    //      ingresar en el launch.json los posibles argumentos que puede 
    //      tener el main cada vez que se inicia una interfaz, basicamente,
    //      agregar a "args" los diferentes PATHS de archivos de configuracion
    //      para que puedan realizarse las PRUEBAS. cualquier duda: arg en main en doc utn

    // FIN DEL COMUNICADO CAPO SEGUI CON LO TUYO

    if(argc !=3){ // verifica que los parametros sean 3, el modulo (el .c), el nombre de interfaz y su archivo de configuracion
        printf("Ingreso incorrecto de parametros");
        return 1;
    }
    
    nombre_de_interfaz = argv[1]; //argv[0] es el comando en sí, por eso no se usa
    path_configuracion = argv[2];
    decir_hola(nombre_de_interfaz);

    logger_obligatorio = log_create("../../utils/logs_obligatorios.log",nombre_de_interfaz,1,LOG_LEVEL_INFO); // todos los modulos modifican este archivo de logger, por eso en utils
    logger_io= log_create("entradasalida_logs.log",nombre_de_interfaz,1,LOG_LEVEL_INFO); // crea el puntero al log
    config_io = config_create(path_configuracion); // crea el puntero al archivo de config
    
    leer_configuraciones();//eso, guarda toda la info necesaria del archivo de las configuraciones
    
    if(iniciar_conexiones()){
        log_info(logger_io,"Error al iniciar conexión al Kernel");
        terminar_programa();
        exit (2);
    }

    //Ponerse a esperar solicitudes de Kernel

    atender_kernel(); //hilo principal
    //la validacion de la instruccion la hace el kernel, una io recibe la operacion solo si el kernel verificó que la conoce => no hace falta verificarla
    //en principio, GEN, STDIN, STDOUT siempre escuchan a kernel constantemente, para devolver respuesta, iniciativa de operacion nunca de interfaz
    // GEN: NO SE COMUNICA CON MEMORIA -- no me preocupo por eso, solo espera un tiempillo
    // STDIN: se recibe de kernel la indicacion de ponerse a leer algo por teclado
    //        eso ingresado tendra un tamaño (que tambien se recibe por parametro)
    //        debe enviarle a memoria eso que se leyó para guardar en la direccion logica indicada
    //         ==> envia algo a memoria pero no necesita una respuesta, solo dice: guardalo en tal lado
    // STDOUT: se conecta a memoria para obtener el contenido de una direccion fisica, memoria debe devolverle ese valor
    //         y se imprime ese valor por pantalla, en esta interfaz
    //         ==> envia algo a memoria y espera su respuesta
    // IMPORTANTE: que se le DEVUELVE al kernel como respuesta? un flag diciendole que ya se hizo la operacion.
    
    terminar_programa();

    return 0;
}

void leer_configuraciones(){

   tipo_de_interfaz = config_get_string_value(config_io,"TIPO_INTERFAZ");
   if(!strcmp(tipo_de_interfaz,"Generica")){
        type_interfaz = GENERICA;
    }else if(!strcmp(tipo_de_interfaz,"STDIN")){
        type_interfaz = STDIN;
    }else if(!strcmp(tipo_de_interfaz,"STDOUT")){
        type_interfaz = STDOUT;
    }else if(!strcmp(tipo_de_interfaz,"DialFS")){
        type_interfaz = DIALFS;
    }else{
        log_info(logger_io,"Tipo de interfaz desconocido");
    }    
    
    switch(type_interfaz){ // Hay algunas que se leen en todos los casos pero la repeticion de logica nos chupa la cabeza de la chota
        case GENERICA:
            tiempo_unidad_trabajo = atoi(config_get_string_value(config_io,"TIEMPO_UNIDAD_TRABAJO"));
            ip_kernel = config_get_string_value(config_io,"IP_KERNEL");
            puerto_kernel = config_get_string_value(config_io,"PUERTO_KERNEL");
            log_info(logger_io,"Tipo de interfaz iniciada: %s", tipo_de_interfaz);
            break;
        case STDIN:
            ip_memoria = config_get_string_value(config_io,"IP_MEMORIA");
            puerto_memoria = config_get_string_value(config_io,"PUERTO_MEMORIA");
            ip_kernel = config_get_string_value(config_io,"IP_KERNEL");
            puerto_kernel = config_get_string_value(config_io,"PUERTO_KERNEL");
            log_info(logger_io,"Tipo de interfaz iniciada: %s", tipo_de_interfaz);
            break;
        case STDOUT:
            tiempo_unidad_trabajo = atoi(config_get_string_value(config_io,"TIEMPO_UNIDAD_TRABAJO"));
            ip_memoria = config_get_string_value(config_io,"IP_MEMORIA");
            puerto_memoria = config_get_string_value(config_io,"PUERTO_MEMORIA");
            ip_kernel = config_get_string_value(config_io,"IP_KERNEL");
            puerto_kernel = config_get_string_value(config_io,"PUERTO_KERNEL");
            log_info(logger_io,"Tipo de interfaz iniciada: %s", tipo_de_interfaz);
            break;
        case DIALFS:
            tiempo_unidad_trabajo = atoi(config_get_string_value(config_io,"TIEMPO_UNIDAD_TRABAJO"));
            ip_kernel = config_get_string_value(config_io,"IP_KERNEL");
            puerto_kernel = config_get_string_value(config_io,"PUERTO_KERNEL");
            ip_memoria = config_get_string_value(config_io,"IP_MEMORIA");
            puerto_memoria = config_get_string_value(config_io,"PUERTO_MEMORIA");
            path_base_dialfs = config_get_string_value(config_io,"PATH_BASE_DIALFS");
            block_size = atoi(config_get_string_value(config_io,"BLOCK_SIZE"));
            block_count = atoi(config_get_string_value(config_io,"BLOCK_COUNT"));
            retraso_compactacion = atoi(config_get_string_value(config_io,"RETRASO_COMPACTACION"));
            path_bitmap = config_get_string_value(config_io,"PATH_BITMAP");
            path_bloques = config_get_string_value(config_io,"PATH_BLOQUES");
            log_info(logger_io,"Tipo de interfaz iniciada: %s", tipo_de_interfaz);
            break;
        default:
            log_info(logger_io,"Tipo de interfaz desconocido");
    }
    
}

bool iniciar_conexiones(){
    
   switch(type_interfaz){
        case GENERICA:
            fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
            return fd_conexion_kernel != (-1);
            break;
        case STDIN:
            fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
            fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_io,"Memoria");
            return fd_conexion_kernel != (-1) && fd_conexion_memoria != (-1); 
            break;
        case STDOUT:
            fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
            fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_io,"Memoria");
            return fd_conexion_kernel != (-1) && fd_conexion_memoria != (-1); 
            break;
        case DIALFS:
            fd_conexion_kernel = crear_conexion(ip_kernel,puerto_kernel,logger_io,"Kernel");
            fd_conexion_memoria = crear_conexion(ip_memoria,puerto_memoria,logger_io,"Memoria");
            return fd_conexion_kernel != (-1) && fd_conexion_memoria != (-1); 
            break;
        default:
            log_info(logger_io,"Tipo de interfaz desconocido");
            return false;
    }
    return false; 
}

void atender_kernel(){
    int una_operacion = 0;
    do{
        una_operacion = recibir_operacion(fd_conexion_kernel,logger_io,"Kernel"); // se supone que solo recibe la operacion
        switch(una_operacion){
            case (INFORMAR_NOMBRE):
                informar_nombre(); //luego el kernel tiene que recibir, codigo nombre informado, el tamaño del nombre, para guardarlo, y el puntero al nombre
                break;
            case (INFORMAR_TIPO):
                send(fd_conexion_kernel,&type_interfaz,sizeof(int),0);
                break;
            case (INSTRUCCION):
                atender_instruccion(); // luego el kernel solo debe recibir la notificacion de la operacion realizada
                break;
            default:
                log_info(logger_io,"Operacion de Kernel NO reconocida"); 
                break;
        }
    }while(una_operacion != (-1));
    log_info(logger_io,"Finalizo la atencion al Kernel, desconectando...DONE.");
}

void informar_nombre(){ // similar a enviar_mensaje tp0, hay que informar el nombre de la interfaz, el cual es un string de tamaño variable, por eso hay que informar tambien su tamaño
    //send(fd_conexion_kernel,nombre_de_interfaz,strlen(nombre_de_interfaz),0); es una solucion muy simple, hay que serializarlo un poco para que kernel reciba además el tamaño, si lo dejo asi, el kernel no sabe con cuantos bytes guardar el mensaje
    t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = NOMBRE_INFORMADO;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(nombre_de_interfaz) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, nombre_de_interfaz, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(fd_conexion_kernel, a_enviar , bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
	
} //podría hacerse en modo paquete? creo que podría simplificarse a igual que informar_tipo

void atender_instruccion(){
    switch(type_interfaz){
        case GENERICA:
            atender_GENERICA();
            break;
        case STDIN:
            atender_STDIN();
            break;
        case STDOUT:
            atender_STDOUT();
            break;
        case DIALFS:
            atender_DIALFS();
            break;
    }
}
//cambiar a recibirlo como una lista? porque si bien se envía como un paquete, se recibe como un mensaje, es compatible? tp0
char * recibir_unidades_de_tiempo(){ // similar a recibir mensaje del tp0
    int size;
    char * buffer = recibir_buffer(&size,fd_conexion_kernel); //cuando se envie una peticion desde kernel: primero codop, despues unidad de tiempo
    log_info(logger_io, "Kernel me dijo que tengo que esperar << %s >> unidades de tiempo", buffer);
    // OJO HAY QUE LIBERAR EL BUFFER, o sea, habra que liberar la instruccion luego, SI NO --> MEMORY LEAK, ------ listo, resuelto con el free(unidades_de_tiempo)
    return buffer;
}

void atender_GENERICA(){// IO_GEN_SLEEP (Interfaz, Unidades de trabajo), no hace falta validar instruccion, ya se supone que la valido el kernel, y ademas para esta interfaz 
    char * unidades_de_tiempo = recibir_unidades_de_tiempo(); // instruccion queda en memoria dinamica
    int tiempo_a_esperar = tiempo_unidad_trabajo * atoi(unidades_de_tiempo);
    usleep((__useconds_t) tiempo_a_esperar);
    avisar_operacion_realizada_kernel(); //esto va a servir para que el kernel procese vuelta a blocked y pase además, otro proceso pueda usar la interfaz
    free(unidades_de_tiempo); // hay que liberarla, ya que como se menciono anteriormente, la instruccion estaba en memoria dinamica
}

void atender_STDIN(){

    t_list * lista = recibir_paquete(fd_conexion_kernel);
    uint32_t * tamanio_escritura = list_get(lista,0);
    uint32_t tam = *tamanio_escritura;
    free(tamanio_escritura);
    t_list * traducciones = desempaquetar_traducciones(lista,1);
    list_destroy(lista);

    char buffer[tam+1]; //agregamos 1 para poder definir escribir por pantalla
    char *booleano;
    do{
        printf("Ingresar línea:\n");
        if (booleano=fgets(buffer, sizeof(buffer), stdin) != NULL) {
            // Elimina el carácter de nueva línea si está presente
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n') { //en caso de que le hayamos dado enter antes de ocupar el tamaño maximo
                buffer[len - 1] = '\0';
            }
            printf("La línea ingresada es: %s\n", buffer);
        } else {
            printf("Error al leer la línea. Intentar de nuevo\n");
        }
    }while(!booleano); 
    
    uint32_t offset=0;
    int cantidad_de_traducciones = list_size(traducciones);
    bool operacion_exitosa=true;

    for(int i =0; i<cantidad_de_traducciones && operacion_exitosa ; i++){
        
        //preparamos datos a enviar
        nodo_lectura_escritura * traduccion = list_get(traducciones,i);
        char string_a_enviar[traduccion->bytes];
        memcpy(string_a_enviar,buffer+offset,traduccion->bytes);//detalle: no guardamos '\0' en lo que le enviamos a memoria para que escriba
        offset+=traduccion->bytes;
        
        //empaquetamos datos y los enviamos a memoria
        t_paquete * paquete2 = crear_paquete(ESCRITURA_MEMORIA);
        agregar_a_paquete(paquete2,&(traduccion->direccion_fisica),sizeof(uint32_t));
        agregar_a_paquete(paquete2,&(traduccion->bytes),sizeof(uint32_t));//para que sepa cuantos bytes escribir
        agregar_a_paquete(paquete2,string_a_enviar,traduccion->bytes);
        enviar_paquete(paquete2,fd_conexion_memoria);
        eliminar_paquete(paquete2);

        //recibimos respuesta de memoria, si falló ALGUNA escritura, operacion fallida
        int cod_op;
	    recv(fd_conexion_memoria, &cod_op, sizeof(int), MSG_WAITALL); //al pedo, esta nada mas para que podamos recibir el codop antes del paquete
        t_list * lista = recibir_paquete(fd_conexion_memoria);
        char * rta = (char*) list_get(lista,0);
        printf("%s\n",rta); //imprime "ok" en pantalla
        operacion_exitosa = strcmp(rta,"Ok"); //comparamos cada respuesta de memoria, alcanza con que alguna NO sea "Ok"
        free(rta);
        list_destroy(lista);
    }

    list_destroy_and_destroy_elements(traducciones,(void*)traduccion_destroyer);

    if(operacion_exitosa){
        int a =1;
        send(fd_conexion_kernel,&a,sizeof(int),0); // le avisa al kernel que la escritura fue exitosa
    }else{ //en teoria no debería entrar aca porque no falla en la escritura, ya lo controlo la MMU
        int b = 0; //si falla al escribir en memoria finalizo el proceso
        send(fd_conexion_kernel,&b,sizeof(int),0); //le avisa al kernel que la escritura salio mal
    }
    
}

void atender_STDOUT(){
     
    t_list * lista = recibir_paquete(fd_conexion_kernel);
    uint32_t * tamanio_lectura = list_get(lista,0);
    uint32_t tam = *tamanio_lectura;
    free(tamanio_lectura);
    t_list * traducciones = desempaquetar_traducciones(lista,1);
    list_destroy(lista);

    char buffer[tam+1];
    
    uint32_t offset=0;
    int cantidad_de_traducciones = list_size(traducciones);
    bool operacion_exitosa=true;

    for(int i =0; i<cantidad_de_traducciones && operacion_exitosa ; i++){
        
        //preparamos datos a enviar
        nodo_lectura_escritura * traduccion = list_get(traducciones,i);
        
        //empaquetamos datos y los enviamos a memoria
        t_paquete * paquete2 = crear_paquete(LECTURA_MEMORIA);
        agregar_a_paquete(paquete2,&(traduccion->bytes),sizeof(uint32_t));
        agregar_a_paquete(paquete2,&(traduccion->direccion_fisica),sizeof(uint32_t));
        enviar_paquete(paquete2,fd_conexion_memoria);
        eliminar_paquete(paquete2);

        //recibimos respuesta de memoria, escribimos en el buffer
        int cod_op;
	    recv(fd_conexion_memoria, &cod_op, sizeof(int), MSG_WAITALL); //al pedo, esta nada mas para que podamos recibir el codop antes del paquete
        t_list * lista = recibir_paquete(fd_conexion_memoria);
        char * string_leido = (char*) list_get(lista,0);
        list_destroy(lista);

        memcpy(buffer+offset,string_leido,(size_t)traduccion->bytes);//detalle: no guardamos caracter nulo
    
        offset+=traduccion->bytes;
    }
    
    buffer[tam+1]='\0'; // agregamos '\0' solo para escribir por pantalla (necesario para printf)

    printf("%s",buffer);

    list_destroy_and_destroy_elements(traducciones,(void*)traduccion_destroyer);

    if(operacion_exitosa){
        int a =1;
        send(fd_conexion_kernel,&a,sizeof(int),0); // le avisa al kernel que la lectura fue exitosa
    }else{
        int b = 0; 
        send(fd_conexion_kernel,&b,sizeof(int),0); 
    }
}

    


void atender_DIALFS(){ // hoy, 2/5/2024, a las 20:07, esuchando JIMMY FALLON de Luchito, empiezo la funcion mas dificil de las interfaces, suerte loko 
                    // hoy, 25/6/2024, a las 11:48 escuchando Not Like us de kendrick (RIP Drake), continuamos esta verga gomosa
   
   // char * instruccion = recibir_instruccion_de_kernel();
    //free(instruccion);
    inicializar_archivos();//.h 
}

void inicializar_archivos(){
    abrir_bitmap();//.h
    abrir_archivo_bloques();//.h
}

void abrir_bitmap(){
    int fd = open(path_bitmap, O_RDWR);
    // uso open ya que a la hora de mapear el archivo con mmap este usa el fd del archivo y open te lo da directo, fopen te da el puntero al archivo
    tamanio_bitmap = ceil(block_count/8); //.h Cantidad de bloques / tamanio de bloques = tamanio del bitmap en bytes. CEIL redondea para arriba 

    buffer_bitmap = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    //parametros mmap(donde se mapea(NULL = elegimos q el sist elija) , tamanio, permisos, dejamos q los cambios que se hacen en memoria se reflejen en el archivo FUNDAMENTAL, fs, desplazamiento)
    bitmap = bitarray_create_with_mode(buffer_bitmap, tamanio_bitmap, LSB_FIRST);

    close(fd);
}

void abrir_archivo_bloques(){
    int fd = open(path_bloques, O_RDWR);
    tamanio_bloques = block_size * block_count;

    buffer_bloques = mmap(NULL, tamanio_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd,0);

    close(fd); 
}

void avisar_operacion_realizada_kernel(){
    int a=1;
    send(fd_conexion_kernel, &a,sizeof(int),0); //ponele que esa notificacion es simplemente recibir el numero 1, al menos para las genericas, por ahora
}

void terminar_programa()
{
	if(logger_io != NULL){
		log_destroy(logger_io);
	}
	if(config_io != NULL){
		config_destroy(config_io);
	}
    liberar_conexion(fd_conexion_kernel);
    liberar_conexion(fd_conexion_memoria);
}




