#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // sleep()関数を使う
#include <time.h>
#include <string.h>

/*
  文字列strの最後がsuffixに一致するか判定する関数
*/
int ends_with(const char str[], const char suffix[]) {

  int len_str = strlen(str);
  int len_suf = strlen(suffix);

  if (len_str < len_suf) return 0;

  return (strcmp(str + len_str - len_suf, suffix) == 0);
}

/*
  文字列を10進数で表した時の長さを返す関数
  ただし、0の長さは0とする
*/
int number_len(int n) {
  int len = 0;
  while(n > 0) {
    len++;
    n /= 10;
  }

  return len;
}

/*
  文字が空白、タブ、CR、LFのいずれかなら1を返す関数
*/
int isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

int loadRLE(const int height, const int width, int cell[height][width], FILE *fp) {

  char buffer[(int)1e4+1]; // 1行は70文字以下だが念のため
  char rule_B[100];
  char rule_S[100];
  int y = 0, x = 0;
  int offsetY = 0, offsetX = 0;
  while(fgets(buffer, 1e4, fp) != NULL) {

    if (buffer[0] == '#') {
      if (buffer[1] == 'P' || buffer[1] == 'R') {
        sscanf(buffer+2, "%d%d", &offsetX, &offsetY);
        //printf("offset: %d %d\n", offsetX, offsetY);
        y = offsetY;
        x = offsetX;
      }
      continue;
    }

    if (buffer[0] == 'x' || buffer[0] == 'y' || buffer[0] == 'r') {
      /*
      int result = sscanf(buffer, "x = %d, y = %d, rule = B%[^/]/S%s", width, height, rule_B, rule_S);
      printf("%d\n", result);
      */
    } else {
      int offset = 0;
      while(1) {
        int len = 0;
        char c;

        while(isWhitespace(buffer[offset])) offset++;

        int result = sscanf(buffer+offset, "%d", &len);
        //printf("result: %d, len: %d\n", result, len);
        offset += number_len(len);

        if (len == 0) len++;

        result = sscanf(buffer+offset, "%c", &c);
        //printf("result: %d, c: %c\n", result, c);
        offset++;

        if (result <= 0 || c == '!') {
          break;
        } else if (c == '$') {
          y += len;
          x = offsetX;
        } else if (c == 'b') {
          x += len;
        } else if (c == 'o') {
          for (int i=0; i<len; i++) {
            cell[y][x] = 1;
            x++;
          }
        } else if (c == '\n' || c == '\r') {
          printf("CRLF");
          break;
        } else {
          fprintf(stderr,"Invalid syntax\n");
          return EXIT_FAILURE;
        }
      }
    }

  }

  return EXIT_SUCCESS;

}

/*
 ファイルによるセルの初期化: 生きているセルの座標が記述されたファイルをもとに2次元配列の状態を初期化する
 fp = NULL のときは、関数内で適宜定められた初期状態に初期化する。関数内初期値はdefault.lif と同じもの
 */
int my_init_cells(const int height, const int width, int cell[height][width], char filename[]) {

  if (filename[0] == 0) {
    /* ランダムに配置する */
    srand(time(NULL));

    for (int y=0; y<height; y++) {
      for (int x=0; x<width; x++) {
        cell[y][x] = (rand() % 10 == 0 ? 1 : 0);
      }
    }
  } else {

    FILE *fp = fopen(filename,"r");
    if (fp == NULL) {
      fprintf(stderr,"cannot open file %s\n", filename);
      return EXIT_FAILURE;
    }

    if (ends_with(filename, ".lif")) {

      fscanf(fp, "%*[^\n]\n"); // バージョン情報は読み飛ばす

      int x, y;
      while (fscanf(fp, "%d%d", &x, &y) > 0) {
        cell[y][x] = 1;
      }

    } else if (ends_with(filename, ".rle")) {

      int result = loadRLE(height, width, cell, fp);
      if (result != 0) return EXIT_FAILURE;

    } else {

      fprintf(stderr,"Supported: .lif .rle\n");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

/*
 グリッドの描画: 世代情報とグリッドの配列等を受け取り、ファイルポインタに該当する出力にグリッドを描画する
 */
void my_print_cells(FILE *fp, int gen, const int height, const int width, int cell[height][width]) {

  /* 0,1それぞれの状態のセルをカウント */
  int count_cells[2] = {0, 0};
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      count_cells[cell[y][x]]++;
    }
  }

  // 世代情報と存在比を表示
  fprintf(fp, "generateion = %d, alive:dead = %7d:%7d\r\n", gen, count_cells[1], count_cells[0]);

  /* 壁 */
  fprintf(fp, "+");
  for (int x=0; x<width; x++) fprintf(fp, "-");
  fprintf(fp, "+\r\n");

  /* グリッド */
  for (int y=0; y<height; y++) {
    fprintf(fp, "|");
    for (int x=0; x<width; x++) {
      fprintf(fp, "\e[31m%c\e[0m", (cell[y][x] ? '#' : ' ')); // 赤色で表示
    }
    fprintf(fp, "|\r\n");
  }

  /* 壁 */
  fprintf(fp, "+");
  for (int x=0; x<width; x++) fprintf(fp, "-");
  fprintf(fp, "+\r\n");

  fflush(fp);
}

int in_cells(int y, int x, const int height, const int width) {

  if (y < 0 || height <= y) return 0;
  if (x < 0 || width <= x) return 0;

  return 1;
}

/*
 着目するセルの周辺の生きたセルをカウントする関数
 */
int my_count_adjacent_cells(int y, int x, const int height, const int width, int cell[height][width]) {

  /*
    dy, dx: 相対位置
    012
    7.3
    654
  */
  int dy[] = {-1, -1, -1, 0, 1, 1, 1, 0};
  int dx[] = {-1, 0, 1, 1, 1, 0, -1, -1};

  int count = 0;
  
  for (int i=0; i<8; i++) {

    int ny = y + dy[i];
    int nx = x + dx[i];

    if (in_cells(ny, nx, height, width) && cell[ny][nx]) {
      count++;
    }

  }

  return count;
}

/*
  着目するセルの次の世代での状態を返す関数
*/
int next_state(int y, int x, const int height, const int width, int cell[height][width], int neighbors) {

  if (cell[y][x]) {
    return (neighbors == 2 || neighbors == 3);
  } else {
    return (neighbors == 3);
  }

}

/*
 ライフゲームのルールに基づいて2次元配列の状態を更新する
 */
void my_update_cells(const int height, const int width, int cell[height][width]) {

  int next_cell[height][width];
  for(int y = 0 ; y < height ; y++){
    for(int x = 0 ; x < width ; x++){
      next_cell[y][x] = 0;
    }
  }

  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      int neighbors = my_count_adjacent_cells(y, x, height, width, cell);
      next_cell[y][x] = next_state(y, x, height, width, cell, neighbors);
    }
  }

  for(int y = 0 ; y < height ; y++){
    for(int x = 0 ; x < width ; x++){
      cell[y][x] = next_cell[y][x];
    }
  }

}

int main(int argc, char **argv)
{
  FILE *fp = stdout;
  const int height = 40;
  const int width = 70;
  int h = 0, w = 0;

  int cell[height][width];
  for(int y = 0 ; y < height ; y++){
    for(int x = 0 ; x < width ; x++){
      cell[y][x] = 0;
    }
  }

  /* ファイルを引数にとるか、ない場合はデフォルトの初期値を使う */
  if ( argc > 2 ) {
    fprintf(stderr, "usage: %s [filename for init]\n", argv[0]);
    return EXIT_FAILURE;
  } else if (argc == 2) {
    int result = my_init_cells(height, width, cell, argv[1]);
    if (result != 0) return EXIT_FAILURE;
  } else{
    int result = my_init_cells(height, width, cell, ""); // デフォルトの初期値を使う
    if (result != 0) return EXIT_FAILURE;
  }

  my_print_cells(fp, 0, height, width, cell); // 表示する

  /* 世代を進める*/
  for (int gen = 1 ;; gen++) {
    my_update_cells(height, width, cell); // セルを更新
    my_print_cells(fp, gen, height, width, cell);  // 表示する
    usleep(200*1000); //0.2秒休止する
    fprintf(fp,"\e[%dA",height+3);//height+3 の分、カーソルを上に戻す(壁2、表示部1)
  }

  return EXIT_SUCCESS;
}
