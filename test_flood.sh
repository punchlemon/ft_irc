#!/bin/bash

# IRCサーバーの設定
IRC_HOST="localhost"
IRC_PORT="6667"
CHANNEL_NAME="#floodtest"
FLOOD_NICK="FloodBotB"
MESSAGE_COUNT=10000    # 送信するメッセージ数 (サーバー制限に達する量)

echo "--- IRC フラッディング Publisher スクリプト ---"
echo "ターゲット: $IRC_HOST:$IRC_PORT / チャネル: $CHANNEL_NAME"
echo "注意: irssi クライアント (Client A) が Ctrl+Z で一時停止していることを確認してください。"
read -p "準備ができたら Enter を押してください..."

# Publisher は別のソケットからメッセージを送信
(
    printf "PASS password\r\n"
    printf "NICK %s\r\n" "$FLOOD_NICK"
    printf "USER flood 0 * :Flood Bot\r\n"
    printf "JOIN %s\r\n" "$CHANNEL_NAME"

    # Bash の算術ループで効率的に回す
    for ((i=1; i<=MESSAGE_COUNT; ++i)); do
        printf "PRIVMSG %s :%d: FLOOD TEST MESSAGE\r\n" "$CHANNEL_NAME" "$i"
    done

    sleep 10

) | nc -N $IRC_HOST $IRC_PORT
