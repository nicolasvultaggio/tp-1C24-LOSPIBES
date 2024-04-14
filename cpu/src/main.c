#include <stdlib.h>
#include <stdio.h>
#include <utils/include/hello.h>
#include </home/utnso/Desktop/TRABAJOPRACTICOSO/tp-2024-1c-Grupo-5/cpu/include/main.h>

int main(int argc, char* argv[]) {
    decir_hola("CPU");
    return 0;

     t_log* logger= log_create("cpu.log","CPU",true, LOG_LEVEL_INFO);
      int socket_server_MEMORIA=0;
      if (!crear_conexion(logger,&socket_server_MEMORIA)){
    liberar_conexion(logger);
       }
    return EXIT_FAILURE;
    int x;
    scanf("&d",&x);
    enviar_mensaje("SOY CPU PUDE CONECTAR", socket_server_MEMORIA);
      liberar crear_conexion(logger);
      return EXIT_SUCCESS;

}