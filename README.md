# ft_irc

このリポジトリはシンプルな IRC サーバ（C++）の実装です。ここではローカル環境で Linux コンテナ（Docker）を使ってビルド・実行するための手順をまとめます。

## 前提
- Docker がインストールされていること（必要に応じて `docker-compose` も）。
- ポート `6667` が他プロセスで使われていないこと。

## イメージのビルド
リポジトリのルートで次を実行します。

```bash
# 通常ビルド
docker build -t ft_irc:latest .

# 特定プラットフォームを指定する例（必要な場合）
docker build --platform linux/amd64 -t ft_irc:latest .
```

ビルドは Dockerfile 内の `make` を実行してバイナリ `ircserv` を生成します。

## コンテナの起動（シンプル）
```bash
# 既に同名コンテナがあれば削除
docker rm -f ft_irc_test 2>/dev/null || true

# 起動（ホストの6667 をコンテナの6667へマップ）
docker run -d --name ft_irc_test -p 6667:6667 ft_irc:latest
```

起動後、ログを見てサーバが起動したことを確認します。

```bash
docker logs --tail 200 ft_irc_test
# 期待される出力例: "Server started on port 6667"
```

ホスト側から接続確認:

```bash
nc -vz 127.0.0.1 6667
```

## docker-compose を使う
リポジトリに `docker-compose.yml` がある場合は以下で起動できます。

```bash
docker-compose up -d --build
docker-compose logs -f
```

## 開発時の使い方（ソースをホストで編集してコンテナ内でビルド／実行）
ソースをコンテナにマウントして手動でビルド・実行する方法の一例です。

```bash
# コンテナでシェルを起動してソースをマウント
docker run -it --rm --name ft_irc_dev -v "$(pwd)":/usr/src/app -w /usr/src/app ubuntu:22.04 /bin/bash

# コンテナ内で（最初に必要なパッケージを入れるか、Dockerfile を変える）
apt update && apt install -y build-essential g++ make
make
./ircserv 6667 password
```

## 環境変数やパスワード管理
- 現在の Dockerfile / docker-compose はサンプルとして `password` を実行コマンドで渡すようになっています。運用時は環境変数やシークレット管理を用いてください。

## 再ビルド／デプロイ手順
コードを変更したらイメージを再ビルドしてコンテナを再作成します。

```bash
docker build -t ft_irc:latest .
docker rm -f ft_irc_test 2>/dev/null || true
docker run -d --name ft_irc_test -p 6667:6667 ft_irc:latest
```

## トラブルシューティング
- Permission denied（docker）: `sudo` を付けるか、ユーザーを `docker` グループに追加して再ログインしてください: `sudo usermod -aG docker $USER`。
- ポートが取れない: 他プロセスが `6667` を使っている可能性があります。`ss -ltnp | grep 6667`（Linux）等で確認してください。
- ビルドエラー: Dockerfile のビルドログを確認し、必要なビルドツール（g++, make 等）が入っているか確かめてください。
- Mac / Apple Silicon でアーキテクチャ不一致が出る場合は `--platform` を指定するか、ローカル環境で別イメージを作成してください。
