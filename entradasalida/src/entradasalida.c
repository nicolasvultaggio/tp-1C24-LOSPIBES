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
    
    inicializar_archivos();

    if(iniciar_conexiones()){
        log_info(logger_io,"Error al iniciar conexión al Kernel");
        terminar_programa();
        exit (2);
    }

    //Ponerse a esperar solicitudes de Kernel

    atender_kernel(); //hilo principal
    
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

    printf("\n%s\n",buffer);

    list_destroy_and_destroy_elements(traducciones,(void*)traduccion_destroyer);

    if(operacion_exitosa){
        int a =1;
        send(fd_conexion_kernel,&a,sizeof(int),0); // le avisa al kernel que la lectura fue exitosa
    }else{
        int b = 0; 
        send(fd_conexion_kernel,&b,sizeof(int),0); 
    }
}

    


void atender_DIALFS(){ 
// hoy, 2/5/2024, a las 20:07, esuchando JIMMY FALLON de Luchito, empiezo la funcion mas dificil de las interfaces, suerte loko 
// hoy, 25/6/2024, a las 11:48 escuchando Not Like us de kendrick (RIP Drake), continuamos esta verga gomosa

    t_list * lista = recibir_paquete(fd_conexion_kernel);

    int operacion = *list_get(lista,0); //siempre el primero
    free(list_get(lista,0));

    char * nombre_archivo_operacion = list_get(lista,1); //siempre el segundo

    switch(operacion){
        case FS_CREATE:
            
            create_file(nombre_archivo_operacion);  
            
            break;
        case FS_DELETE: 
            
            delete_file(nombre_archivo_operacion);
            
            break;
        case FS_READ:
            
            uint32_t tamanio_lectura = *list_get(lista,2);
            uint32_t puntero_archivo_r = *list_get(lista,3);
            t_list * traducciones_r = desempaquetar_traducciones(lista,4);
            
            read_file(nombre_archivo_operacion, tamanio_lectura,puntero_archivo_r,traducciones_r);
            
            free(list_get(lista,2));
            free(list_get(lista,3));
            list_destroy_and_destroy_elements(traducciones_r,(void*)traduccion_destroyer);
            
            break;
        case FS_WRITE:
            
            uint32_t tamanio_escritura = *list_get(lista,2);
            uint32_t puntero_archivo_w = *list_get(lista,3);
            t_list * traducciones_w = desempaquetar_traducciones(lista,4);
            
            write_file(nombre_archivo_operacion, tamanio_escritura,puntero_archivo_w,traducciones_w);
            
            free(list_get(lista,2));
            free(list_get(lista,3));
            list_destroy_and_destroy_elements(traducciones_w,(void*)traduccion_destroyer);
            
            break;
        case FS_TRUNCATE:
            uint32_t nuevo_tamanio_archivo = *list_get(lista,2);
            
            truncate_file(nombre_archivo_operacion,nuevo_tamanio_archivo);
            
            free(list_get(lista,2));
            break;
    }

    free(nombre_archivo_operacion);
    list_destroy(lista);


}

fcb * buscar_archivo(char * name_file){
    
    bool archivo_de_nombre(void* nc){
        fcb * un_fcb = (fcb*) nc;
        return !strcmp(un_fcb->nombre_archivo,name_file);
    }

    fcb * archivo_encontrado= list_find(lista_fcbs,(void*)archivo_de_nombre);
    
    return archivo_encontrado;

}

void create_file(char * name_file){
    
    int nro_bloque = (int)buscar_primer_bloque_libre(); //debemos buscar en el bitmap alguno libre

    if( nro_bloque != (-1)){//si encontro un bloque libre
        
        //actualiza el bitmap
        bitarray_set_bit(bitmap, (off_t) nro_bloque);

        //formamos ruta relativa
        char ruta_relativa[strlen(name_file)+2+1];//dos para el "./" y uno para el '\0'
        memcpy(ruta_relativa,"./",3);
        strcat(ruta_relativa+2, name_file);
        
        //creamos archivo de texto para poder relacionarlo al t_config (si el archivo no existe, config_create devuelve null)
        FILE * f;
        f = fopen(ruta_relativa, "w");
        fclose(f); 

        //lo guardamos en el config
        t_config * new_metadata = config_create(ruta_relativa);

        //escribimos valores iniciales
        escribir_metadata(new_metadata,"BLOQUE_INICIAL", nro_bloque);
        escribir_metadata(new_metadata, "TAMANIO_ARCHIVO",0);

        //acomodamos todos los datos en la estructura creada
        fcb * new_fcb = malloc(sizeof(fcb));
        new_fcb->metadata = new_metadata;
        new_fcb->bloque_inicial = nro_bloque;
        new_fcb->nombre_archivo=name_file;
        new_fcb->tamanio_archivo=0;
        
        //agregamos a la lista de fcbs
        list_add(lista_fcbs,new_fcb);
    }

    return;
    
}

off_t buscar_primer_bloque_libre() {// guarda que no estaba en el .h

    for (off_t i = 0; i < tamanio_bitmap; i++) {
        if (!bitarray_test_bit(bitmap, i)) {
            return i; // devuelve la posicion dentro del bitmap del primer bloque libre que encontro
        }
    }

    return (-1); // devuelve (-1) porque no se encontró ningún bloque
};

char * intTOString(int numero){
    char buffer[contar_digitos(numero)+1];//uno más para el '\0'
    sprintf(buffer, "%d", numero);
    return buffer;
}

void escribir_metadata(t_config * metadata,char * key, int valor){// si no existe lo crea (y escribe), y si ya existe, lo borra y vuelve a escribir

    char * valor_como_char = intTOString(valor);

    config_set_value(metadata,key,valor_como_char);

    return;
    
} 
int leer_metadata(t_config * metadata, char* key){
    return config_get_int_value(metadata,key);
}

void delete_file(char * name_file){
    
    fcb * fcb_file = buscar_archivo(name_file);

    //actualiza bitmap
    int bloque_inicial = fcb_file->bloque_inicial;
    int tamanio = fcb_file->tamanio_archivo;
    int cant_bloques = ceil((double)tamanio/block_size);
    for (int i=0; i < cant_bloques; i++){
        bitarray_clean_bit(bitmap,(off_t) bloque_inicial + i);
    }
    
    //2) eliminar de la lista de fcb el fcb del archivo y hacerle free

    list_remove_element(lista_fcbs, fcb_file);
    //free(fcb_file->nombre_archivo);
    //free(fcb_file->bloque_inicial);
    //free(fcb_file->tamanio_archivo);
    //actualizo metadata y desp le hago free?

    //3) actualizar metadata
    
}

void truncate_file(char * name_file,uint32_t nuevo_tamanio){
    /*
    EN PAINT esta graficado, lo pongo aca para verlo mientras desarrollamos
    REVISENLO BIEN Y ATENTOS PORQ SEGURAMENTE EN ALGO PIFIE, si en alguna implementacion no estan de acuerdo avisen y lo hablamos en ds
    PASOS:
    1. Buscar el archivo y obtenemos el valor del bloque inicial
    2. Buscamos en el bitmap si se le puede asignar la cantidad de bloques que pidio (LO mas probable es que NO)
    2.1 Si se puede, ocupamos esos bits en el bitmap, actualizamos el tamanio en el metada y en el fcb
    2.2 No se puede, pasamos al paso 3.
    3. Calculamos cuantos bloques ocupa y actualizamos en el bitmap esa cantidad de 1s por 0s
    4. Buscamos en al archivo de bloques donde esta la info escita por el archivo y guardamos en un buffer auxiliar todo lo escrito (Puede ser que no haya nada escrito en esos bloques, pero no se si hay forma de comprobar eso me parece que si o si deberiamos desarrollar una funcion para chequearlo. Si se puede comprobar antes de copiar en el buffer comprobamos que haya cosas escritas)
                                                                                                                                    ^Hay que ver bien esto
    5. Ahora COMPACTAMOS. Movemos todos los bloques ocupados al principio para poder dejar la mayor cantidad de bloques continuos al final (ejemplo de como queadria el bitmap si tenemos 3 archivos q ocupan 1 bloque: 1110000000000). Pasos:
    Vamos a tener 2 punteros. PunteroA(PA): Apunta al primer espacio libre. Puntero B(PB): Apunta al bloque inicial del archivo a mover.
    Secuencia:
    1. Recorremos con el PA el bitmap hasta encontrar el primer 0 (Importante guardar este valor ya que va a pasar a ser el bloque inicial del archivo a mover)
    2. A partir de la posicion de PA, con PB buscamos el primer 1
    3. PB nos va a dar la POSICION INICIAL de algun archivo, entonces, con ese valor buscamos en la lista de fcbs cual es el archivo que coincide con ese bloque inicial. (Importante guardar este valor para ir al archivo de bloques)
    4. Calculamos cuantos bloques ocupa ese archivo (cantidad = N). 
    5. Cambiamos N 1s por 0s a partir de la posicion de PB y tambien cambiamos N 0s por 1s a partir de la posicion de PA, esto quiere decir que nuestro archivo ya va a estar ocupando su nueva posicion.  
    6. Buscamos en el archivo de bloques la posicion dada por PB y en un buffer escribimos los datos escritos en los N bloques que ocupa el archivo. (Puede ser que no haya nada escrito en esos bloques, pero no se si hay forma de comprobar eso me parece que si o si deberiamos desarrollar una funcion para chequearlo. Si se puede comprobar antes de copiar en el buffer comprobamos que haya cosas escritas)
    7. BUscamos en el archivo de bloques la posicion dada por PA y escribimos lo que esta en el buffer. (NO importa si antes habia algo escrito en esos bloques ya que lo podemos sobreescribir sin problema)
    8. Volvemos al paso 1 de la secuencia.
    */

}

void read_file(char* nombre_archivo,uint32_t tamanio_lectura,uint32_t puntero_archivo,t_list * traducciones){
    
    //buscamos leer el archivo

    fcb * fcb_file = buscar_archivo(nombre_archivo);
   
    bool operacion_exitosa=true;
    int first_block;
    uint32_t direccion_archivo_bloquesDAT;
    char buffer[tamanio_lectura];
    uint32_t offset;
    int cantidad_de_traducciones;

    operacion_exitosa = bytes_pertenecen_a_archivo(fcb_file,puntero_archivo,tamanio_lectura);//lo pone en false si no cumple

    if(operacion_exitosa){

        first_block = fcb_file->bloque_inicial;

        direccion_archivo_bloquesDAT = ((uint32_t)first_block)*((uint32_t)block_size)+puntero_archivo;
    
        memcpy(buffer,buffer_bloques+direccion_archivo_bloquesDAT,(size_t)tamanio_lectura);
   
        //ya leimos el archivo y guardamos los bytes en 'buffer'
        offset=0;
        cantidad_de_traducciones = list_size(traducciones);

    }
    
    for(int i =0; i<cantidad_de_traducciones && operacion_exitosa ; i++){
        
        //leemos bytes del buffer como indique esa traduccion
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
        operacion_exitosa = !strcmp(rta,"Ok"); //comparamos cada respuesta de memoria, alcanza con que alguna NO sea "Ok"
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


void write_file(char* nombre_archivo, uint32_t tamanio_escritura, uint32_t posicion_a_escribir,t_list * traducciones){

    fcb* archivo = buscar_archivo(nombre_archivo);

    int tam =(int) tamanio_escritura;
    char buffer[tam+1];
    
    uint32_t offset=0;
    int cantidad_de_traducciones = list_size(traducciones);
    bool operacion_exitosa=true;

    if(!bytes_pertenecen_a_archivo(archivo,posicion_a_escribir,tamanio_escritura)){
        log_info(logger_io,"FALLO AL ESCRIBIR: %s. SEGMENTATION FAULT. Estos bytes no le pertenecen al archivo.");
        operacion_exitosa=false;
    }

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

    buffer[tam+1]='\0'; //En STDOUT se usaba para poder hacer printf. Sis pero para escribirlo necesito saber el tamanio por eso lo agrego 

    if (operacion_exitosa){
        escribir_archivo(archivo, posicion_a_escribir, buffer);
    }
    

    list_destroy_and_destroy_elements(traducciones,(void*)traduccion_destroyer);

    if(operacion_exitosa){
        int a =1;
        send(fd_conexion_kernel,&a,sizeof(int),0); // le avisa al kernel que la lectura fue exitosa
    }else{
        int b = 0; 
        send(fd_conexion_kernel,&b,sizeof(int),0); 
    }
    
    
}

bool bytes_pertenecen_a_archivo(fcb* archivo, uint32_t posicion, uint32_t tamanio_operacion){
    return (posicion + tamanio_operacion) < archivo->tamanio_archivo;
}

void escribir_archivo(fcb* archivo, uint32_t posicion_a_escribir, char* buffer){  
    int posicion_bloque = archivo->bloque_inicial;
    int tamanio_buffer = strlen(buffer);

    void* posicion_a_escribir_en_bloques = buffer_bloques + ((archivo->bloque_inicial)*block_size) + (int) posicion_a_escribir;
    
    memcpy(posicion_a_escribir_en_bloques, buffer, tamanio_buffer-1); //Ahi se escribe sin el \0

    msync(posicion_a_escribir_en_bloques,tamanio_bloques,MS_SYNC);

    return;
}

int contar_digitos(int numero) {
    int digitos = 0;

    // Maneja el caso especial para 0
    if (numero == 0) {
        return 1;
    }

    // Si el número es negativo, conviértelo a positivo
    if (numero < 0) {
        numero = -numero;
    }

    // Cuenta los dígitos
    while (numero > 0) {
        numero /= 10;
        digitos++;
    }

    return digitos;
}



void inicializar_archivos(){
    abrir_bitmap();
    abrir_archivo_bloques();
    lista_fcbs = list_create();
}

void abrir_bitmap(){
    int fd;
    tamanio_bitmap = ceil(block_count / 8);

    // Verificar si el archivo existe
    if(access(path_bitmap, F_OK) == (-1)){ // acces verifica que el archivo EXISTA
        // Si el archivo no existe, hay que crearlo. Con OPEN tambien podemos crearlo si le ponemos el flag o_create.
        log_info(logger_io, "El archivo bitmap no existe, creamos uno nuevo.");
        //Como se lee este open? Si existe abrilo con permiso de lectura y escritura -> es la parte de open(path_bitmap, O_RDWR) | si no existe crealo y dale permiso de lectura y escritura la parte de open(lo anterior| o_create, s_irsusr | siwusr)
        fd = open(path_bitmap, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == (-1)) {
            log_error(logger_io, "No pude crear el archivo bitmap");
            exit(1);
        }

        if (ftruncate(fd, tamanio_bitmap) == (-1)) { // Una vez creado le damos el tamanio del bitmap(esto vi en el foro que esta bien, ese seria el tamanio)
            log_error(logger_io, "No puedo agrandar el archivo");
            close(fd);
            exit(1);
        }
    } else {
        //si el archivo existe solamente lo creamos
        fd = open(path_bitmap, O_RDWR); 
        if (fd == (-1)) {
            log_error(logger_io, "No pude abrir el archivo bitmap");
            exit(1);
        }
    }

    // Mapear el archivo a memoria
    // BOrre la explicacion de mmap con esta implementacion, si alguno tiene alguna duda de como esta hecho pregunte.
    buffer_bitmap = mmap(NULL, tamanio_bitmap, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer_bitmap == MAP_FAILED) {
        log_error(logger_io, "Error al mapear el archivo bitmap.");
        close(fd);
        exit(1);
    }

    // Crear el bitarray con el buffer mapeado
    bitmap = bitarray_create_with_mode(buffer_bitmap, tamanio_bitmap, LSB_FIRST);

    close(fd);
}


void abrir_archivo_bloques(){
    int fd;
    tamanio_bloques = block_size * block_count;

    // Verificar si el archivo existe
    if(access(path_bloques, F_OK) == (-1)){// acces verifica que el archivo EXISTA
        // El archivo no existe, hay que crearlo (TODA LA EXPLICACION DEL OPEN ESTA ARRIBA, ANDA A LEERLO LA CONCHA DE TU RENEGRIDA MADRE)
        log_info(logger_io, "El archivo de bloques no existe, creamos uno nuevo");
        fd = open(path_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd == (-1)) {
            log_error(logger_io, "No pude crear el archivo de bloques");
            exit(1);
        }
        // Le damos mas focking tamanio
        if (ftruncate(fd, tamanio_bloques) == (-1)) {
            log_error(logger_io, "No pude redimensionar el archivo de bloques");
            close(fd);
            exit(1);
        }
    } else {
        // si existe simplemente hay que abrirlo
        fd = open(path_bloques, O_RDWR);
        if (fd == (-1)) {
            log_error(logger_io, "No pude abrir el archivo de bloques");
            exit(1);
        }
    }

    // Mapear el archivo a memoria
    buffer_bloques = mmap(NULL, tamanio_bloques, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer_bloques == MAP_FAILED) {
        log_error(logger_io, "Error al mapear el archivo de bloques.");
        close(fd);
        exit(1);
    }

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




