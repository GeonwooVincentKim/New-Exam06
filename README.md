# Mini DB

## コンパイル方法

このプロジェクトはC++で書かれており、以下の方法でコンパイルできます。

### 方法1: g++を使用（推奨）

```bash
g++ -Wall -Wextra -Werror mini_db.cpp -o mini_db
```

`g++`はC++コンパイラで、C++標準ライブラリを自動的にリンクします。

### 方法2: gccを使用

```bash
gcc -Wall -Wextra -Werror -lstdc++ mini_db.cpp -o mini_db
```

`gcc`でC++コードをコンパイルする場合、`-lstdc++`オプションでC++標準ライブラリを明示的にリンクする必要があります。

**注意**: `gcc -Wall -Wextra -Werror mini_db.cpp`だけでは、C++標準ライブラリがリンクされずにリンクエラーが発生します。

## 実行方法

```bash
./mini_db <port> <database_file>
```

- `port`: サーバーがリッスンするポート番号
- `database_file`: データベースファイルのパス

## 使用方法

サーバーは以下のコマンドを受け付けます：

- `POST <key> <value>`: キーと値を保存
- `GET <key>`: キーの値を取得
- `DELETE <key>`: キーを削除

Ctrl+Cでサーバーを終了すると、データベースがファイルに保存されます。

