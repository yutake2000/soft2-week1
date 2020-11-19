#include <stdio.h>
#include <stdlib.h>

// 名前入力
// なぜこんなことがおこるのか？

typedef struct person{
  char name[10];
  unsigned char age;
} Person;

int main(int argc, char**argv)
{

  Person p = { .name = "hoge", .age = 28 };

  // printf でプロンプトを表示
  // scanf は改行以外の文字をまとめて読むように指示
  // その後、代入抑止 %* で、代入せずに読み込む
  // scanf はデフォルトでは空白文字（スペース、タブ、改行）を読み飛ばしてしまう
  // 改行のみを区切り文字にするため、
  printf("Input the name: ");
  scanf("%9[^\n]%*1[\n]",p.name);

  printf("age: %d\n",(int)p.age);
  printf("name: %s\n",p.name);

  return EXIT_SUCCESS;
}