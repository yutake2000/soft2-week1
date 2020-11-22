#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // sleep()関数を使う

/*
 ファイルによるセルの初期化: 生きているセルの座標が記述されたファイルをもとに2次元配列の状態を初期化する
 fp = NULL のときは、関数内で適宜定められた初期状態に初期化する。関数内初期値はdefault.lif と同じもの
 */
void my_init_cells(const int height, const int width, int cell[height][width], FILE* fp) {

  int is_default = (fp == NULL); // デフォルトファイルを読み込むかどうか

  if (is_default) {
    fp = fopen("default.lif","r");
    if (fp == NULL) {
      fprintf(stderr,"cannot open file %s\n", "default.lif");
      return;
    }
  }

  fscanf(fp, "%*[^\n]\n"); // バージョン情報は読み飛ばす

  int x, y;
  while (fscanf(fp, "%d%d", &x, &y) > 0) {
    cell[y][x] = 1;
  }

  if (is_default) {
    fclose(fp);
  }

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

  // 世代情報を表示
  fprintf(fp, "generateion = %d, alive:dead = %d:%d\r\n", gen, count_cells[1], count_cells[0]);

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
  }
  else if (argc == 2) {
    FILE *lgfile;
    if ((lgfile = fopen(argv[1],"r")) != NULL ) {
      my_init_cells(height,width,cell,lgfile); // ファイルによる初期化
    }
    else{
      fprintf(stderr,"cannot open file %s\n",argv[1]);
      return EXIT_FAILURE;
    }
    fclose(lgfile);
  }
  else{
    my_init_cells(height, width, cell, NULL); // デフォルトの初期値を使う
  }

  my_print_cells(fp, 0, height, width, cell); // 表示する
  sleep(1); // 1秒休止

  /* 世代を進める*/
  for (int gen = 1 ;; gen++) {
    my_update_cells(height, width, cell); // セルを更新
    my_print_cells(fp, gen, height, width, cell);  // 表示する
    sleep(1); //1秒休止する
    fprintf(fp,"\e[%dA",height+3);//height+3 の分、カーソルを上に戻す(壁2、表示部1)
  }

  return EXIT_SUCCESS;
}
