# Sistemas operativos (Proyecto Final): Lectura y Visualización de una Imagen EXT4.

Este proyecto tiene como objetivo leer el contenido de un archivo `imagen.img` (imagen descomprimida de `imagen.img.gz`) y mostrar información relevante del sistema de archivos EXT4, incluyendo SuperBlock, Descriptor de Bloques e Inodes. Además, permite navegar por la estructura de directorios y visualizar archivos en formato hexadecimal.
## Tabla de Contenidos

- [Descripción](#descripción)
- [Requerimientos de Instalación](#requerimientos-de-instalación)
- [Instalación](#instalación)
- [Uso](#uso)
- [Referencias](#referencias)
- [Licencia](#licencia)
- [Contacto](#contacto)

## Descripción

El propósito de este proyecto es leer y mostrar información relevante del sistema de archivos EXT4 desde un archivo de imagen. Las funcionalidades incluyen:

- Mostrar información del SuperBlock de EXT4.
- Mostrar información del Descriptor de Bloques e Inodes (opcional).
- Navegar por el directorio de la imagen y seguir la estructura de directorios.
- Visualizar archivos en formato hexadecimal.
- Guardar el contenido de un archivo seleccionado.
- 
## Requerimientos de Instalación

Para poder instalar y ejecutar este proyecto, necesitarás tener las siguientes herramientas instaladas:

- C Compiler (GCC)
- make
- zlib (para descomprimir archivos .gz)
- ncurses (para la visualización en formato hexadecimal)

## Instalación

Para instalar y ejecutar el proyecto localmente, sigue estos pasos:

1. Clona el repositorio:

   ```bash
   git clone https://github.com/tu_usuario/tu_proyecto.git
   cd tu_proyecto
2. Descomprime el archivo de imagen:

   ```bash
   gunzip imagen.img.gz
3. Compila el proyecto usando 'make':

   ```bash
   make
4. Ejecuta el programa:

   ```bash
   ./ext4_reader imagen.img

## Uso
Para utilizar este programa, simplemente ejecuta los comandos mencionados anteriormente. Una vez ejecutado, el programa te permitirá:
* Mostrar la información del SuperBlock de EXT4.
* Mostrar información del Descriptor de Bloques e Inodes.
* Navegar por el directorio de la imagen y seguir la estructura de directorios.
* Visualizar archivos en formato hexadecimal.

## Referencias
Visite los siguientes sitios para obtener más información sobre EXT4 y ncurses:
- [EXT4](https://blogs.oracle.com/linux/post/understanding-ext4-disk-layout-part-1)
- [EXT4 Parte 2](https://blogs.oracle.com/linux/post/understanding-ext4-disk-layout-part-2)
- [Inodes](https://www.kernel.org/doc/html/next//filesystems/ext4/dynamic.html)

## Licencia
La licencia de este proyecto pertenece a la Universidad Autónoma Metropolitana unidad Iztapalapa.
Visite su portal oficial en el siguiente enlace: [https://www.izt.uam.mx/](https://www.izt.uam.mx/)

## Contacto

Si tienes alguna duda o sugerencia, puedes contactar a:

- [cbi2193052801@izt.uam.mx](mailto:cbi2193052801@izt.uam.mx)
- [axelhuertadev@proton.me](mailto:axelhuertadev@proton.me)
- [octasil94@gmail.com ](octasil94@gmail.com)
