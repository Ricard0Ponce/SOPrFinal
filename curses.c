#include <curses.h>
#include "ext4.h" // Sistema usamos: < >, local usamos " " 

int leeChar() {
  int chars[5];
  int ch, i = 0;
  nodelay(stdscr, TRUE);
  while ((ch = getch()) == ERR); /* Espera activa */
  ungetch(ch);
  while ((ch = getch()) != ERR) {
    chars[i++] = ch;
  }
  /* Convierte a numero con todo lo leido */
  int res = 0;
  for (int j = 0; j < i; j++) {
    res <<= 8;
    res |= chars[j];
  }
  return res;
}

int main() {
  struct info;
  
  /* Block size */ 
  int tam; 
  char *lista[] = {
    "0          0|32|33     83  32|162|3        2048           522241",
    "1          32|194|35    7  65|69|4       526336           522240",
    "2          0|0|0        0   0|0|0             0                0",
    "3          0|0|0        0   0|0|0             0                0"
  };
  int i = 0;
  int c;
  initscr();
  raw();
  noecho(); /* No mostrar el caracter leido */
  cbreak(); /* Hacer que los caracteres se le pasen al usuario */
  //start_color();
  //init_pair(1, COLOR_WHITE, COLOR_BLUE);
  //bkgd(COLOR_PAIR(1));

  /* Imprimir cabecera */
  attron(A_BOLD);
  mvprintw(2, 2, "Particion  CHS        Tipo      CHS         LBA              TAM");
  attroff(A_BOLD);

  do {
    for (int j = 0; j < 4; j++) {
      if (j == i) {
        attron(A_REVERSE);
      }
      mvprintw(4 + j, 2, lista[j]);
      if (j == i) {
        attroff(A_REVERSE);
      }
    }
    move(4 + i, 2);
    refresh();
    c = leeChar();
    switch (c) {
      case 0x1B5B41: /* Flecha arriba */
        i = (i > 0) ? i - 1 : 3;
        break;
      case 0x1B5B42: /* Flecha abajo */
        i = (i < 3) ? i + 1 : 0;
        break;
      default:
        // Nada
        break;
    }
    move(10, 2);
    printw("Estoy en %d: Lei %d", i, c);
  } while (c != 'q');
  endwin();
  return 0;
}
