# lanzaServidor.sh
# Lanza el servidor que es un daemon y varios clientes
# las ordenes est�n en un fichero que se pasa como tercer par�metro
sleep 1
clear

./servidor
#./cliente nogal TCP ordenes1.txt &
#./cliente nogal TCP ordenes2.txt &
#./cliente nogal TCP ordenes3.txt &
#./cliente nogal UDP ordenes1.txt &
#./cliente nogal UDP ordenes2.txt &
#./cliente nogal UDP ordenes3.txt &

./cliente localhost TCP ordenes1.txt &
./cliente localhost TCP ordenes2.txt &
./cliente localhost TCP ordenes3.txt &
./cliente localhost UDP ordenes1.txt &
./cliente localhost UDP ordenes2.txt &
./cliente localhost UDP ordenes3.txt &
