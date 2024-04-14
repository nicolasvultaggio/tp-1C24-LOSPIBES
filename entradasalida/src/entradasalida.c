#include <stdlib.h>
#include <stdio.h>
#include <../include/entradasalida.h>

int main(int argc, char* argv[]) {
    
    decir_hola("una Interfaz de Entrada/Salida"); // se conecta la entrada y salida

    logger = log_create("entradasalida_logs.log","entradasalida",1,LOG_LEVEL_INFO); // crea el puntero al log
    config = config_create("./entradasalida.config"); // crea el puntero al archivo de config
    
    leer_las_configs();//eso, guarda toda la info necesaria del archivo de las configuraciones
    
    if(!conectar_a_kernel()){
        log_error(logger,"No se pudo conectar al Kernel del sistema");
        terminar_programa();
        exit(1);
    } else if(!conectar_a_memoria()){
        log_error(logger,"No se pudo conectar a la memoria del sistema");
        terminar_programa();
        exit(2);
    }

    return 0;
}

void leer_las_configs(){
    ip_memoria = config_get_string_value(config,"IP_MEMORIA");
    puerto_memoria = config_get_string_value(config,"PUERTO_MEMORIA");
    ip_kernel = config_get_string_value(config,"IP_KERNEL");
    puerto_kernel = config_get_string_value(config,"PUERTO_KERNEL");
    return;
}

bool conectar_a_kernel(){
    socket_cliente_a_kernel = crear_conexion(ip_kernel,puerto_kernel); 
    return socket_cliente_a_kernel != -1; 
}

bool conectar_a_memoria(){
    socket_cliente_a_memoria = crear_conexion(ip_memoria,puerto_memoria); 
    return socket_cliente_a_memoria != -1; 
}

void terminar_programa()
{
	if(logger != NULL){
		log_destroy(logger);
	}
	if(config != NULL){
		config_destroy(config);
	}
}