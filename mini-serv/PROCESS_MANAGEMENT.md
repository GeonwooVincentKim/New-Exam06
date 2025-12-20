# プロセス管理ガイド

## ポート1221を使用しているプロセスを確認・終了する方法

### 1. ポートを使用しているプロセスを確認

```bash
# 方法1: lsofを使用（インストールが必要な場合あり）
lsof -i :1221

# 方法2: netstatを使用
netstat -tulpn | grep :1221

# 方法3: ssを使用（現代的な方法）
ss -tulpn | grep :1221

# 方法4: fuserを使用
fuser 1221/tcp
```

### 2. プロセスを終了する

```bash
# 方法1: PIDが分かっている場合
kill <PID>

# 方法2: 強制終了
kill -9 <PID>

# 方法3: プロセス名で終了
pkill a.out
# または
killall a.out

# 方法4: ポートを使用しているプロセスを直接終了
fuser -k 1221/tcp
```

### 3. a.outやmini_serv関連のプロセスを確認

```bash
# 実行中のa.outプロセスを確認
ps aux | grep a.out | grep -v grep

# すべてのa.outプロセスを終了
pkill a.out
# または
killall a.out
```

### 4. よく使うコマンドの組み合わせ

```bash
# ポート1221を使用しているプロセスを確認して終了
PID=$(lsof -ti :1221)
if [ ! -z "$PID" ]; then
    echo "Killing process $PID"
    kill $PID
fi

# または一行で
lsof -ti :1221 | xargs kill
```

### 5. すべての関連プロセスを一度に終了

```bash
# a.outとmini_servのすべてのプロセスを終了
pkill -f "a.out|mini_serv"
```

## トラブルシューティング

### "Fatal error" が表示される場合

1. **ポートが既に使用されている**
   ```bash
   # ポートを確認
   lsof -i :1221
   # または
   ss -tulpn | grep :1221
   ```

2. **別のポートを試す**
   ```bash
   ./a.out 1222  # 別のポート番号
   ```

3. **権限の問題**
   - 1024未満のポートはroot権限が必要な場合があります
   - 1221は通常のユーザーで使用可能です

4. **ソケットがTIME_WAIT状態**
   ```bash
   # 少し待ってから再試行
   # または別のポートを使用
   ```
