## Instrucciones para deployar:
## Escribir make deploy en la consola.
## Cuando pida contraseña de root, proporcionarla.
## Cuando pida user de github, proporcionarlo.

## Cada vez que se haga "make deploy" el make automáticamente
## bajará lo que no esté bajado y compilará lo no compilado.
## Si se desea rehacer el proceso, escribir "sudo make clean", lo que reiniciará todo.

deploy: dependencias build

dependencias: so-commons-library ansisop-parser b

so-commons-library:
	$(call mostrarTitulo,$@)
	cd ..; git clone https://github.com/sisoputnfrba/so-commons-library
	cd ../so-commons-library; sudo make install
	
ansisop-parser:
	$(call mostrarTitulo,$@)
	cd ..; git clone https://github.com/sisoputnfrba/ansisop-parser
	cd ../ansisop-parser/parser; make all	
	cd ../ansisop-parser/parser; sudo make install	
	tar -xzvf ansisop-parser/programas-ejemplo/evaluacion-final-esther/FS-ejemplo/FS.tgz
	tar -xvzf ansisop-parser/programas-ejemplo/evaluacion-final-esther/FS-ejemplo/SADICA_FS_V2.tar.gz
	
build: herramientas filesystem cpu consola kernel memoria

herramientas: 
	$(call mostrarTitulo,$@)
	cd Herramientas/Debug/; make all
	sudo cp -u ./Herramientas/Debug/libHerramientas.so /usr/lib/libHerramientas.so
	sudo cp -u ./Herramientas/* /usr/include

filesystem:
	$(call mostrarTitulo,$@)
	cd Filesystem/Debug; make all

consola:
	$(call mostrarTitulo,$@)
	cd CPU/Debug; make all
	
consola:
	$(call mostrarTitulo,$@)
	cd Consola/Debug; make all
	
kernel:
	$(call mostrarTitulo,$@)
	cd Kernel/Debug; make all

memoria:
	$(call mostrarTitulo,$@)
	cd Memoria/Debug; make all
	
clean:
	$(call mostrarTitulo,$@)
	rm -rf ../base.bin*
	rm -rf ../completa.bin*
	rm -rf ../osada-tests
	rm -rf ../osada-utils
	rm -rf ../so-commons-library
	rm -rf ../so-nivel-gui-library
	rm -rf ../so-pkmn-utils
	rm -rf ../massive-file-creator
	
define mostrarTitulo
	@echo
	@echo "########### $(1) ###########################################"
endef
