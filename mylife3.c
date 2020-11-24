/*================================================================================================

RLEファイルの読み込みに対応

ファイルの拡張子がrle(大文字でも可)ならRLEフォーマットとして、lifならLife 1.06フォーマットとして読み込む。

ヘッダー情報は基本読み飛ばすが、#P,#Rで指定されたオフセット情報は適用する(0以上の場合のみ)。

パターンのサイズは読み飛ばすが、B3/S23などのルールは反映する。
  can_survive[i]は、隣接iマスが生存しているときに生存できるなら1。
  can_born[i]は、隣接iマスが生存しているときに誕生できるなら1。
(Bomber.rleはB36/S23のHighLifeというルールで動く。)

fgetsで1行ずつ受け取ってからsscanfで情報を読み取っている(詳しくはloadRLE()を参照)。

wikiによると「2b 3o」のように間に空白が入ることも許されているようなので、念のためそれにも対応した。
(Pulsar.rleはわざと間に空白を入れてある。)

================================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // sleep()関数を使う
#include <time.h>
#include <string.h>

/* デフォルトのルール */
int can_survive[9] = {0, 0, 1, 1, 0};
int can_born[9] = {0, 0, 0, 1, 0};
char rule_S[10] = "23";
char rule_B[10] = "3";

/*
  文字列strの最後がsuffixに一致するか判定する関数
*/
int ends_with(const char str[], const char suffix[]) {

  int len_str = strlen(str);
  int len_suf = strlen(suffix);

  if (len_str < len_suf) return 0;

  char lower_str[255];
  char lower_suffix[255];
  strcpy(lower_str, str);
  strcpy(lower_suffix, suffix);

  /* 大文字は全て小文字にする */
  for (int i=0; lower_str[i] != 0; i++) {
    if ('A' <= lower_str[i] && lower_str[i] <= 'Z') {
      lower_str[i] += 'a' - 'A';
    }
  }
  for (int i=0; lower_suffix[i] != 0; i++) {
    if ('A' <= lower_suffix[i] && lower_suffix[i] <= 'Z') {
      lower_suffix[i] += 'a' - 'A';
    }
  }

  return (strcmp(lower_str + len_str - len_suf, lower_suffix) == 0);
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
  int y = 0, x = 0;
  int offsetY = 0, offsetX = 0;
  while(fgets(buffer, 1e4, fp) != NULL) {

    /* オフセット指定以外のヘッダー情報は読み飛ばす */
    if (buffer[0] == '#') {
      if (buffer[1] == 'P' || buffer[1] == 'R') {
        sscanf(buffer+2, "%d%d", &offsetX, &offsetY);
        if (offsetY < 0 || offsetX < 0) {
          offsetY = 0;
          offsetX = 0;
        } else {
          y = offsetY;
          x = offsetX;
        }
      }
      continue;
    }

    /* サイズ情報は読み飛ばし、ルールは反映する */
    if (buffer[0] == 'x') {
      int result = sscanf(buffer, "x = %*d, y = %*d, rule = B%9[^/n]/S%9s", rule_B, rule_S);
      printf("%s %s", rule_B, rule_S);

      for (int i=0; i<=8; i++) {
        can_survive[i] = 0;
        can_born[i] = 0;
      }

      for (int i=0; i<10; i++) {
        int s = rule_S[i] - '0';
        int b = rule_B[i] - '0';
        if (0 <= s && s <= 8) can_survive[s] = 1;
        if (0 <= b && b <= 8) can_born[b] = 1;
      }

      continue;
    }

    int offset = 0; // 行の何文字目を次に読むか
    while(1) {
      int len = 0; // ランの長さ
      char c; // タグ('o', 'b', '$', '!'のいずれか)

      // 途中に空白が入っても対応可能
      while(isWhitespace(buffer[offset])) offset++;

      // ランの長さを取得(1が省略されている場合は何も読み込まない)
      int result = sscanf(buffer+offset, "%d", &len);
      // 読み込んだ分だけoffsetを加算(ただし0のままの場合は読みこんでないのでそのまま)
      offset += number_len(len);

      // 長さ省略時は1
      if (len == 0) len = 1;

      // ランのタグを取得
      result = sscanf(buffer+offset, "%c", &c);
      offset++;

      if (result <= 0 || c == '!') { // 終了
        break;
      } else if (c == '$') { // 改行
        y += len;
        x = offsetX;
      } else if (c == 'b') { // dead
        x += len;
      } else if (c == 'o') { // alive
        for (int i=0; i<len; i++) {
          cell[y][x] = 1;
          x++;
        }
      } else {
        fprintf(stderr,"Invalid syntax\n");
        return EXIT_FAILURE;
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
  fprintf(fp, "rule: B%s/S%s, generateion = %d, alive:dead = %7d:%7d\r\n", rule_B, rule_S, gen, count_cells[1], count_cells[0]);

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
    return can_survive[neighbors];
  } else {
    return can_born[neighbors];
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
