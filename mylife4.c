/*===========================================================

羊と草地のシミュレーション
(1Aのアルゴリズム入門でPythonの課題として出されたもの)

セルの状態を空き地(0)、草地(1)、羊(2)の3パターン。
草地は緑の「w」、羊は白の「#」で表す。

ルールは以下の通り
羊:
  隣接8セルに草地がなければ死ぬ。1カ所以上なら生きて仔を生む。
  仔は乱数で決まる隣接8セルのどこかに生まれる。
  もしそこに既に羊がいたら生まれない。
  そこが草地か空き地なら生まれる。
空き地:
  隣接8セルに草が2カ所以上なら草が生える。
  そうでない場合も、1/1000の確率で生える。
草地:
  仔が生まれない限りそのまま。

引数
  arg[1] 初期状態での草地の割合(%)
  arg[2] 初期状態での羊の割合(%)

実行例
  ./a.out 80 10
    ほとんどの場合最初に羊が草を食べ尽くし絶滅する。
  ./a.out 1 20
    羊が前線のように草を追いかけて移動していく様子がわかる。
    端まで食べ尽くすと小さな集団がたくさん現れる。

===========================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // sleep()関数を使う
#include <time.h>
#include <string.h>

/*
 ファイルによるセルの初期化: ランダムで作成
 */
void my_init_cells(const int height, const int width, int cell[height][width], int glass_rate, int sheep_rate) {

  /* ランダムに配置する */
  srand(time(NULL));

  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      int r = rand() % 100;
      if (r < glass_rate) {
        cell[y][x] = 1;
      } else if (r < glass_rate + sheep_rate) {
        cell[y][x] = 2;
      } else {
        cell[y][x] = 0;
      }
    }
  }

}

/*
 グリッドの描画: 世代情報とグリッドの配列等を受け取り、ファイルポインタに該当する出力にグリッドを描画する
 */
void my_print_cells(FILE *fp, int gen, const int height, const int width, int cell[height][width]) {

  /* 0,1それぞれの状態のセルをカウント */
  int count_cells[3] = {0, 0, 0};
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      count_cells[cell[y][x]]++;
    }
  }

  // 世代情報と存在比を表示
  fprintf(fp, "generateion = %d, none:glass:sheep = %7d:%7d:%7d\r\n", gen, count_cells[0], count_cells[1], count_cells[2]);

  /* 壁 */
  fprintf(fp, "+");
  for (int x=0; x<width; x++) fprintf(fp, "-");
  fprintf(fp, "+\r\n");

  /* グリッド */
  for (int y=0; y<height; y++) {
    fprintf(fp, "|");
    for (int x=0; x<width; x++) {
      if (cell[y][x] == 1) {
        fprintf(fp, "\e[32mw\e[0m"); // 草:緑で表示
      } else if (cell[y][x] == 2) {
        fprintf(fp, "\e[37m#\e[0m"); // 羊:白で表示
      } else {
        fprintf(fp, " ");
      }
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
 着目するセルの周辺の草地と羊をカウントする関数
 */
void my_count_adjacent_cells(int y, int x, const int height, const int width, int cell[height][width], int *glass, int *sheep) {

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

    if (in_cells(ny, nx, height, width)) {
      if (cell[ny][nx] == 1) {
        (*glass)++;
      } else if (cell[ny][nx] == 2) {
        (*sheep)++;
      }
    }

  }

}

/*
  着目するセルの次の世代での状態を返す関数
  仔を生む場合、childY,childXにその座標を書き込む
*/
int next_state(int y, int x, const int height, const int width, int cell[height][width], int glass, int sheep, int *childY, int *childX) {

  if (cell[y][x] == 1) {
    return 1;
  } else if (cell[y][x] == 2) {
    if (glass == 0) {
      return 0;
    } else {
      do {
        *childY = y - 1 + rand() % 3;
        *childX = x - 1 + rand() % 3;
      } while(!in_cells(*childY, *childX, height, width) || (*childY == y && *childX == x));
      return 2;
    }
  } else {
    if (glass >= 2) return 1;
    return (rand() % 1000 == 0 ? 1: 0);
  }

}

/*
 ルールに基づいて2次元配列の状態を更新する
 */
void my_update_cells(const int height, const int width, int cell[height][width]) {

  int next_cell[height][width];
  for(int y = 0 ; y < height ; y++){
    for(int x = 0 ; x < width ; x++){
      next_cell[y][x] = -1; // 初期状態は-1とする
    }
  }

  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {

      // 仔がその場所に生まれていて、既に書き換えられている場合
      if (next_cell[y][x] != -1) continue;

      int glass = 0, sheep = 0;
      my_count_adjacent_cells(y, x, height, width, cell, &glass, &sheep);

      int childY = -1, childX = -1;
      next_cell[y][x] = next_state(y, x, height, width, cell, glass, sheep, &childY, &childX);

      // 既に羊がいなければ仔が生まれる
      if (childY != -1 && cell[childY][childX] != 2) {
          next_cell[childY][childX] = 2;
      }
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

  if (argc != 3) {
    fprintf(stderr, "usage: %s [the rate of glass] [the rate of sheep]\n", argv[0]);
    return EXIT_FAILURE;
  }

  int cell[height][width];
  for(int y = 0 ; y < height ; y++){
    for(int x = 0 ; x < width ; x++){
      cell[y][x] = 0;
    }
  }

  my_init_cells(height, width, cell, atoi(argv[1]), atoi(argv[2]));

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
