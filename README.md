# simpleshell
## 概要
* シンプルなシェル
* パイプ `|` とリダイレクト `>` を実装している
* 環境変数 PATH を読み込むためコマンドのフルパス表記は不要
* Ctrl+d でシェルを終了する

## 動作例
```
$ ./simpleshell 
> ls /dev | grep -E tty[0-9]$ > out.log
> cat out.log
tty0
tty1
tty2
tty3
tty4
tty5
tty6
tty7
tty8
tty9
> pwd
/home/cpp/simpleshell
> date
2020年 10月 19日 月曜日 23:56:41 JST
```