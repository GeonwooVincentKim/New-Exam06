# mini_serv (Exam 06)

A simple chat server implementation in C that listens for clients on a specific port (127.0.0.1) and broadcasts messages between them. This project demonstrates the use of `select()` for non-blocking I/O multiplexing.

## Features
- Manages multiple client connections simultaneously.
- Broadcasts messages from one client to all other connected clients.
- Notifies when a client joins or leaves the chat.
- Handles partial messages and multiple lines correctly.
- No memory leaks (Proper cleanup on disconnect).

## Compilation
Compile the program using `gcc`:

```bash
gcc -Wall -Wextra -Werror mini_serv.c -o mini_serv
```

## Usage
Run the server by specifying a port number:

```bash
./mini_serv <port>
```

Example:
```bash
./mini_serv 8080
```

## Testing

### 1. Manual Testing with Netcat
You can open multiple terminal windows to simulate different clients.

**Terminal 1 (Server):**
```bash
./mini_serv 8080
```

**Terminal 2 (Client 1):**
```bash
nc 127.0.0.1 8080
```

**Terminal 3 (Client 2):**
```bash
nc 127.0.0.1 8080
```
*Anything typed in Client 1 should appear in Client 2, and vice versa.*

### 2. Automated Testing Suite
A comprehensive test script is included to verify all functionalities (Notification, Broadcasting, Partial messages, etc.).

**How to run:**
```bash
chmod +x comprehensive_test.sh
./comprehensive_test.sh
```

**What it tests:**
1.  **Connection Notifications:** Checks if existing clients are notified when a new client joins.
2.  **Broadcasting:** Verifies that messages sent by one client are received by others.
3.  **Disconnect Notifications:** Checks if clients are notified when someone leaves.
4.  **Partial Messages:** Ensures the server buffers split messages correctly.
5.  **Multi-line Messages:** Ensures the server processes multiple newline-separated messages correctly.

## Implementation Details
- Uses `select()` to handle multiple file descriptors without blocking.
- `FD_ISSET`, `FD_SET`, `FD_CLR`, `FD_ZERO` macros are used for managing the file descriptor sets.
- `SO_REUSEADDR` is used to allow immediate restart of the server on the same port.

提供された `mini_db` の解説構成をベースに、`mini_serv.c` の動作と構造をまとめたREADMEを作成しました。

試験対策として、特に「メッセージの断片化（Partial Read）への対応」と「一斉送信（Broadcast）」のロジックに注目して構成しています。

---

# mini_serv.c 構造解説

このプログラムは、`select()` を利用したマルチクライアント対応のチャットサーバーです。複数のクライアントが接続し、誰かが送ったメッセージが自分以外の全員に転送されます。

## 1. プログラム起動時の流れ

`main()` 開始
↓
**引数チェック** (ポート番号のみ)
↓
**ソケット作成** (`socket()`)
↓
**バインド・リッスン開始**

* `127.0.0.1` (localhost) に固定
* `bind()` + `listen()`
↓
**監視セットの初期化**
* `FD_ZERO(&active)`
* `FD_SET(sockfd, &active)`
* `max_fd = sockfd`
↓
**メインループ開始** (`while(1)`)

## 2. グローバル変数とデータ管理

* `int ids[65536]`: ファイルディスクリプタ(fd)をキーに、クライアントのIDを保存。
* `char *msgs[65536]`: 各クライアントから届いた「未完成のメッセージ」を保存するバッファ（動的メモリ）。
* `fd_set active`: 現在接続中の全fdを管理するマスターセット。
* `char buf_read`, `msg_buf`: 受信・整形用の静的バッファ。

## 3. サーバーの動作フロー（メインループ）

`select()` で読み込み可能なfdを待機
↓
**全fdをループ巡回 (`0` ～ `max_fd`)**
↓
┌─────────────────────────────────────┐
│ **新規接続?** (`fd == sockfd`)         │
│ 1. `accept()` で接続許可              │
│ 2. `FD_SET` で監視に追加               │
│ 3. `ids[conn] = count++` でID割当      │
│ 4. `notify()` で「just arrived」を全員に送信 │
└─────────────────────────────────────┘
↓
┌─────────────────────────────────────┐
│ **既存クライアントからのデータ受信?** │
│ 1. `recv()` で最大1000バイト受信       │
│ 2. **切断 (`ret <= 0`) の場合**:       │
│    - `notify()` で「just left」を送信  │
│    - `free(msgs[fd])`, `FD_CLR`, `close` │
│ 3. **受信 (`ret > 0`) の場合**:        │
│    - `str_join()` で既存バッファに結合  │
│    - `while(extract_message)` でループ  │
│    - `notify()` で完成した各行を全員に転送 │
└─────────────────────────────────────┘

## 4. 関数構造

### `fatal()`

* エラー発生時に `Fatal error` を標準エラー出力に書き込み、`exit(1)` する。

### `notify(int sender, char *s)`

* `active` セット内の全fdを確認。
* 「リスニングソケット自身」と「送信者自身」**以外**の全員に `send()` する。

### `str_join(char *buf, char *add)`

* 受信したデータを `msgs[fd]` に追加するための動的メモリ確保関数。

### `extract_message(char **buf, char **msg)`

* **最重要関数。** バッファ内に `\n` があるか探し、あればそこまでをメッセージとして切り出し、残りをバッファに戻す。
* これにより、1回で2行届いた場合や、1行が2回に分かれて届いた場合でも正しく処理できる。

## 5. 試験で間違えやすい重要ポイント

1. **Partial Readの処理**: `recv` したデータを直接転送せず、必ず `msgs[fd]` に溜めてから `extract_message` で改行ごとに処理すること。
2. **メモリ管理**: クライアントが切断された際、`msgs[fd]` を `free` しないとメモリリークになる。
3. **selectの引数**: `select(max_fd + 1, ...)` の `+1` を忘れないこと。
4. **自分を除外**: `notify` 関数内で、自分自身（sender）にメッセージを送らないように条件分岐すること。
5. **127.0.0.1**: IPアドレスは `htonl(2130706433)`（127.0.0.1）を直接指定するか、適切な変換を行う。

---

このREADMEの内容（特に3番のフローと5番の注意点）を頭に入れておけば、`mini_serv.c` の暗記効率が格段に上がります。

次は、この `mini_serv.c` の中で最も複雑な **「受信したデータの処理（`else` ブロック内）」の穴埋め問題** などに挑戦してみますか？ Would you like me to ...?
