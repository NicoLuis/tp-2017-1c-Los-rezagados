#!usr/bin/sh

clear

cd ..

cd so-commons-library
make all
sudo make install
echo "Se compilo la commons."

cd ..

cd ansisop-parser
cd parser
make all
sudo make install
echo "Se compilo el parser."

cd ../..

cd tp-2017-1c-Loz-rezagados

cd Herramientas
cd Debug
make clean
make install
cd ..
cd ..

cd Consola
cd Debug
make clean
make all
cd ..
cd ..

cd CPU
cd Debug
make clean
make all
cd ..
cd ..

cd Memoria
cd Debug
make clean
make all
cd ..
cd ..

cd FileSystem
cd Debug
make clean
make all
cd ..
cd ..

cd Kernel
cd Debug
make clean
make all
cd ..
cd ..

cd Consola
cd Debug
cd pruebas
chmod +x *.ansisop

echo ""
echo "Se compilaron los 5 modulos."
