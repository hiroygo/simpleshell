# simpleshell
## 概要
* シンプルなシェル
* パイプ `|` とリダイレクト `>` を実装している
* 環境変数は読み込まないので、コマンドはフルパスで記述する必要がある
* Ctrl+d でシェルを終了する

## 動作例
```
$ ./simpleshell 
> /bin/ls /dev | /bin/grep -E tty[0-9]$ > out.log
> /bin/cat out.log
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
> /bin/pwd
/home/cpp/simpleshell
> /bin/date
2020年 10月 19日 月曜日 23:56:41 JST
```