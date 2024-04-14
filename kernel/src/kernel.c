#include <stdlib.h>
#include <stdio.h>
#include <kernel/include/kernel.h>

int main(int argc, char* argv[]) {
    
    //Despues el los ips y puertos los leeriamos de un config, HAY QUE IMPLEMENTARLO. 
    //Podria implementarse un IF que diga si estan bien hechas las conexion, charlarlo en grupo.
    socket_cliente_a_CPU = crear_conexion(ipCPU,puertoCPU);
    socket_cliente_a_MEMORIA = crear_conexion(ipMEMORIA,puertoMEMORIA);

    server_socket = iniciar_servidor(ip , puerto)

    decir_hola("Kernel");
    return 0;
}
