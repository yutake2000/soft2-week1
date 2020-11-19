/* 境界つきで、グリッドを表示するプログラム */
/* この表示を元にターミナルサイズを調整しておく */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
  // 変更されない変数にはconstをつけておく
  // プログラム内のどこかで変数が変更されるとコンパイルエラーになる
  const int height = 40;
  const int width = 70;

  // C99 では可変長配列（VLA）がサポートされている
  int cell[height][width];

  // 初期化する
  // Macの場合のみ、int cell[height][width] = {0}; でもよいが...
  // 横方向にx, 縦方向にy をループ変数に用いる
  // cell[y][x] となることに注意
  for(int y = 0 ; y < height ; y++){
    for(int x = 0 ; x < width ; x++){
      cell[y][x] = 0;
    }
  }

  /* 適当に何箇所か生きているセルを置く */
  /* life game の引数なしの場合と同じ配置 */
  cell[20][20] = 1;
  cell[20][21] = 1;
  cell[20][22] = 1;

  //上の壁
  FILE *fp = stdout; // 標準出力に出す
  fprintf(fp,"[calibration]\n"); // この場合 (fp = stdout), printfと同じ
  fprintf(fp,"+");
  for (int x = 0 ; x < width ; x++)
    fprintf(fp, "-");
  fprintf(fp, "+\n");
  
  /* 外壁と 内側のゲーム部分 */
  for (int y = 0; y < height; y++) {
    fprintf(fp,"|");
    for (int x = 0; x < width; x++) {
      // ANSIエスケープコードを用いて、赤い"#" を表示
      // \e[31m で 赤色に変更
      // \e[0m でリセット（リセットしないと以降も赤くなる）
      if(cell[y][x]){
	fprintf(fp, "\e[31m#\e[0m");
      }
      else{
	fprintf(fp, " ");
      }
    }
    fprintf(fp,"|\n");
  }

  // 下の壁
  fprintf(fp, "+");
  for (int x = 0 ; x < width ; x++)
    fprintf(fp, "-");
  fprintf(fp, "+\n");
  
  fflush(fp); // バッファされている文字列を出力する
  return EXIT_SUCCESS; // stdlib.h で定義されている実行成功を表す整数マクロ: 実体は0
}
