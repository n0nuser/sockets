# Práctica Redes I (USAL) - Sockets 

# Nota Final : 8

## Parcipantes:

<table>
  <tr>
    <td align="center"><a href="https://github.com/AnOrdinaryUsser"><img width="100px;" src="https://avatars2.githubusercontent.com/u/61872281?s=460&u=e276002ebcb7a49338dac7ffb561cf968d6c0ee4&v=4"></td>
    <td align="center"><a href="https://github.com/n0nuser"><img width="100px;" src="https://avatars3.githubusercontent.com/u/32982175?s=460&u=ce93410c9c5e0f3ffa17321e16ee2f2b8879ca6f&v=4"></td>
  </tr>
</table>

# Requisitos para la práctica

## Programa Servidor
- Aceptará peticiones de sus clientes tanto en TCP como en UDP
- Registrará todas las peticiones en un fichero de "log" llamado `nntpd.log` en el que anotará:
  - Fecha y hora Comunicación realizada: nombre del host, dirección IP, protocolo de transporte y nº de puerto efímero del cliente
  - Fecha y hora, comando enviado o recibido
- Se ejecutará como un "daemon"

## Programa Cliente

- Se conectará con el servidor bien con TCP o UDP
- Leerá por parámetros el nombre del servidor y el protocolo de transporte TCP o UDP de la siguiente forma:
  - cliente nombre_o_IP_del_servidor TCP
- Consultará y enviará noticias

## Pruebas

Durante la fase de pruebas el cliente podrá ejecutarse como se muestra en el ejemplo de diálogo anterior, pero en la versión para entregar el cliente:
- Leerá las órdenes desde un fichero .txt que se leerá como tercer parámetro. Así los clientes se ejecutarán:
  - ./cliente nogal TCP ordenes1.txt &
- Escribirá los mensajes de progreso y los mensajes de error y/o depuración en un fichero con nombre el número de puerto efímero del cliente y extensión `.txt`

## Versión entregable

- Para verificar que esta práctica funciona correctamente y permite operar con varios clientes, se utilizará el script `lanzaServidor.sh` que ha de adjuntarse obligatoriamente en el fichero de entrega de esta práctica
- El contenido de `lanzaServidor.sh` es el siguiente:

```
# lanzaServidor.sh
# Lanza el servidor que es un daemon y varios clientes
./servidor
./cliente nogal TCP ordenes1.txt &
./cliente nogal TCP ordenes2.txt &
./cliente nogal TCP ordenes3.txt &
./cliente nogal UDP ordenes1.txt &
./cliente nogal UDP ordenes2.txt &
./cliente nogal UDP ordenes3.txt &
```

- Entregar la estructura de directorios del servidor que permita ejecuta las diferentes órdenes

## Documentación

Entregar un informe en formato PDF que contenga:
- Detalles relevantes del desarrollo de la práctica
- Documentación de las pruebas de funcionamiento realizadas
