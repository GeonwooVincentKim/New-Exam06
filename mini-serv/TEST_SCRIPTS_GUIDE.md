# テストスクリプト実行ガイド

このディレクトリには、`mini_serv`をテストするための複数のスクリプトが用意されています。各スクリプトの使い方を説明します。

## 前提条件

1. **コンパイル済みの実行ファイルが必要**
   ```bash
   cc -Wall -Werror -Wextra mini_serv.c -o mini_serv
   ```

2. **必要なツール**
   - `nc` (netcat) - クライアント接続用
   - `bash` - スクリプト実行用
   - `timeout` - 一部のテストで使用（オプション）

## テストスクリプト一覧

### 1. `simple_test.sh` - シンプルな基本テスト

**目的**: 最も基本的な動作確認（クライアント接続、メッセージ送信）

**実行方法**:
```bash
cd /mnt/e/Study/C/Cluster5/Exam/Exam06/New-Exam06/mini-serv
bash simple_test.sh
```

**テスト内容**:
- 2つのクライアントが接続できるか
- クライアント間でメッセージが送信できるか
- 切断通知が動作するか

**使用ポート**: 1225

**出力例**:
```
=== Simple Test ===
Starting server on port 1225
Server PID: 12345

Test 1: Two clients connecting
Client 1 output:
Client 2 output:
server: client 1 just arrived
client 1: World
server: client 0 just left

Stopping server...
```

---

### 2. `test_server.sh` - 複数のテストケース

**目的**: 複数のテストケースを順番に実行

**実行方法**:
```bash
bash test_server.sh
```

**テスト内容**:
1. クライアント1の接続テスト
2. クライアント2の接続とメッセージ送信テスト
3. ファイルからの入力テスト

**使用ポート**: 1222

**出力例**:
```
=== Starting server on port 1222 ===
Server started with PID: 12345

=== Test 1: Client 1 connecting ===
=== Test 2: Client 2 connecting and sending message ===
server: client 1 just arrived
client 1: Hello from client 2
server: client 0 just left
=== Test 3: Sending from file ===
server: client 2 just arrived
client 2: Hello from client 0
client 2: Line 2
client 2: Line 3

=== Tests completed ===
Server stopped
```

---

### 3. `detailed_test.sh` - 詳細な自動テスト

**目的**: 各機能を個別にテストし、PASS/FAILを判定

**実行方法**:
```bash
bash detailed_test.sh
```

**テスト内容**:
1. **テスト1**: クライアント到着通知の確認
   - クライアント1が接続中にクライアント2が接続
   - クライアント1が「server: client 1 just arrived」を受信するか確認

2. **テスト2**: メッセージブロードキャストの確認
   - クライアント1がメッセージを送信
   - クライアント2が「client 0: Hello from client 0」を受信するか確認

3. **テスト3**: 切断通知の確認
   - クライアント1が切断
   - クライアント2が「server: client 0 just left」を受信するか確認

**使用ポート**: 1223

**出力例**:
```
=== Detailed Test Suite ===
Starting server on port 1223
Server PID: 12345

Test 1: Client 1 should receive notification when Client 2 connects
✓ PASS: Client 1 received arrival notification

Test 2: Message broadcasting between clients
✓ PASS: Client 2 received message from Client 1

Test 3: Disconnect notification
✓ PASS: Client 2 received disconnect notification

=== Test Summary ===
```

**注意**: このテストは一時ファイル（`/tmp/test_*.txt`）を作成します。テスト終了時に自動的に削除されます。

---

### 4. `manual_test_guide.sh` - 手動テストガイド

**目的**: サーバーを起動し、手動でテストするためのガイドを表示

**実行方法**:
```bash
bash manual_test_guide.sh
```

**動作**:
1. サーバーを起動
2. テスト手順を表示
3. Enterキーを押すまで待機
4. Enterキーでサーバーを停止

**使用ポート**: 1226

**出力例**:
```
=== Manual Test Guide ===

This script will start the server and provide instructions for manual testing.

Server will start on port 1226

Server started with PID: 12345

=== Test Instructions ===

1. Open a NEW terminal and run:
   nc 127.0.0.1 1226
   This will be Client 1 (ID: 0)

2. Open ANOTHER terminal and run:
   nc 127.0.0.1 1226
   This will be Client 2 (ID: 1)
   Client 1 should see: 'server: client 1 just arrived'

3. In Client 1 terminal, type: 'Hello' and press Enter
   Client 2 should see: 'client 0: Hello'

4. In Client 2 terminal, type: 'World' and press Enter
   Client 1 should see: 'client 1: World'

5. In Client 1 terminal, press Ctrl+C to disconnect
   Client 2 should see: 'server: client 0 just left'

Press Enter when you're done testing...
```

**使い方**:
1. スクリプトを実行
2. 表示された手順に従って、別のターミナルで`nc`コマンドを実行
3. テストが完了したら、スクリプトを実行したターミナルでEnterキーを押す

---

## トラブルシューティング

### スクリプトが実行できない場合

**問題**: `bad interpreter: No such file or directory`
```bash
# 改行コードを修正
sed -i 's/\r$//' test_server.sh
# または
dos2unix test_server.sh
```

**問題**: 権限エラー
```bash
# 実行権限を付与
chmod +x test_server.sh
```

### ポートが既に使用されている場合

**問題**: `Fatal error`が表示される
```bash
# 使用中のプロセスを確認
lsof -i :1222
# または
ss -tulpn | grep :1222

# プロセスを終了
pkill -f "mini_serv.*1222"
```

### テストが失敗する場合

1. **サーバーが正しくコンパイルされているか確認**
   ```bash
   cc -Wall -Werror -Wextra mini_serv.c -o mini_serv
   ```

2. **サーバーが起動しているか確認**
   ```bash
   ps aux | grep mini_serv
   ```

3. **ポート番号が正しいか確認**
   - 各スクリプトの`PORT`変数を確認

4. **`nc`コマンドが利用可能か確認**
   ```bash
   which nc
   # または
   nc --version
   ```

---

## 推奨されるテスト順序

1. **最初に**: `simple_test.sh`で基本動作を確認
2. **次に**: `test_server.sh`で複数のテストケースを実行
3. **詳細確認**: `detailed_test.sh`で各機能を個別にテスト
4. **手動確認**: `manual_test_guide.sh`で実際の動作を目視確認

---

## カスタマイズ

各スクリプトのポート番号を変更するには、スクリプト内の`PORT`変数を編集してください：

```bash
PORT=1222  # この値を変更
```

複数のテストを同時に実行する場合は、異なるポート番号を使用してください。
